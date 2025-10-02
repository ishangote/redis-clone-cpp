#include "network/server.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <pthread.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cctype>
#include <cerrno>
#include <csignal>
#include <cstring>
#include <iostream>
#include <mutex>
#include <sstream>
#include <stdexcept>
#include <thread>

// Declare the global running flag defined in main.cpp
extern volatile sig_atomic_t g_running;

namespace redis_clone {
namespace network {

RedisServer::RedisServer(int port) : port_(port), server_fd_(-1) { initialize_server(); }

void RedisServer::initialize_server() {
    server_fd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd_ < 0) {
        throw std::runtime_error("Failed to create socket: " + std::string(strerror(errno)));
    }

    // Allow port reuse
    int opt = 1;
    if (setsockopt(server_fd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        close(server_fd_);
        throw std::runtime_error("Failed to set socket options: " + std::string(strerror(errno)));
    }

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port_);

    if (bind(server_fd_, reinterpret_cast<struct sockaddr*>(&address), sizeof(address)) < 0) {
        close(server_fd_);
        throw std::runtime_error("Failed to bind to port " + std::to_string(port_) + ": " +
                                 std::string(strerror(errno)));
    }

    if (listen(server_fd_, SOMAXCONN) < 0) {
        close(server_fd_);
        throw std::runtime_error("Failed to listen: " + std::string(strerror(errno)));
    }
}

void RedisServer::run() {
    while (g_running) {
        int client_fd = accept(server_fd_, nullptr, nullptr);
        if (client_fd < 0) {
            if (errno == EINTR) {
                // Interrupted system call (probably due to Ctrl+C)
                // Check g_running and continue if we should keep running
                if (g_running)
                    continue;
                else
                    break;
            }
            throw std::runtime_error("Failed to accept connection: " +
                                     std::string(strerror(errno)));
        }

        try {
            std::thread client_thread(&RedisServer::handle_client, this, client_fd);
            client_thread.detach();
        } catch (const std::exception& e) {
            // Log error but continue serving other clients
            std::cerr << "Error handling client: " << e.what() << std::endl;
            close(client_fd);
        }
    }

    // Cleanup when shutting down
    std::cout << "Server shutting down..." << std::endl;
    close(server_fd_);
}

void RedisServer::handle_client(int client_fd) {
    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set, SIGINT);
    sigaddset(&set, SIGTERM);
    pthread_sigmask(SIG_BLOCK, &set, nullptr);

    std::string command_buffer;

    constexpr size_t buffer_size = 1024;
    char buffer[buffer_size];

    while (true) {
        ssize_t bytes_read = recv(client_fd, buffer, buffer_size - 1, 0);
        if (bytes_read <= 0) {
            if (bytes_read < 0) {
                std::cerr << "Client disconnected unexpectedly: " + std::string(strerror(errno));
            }
            close(client_fd);
            return;  // Client closed connection
        }

        command_buffer.append(buffer, bytes_read);
        // Check for complete commands (ending with \r\n or \n)
        size_t pos;
        while ((pos = command_buffer.find('\n')) != std::string::npos) {
            // Extract one complete command
            std::string complete_command = command_buffer.substr(0, pos);

            // Remove processed command from buffer (including the \n)
            command_buffer.erase(0, pos + 1);

            // Remove \r if present (handle \r\n)
            if (!complete_command.empty() && complete_command.back() == '\r') {
                complete_command.pop_back();
            }

            // Process this complete command
            if (!complete_command.empty()) {
                CommandParts parts = extract_command(complete_command);

                if (parts.command == "QUIT") {
                    std::string response = "+OK\r\n";
                    send_command(client_fd, response);
                    close(client_fd);
                    return;
                }

                std::string response = process_command(complete_command);
                send_command(client_fd, response);
            }
        }
    }
    close(client_fd);
}

std::string RedisServer::process_command(const std::string& command) {
    std::istringstream iss(command);
    std::string cmd;
    iss >> cmd;

    // Convert command to uppercase for case-insensitive comparison
    for (char& c : cmd) {
        c = std::toupper(c);
    }

    std::lock_guard<std::mutex> lock(db_mutex_);

    if (cmd == "SET") {
        std::string key, value;
        if (!(iss >> key >> value)) {
            return "-ERR wrong number of arguments for 'set' command\r\n";
        }
        db_.set(key, value);
        return "+OK\r\n";
    } else if (cmd == "GET") {
        std::string key;
        if (!(iss >> key)) {
            return "-ERR wrong number of arguments for 'get' command\r\n";
        }
        auto result = db_.get(key);
        if (result.has_value()) {
            return "$" + std::to_string(result->size()) + "\r\n" + *result + "\r\n";
        }
        return "$-1\r\n";  // Redis null bulk string
    } else if (cmd == "DEL") {
        std::string key;
        if (!(iss >> key)) {
            return "-ERR wrong number of arguments for 'del' command\r\n";
        }
        return ":" + std::to_string(db_.del(key)) + "\r\n";
    } else if (cmd == "EXISTS") {
        std::string key;
        if (!(iss >> key)) {
            return "-ERR wrong number of arguments for 'exists' command\r\n";
        }
        return ":" + std::to_string(db_.exists(key)) + "\r\n";
    } else if (cmd == "QUIT") {
        return "+OK\r\n";
    }

    return "-ERR unknown command '" + cmd + "'\r\n";
}

RedisServer::CommandParts RedisServer::extract_command(const std::string& input) {
    std::istringstream iss(input);
    std::string cmd, key, value;

    iss >> cmd;
    if (iss >> key) {
        iss >> value;  // This might be empty for some commands
    }

    // Convert command to uppercase for consistency
    for (char& c : cmd) {
        c = std::toupper(c);
    }

    return {cmd, key, value};
}

void RedisServer::send_command(int client_fd, const std::string& response) {
    if (send(client_fd, response.c_str(), response.size(), 0) < 0) {
        std::cerr << "Failed to send response to client: " << std::string(strerror(errno))
                  << std::endl;
    }
}

}  // namespace network
}  // namespace redis_clone