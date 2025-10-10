#include "network/threaded_server.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <pthread.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cerrno>
#include <csignal>
#include <cstring>
#include <iostream>
#include <mutex>
#include <sstream>
#include <stdexcept>
#include <thread>

#include "network/redis_utils.h"

extern volatile sig_atomic_t g_running;

namespace redis_clone {
namespace network {

ThreadedRedisServer::ThreadedRedisServer(int port) : port_(port), server_fd_(-1) {
    initialize_server();
}

void ThreadedRedisServer::initialize_server() {
    server_fd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd_ < 0) {
        throw std::runtime_error("Failed to create socket: " + std::string(strerror(errno)));
    }

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

void ThreadedRedisServer::run() {
    while (g_running) {
        int client_fd = accept(server_fd_, nullptr, nullptr);
        if (client_fd < 0) {
            if (errno == EINTR) {
                if (g_running)
                    continue;
                else
                    break;
            }
            throw std::runtime_error("Failed to accept connection: " +
                                     std::string(strerror(errno)));
        }

        try {
            // Spawn new thread for each client
            std::thread client_thread(&ThreadedRedisServer::handle_client, this, client_fd);
            client_thread.detach();
        } catch (const std::exception& e) {
            std::cerr << "Error handling client: " << e.what() << std::endl;
            close(client_fd);
        }
    }

    std::cout << "Threaded server shutting down..." << std::endl;
    close(server_fd_);
}

void ThreadedRedisServer::handle_client(int client_fd) {
    // Block signals for worker threads (main thread handles them)
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
            return;
        }

        command_buffer.append(buffer, bytes_read);

        size_t pos = command_buffer.find("\n");
        if (pos != std::string::npos) {
            std::string complete_command = command_buffer.substr(0, pos);
            command_buffer.erase(0, pos + 1);

            if (!complete_command.empty() && complete_command.back() == '\r') {
                complete_command.pop_back();
            }

            if (!complete_command.empty()) {
                redis_utils::CommandParts parts = redis_utils::extract_command(complete_command);
                if (parts.command == "QUIT") {
                    send_command(client_fd, "+OK\r\n");
                    close(client_fd);
                    return;
                }

                std::string response = process_command(complete_command);
                send_command(client_fd, response);
            }
        }
    }
}

std::string ThreadedRedisServer::process_command(const std::string& command) {
    redis_utils::CommandParts parts = redis_utils::extract_command(command);

    // Thread-safe access to database layer
    std::lock_guard<std::mutex> lock(db_mutex_);

    if (parts.command == "SET") {
        if (parts.key.empty() || parts.value.empty()) {
            return "-ERR wrong number of arguments for 'set' command\r\n";
        }
        db_.set(parts.key, parts.value);
        return "+OK\r\n";
    } else if (parts.command == "GET") {
        if (parts.key.empty()) {
            return "-ERR wrong number of arguments for 'get' command\r\n";
        }
        auto result = db_.get(parts.key);
        if (result.has_value()) {
            return "$" + std::to_string(result->size()) + "\r\n" + *result + "\r\n";
        }
        return "$-1\r\n";  // Redis null bulk string
    } else if (parts.command == "DEL") {
        if (parts.key.empty()) {
            return "-ERR wrong number of arguments for 'del' command\r\n";
        }
        return ":" + std::to_string(db_.del(parts.key)) + "\r\n";
    } else if (parts.command == "EXISTS") {
        if (parts.key.empty()) {
            return "-ERR wrong number of arguments for 'exists' command\r\n";
        }
        return ":" + std::to_string(db_.exists(parts.key)) + "\r\n";
    } else if (parts.command == "QUIT") {
        return "+OK\r\n";
    }

    return "-ERR unknown command '" + parts.command + "'\r\n";
}

void ThreadedRedisServer::send_command(int client_fd, const std::string& response) {
    if (send(client_fd, response.c_str(), response.size(), 0) < 0) {
        std::cerr << "Failed to send response to client: " << std::string(strerror(errno))
                  << std::endl;
    }
}

}  // namespace network
}  // namespace redis_clone