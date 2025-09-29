#include "network/server.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cerrno>
#include <cstring>
#include <iostream>
#include <sstream>
#include <stdexcept>

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
            handle_client(client_fd);
        } catch (const std::exception& e) {
            // Log error but continue serving other clients
            std::cerr << "Error handling client: " << e.what() << std::endl;
        }

        close(client_fd);
    }

    // Cleanup when shutting down
    std::cout << "Server shutting down..." << std::endl;
    close(server_fd_);
}

void RedisServer::handle_client(int client_fd) {
    // TODO: Consider enhancing to maintain persistent connections. Currently closes after one command.
    constexpr size_t buffer_size = 1024;
    char buffer[buffer_size];

    ssize_t bytes_read = recv(client_fd, buffer, buffer_size - 1, 0);
    if (bytes_read <= 0) {
        if (bytes_read < 0) {
            throw std::runtime_error("Error reading from client: " + std::string(strerror(errno)));
        }
        return;  // Client closed connection
    }

    std::string command(buffer, bytes_read);
    std::string response = process_command(command);

    if (send(client_fd, response.c_str(), response.size(), 0) < 0) {
        throw std::runtime_error("Error sending response: " + std::string(strerror(errno)));
    }
}

std::string RedisServer::process_command(const std::string& command) {
    std::istringstream iss(command);
    std::string cmd;
    iss >> cmd;

    // Convert command to uppercase for case-insensitive comparison
    for (char& c : cmd) {
        c = std::toupper(c);
    }

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
    }

    return "-ERR unknown command '" + cmd + "'\r\n";
}

}  // namespace network
}  // namespace redis_clone