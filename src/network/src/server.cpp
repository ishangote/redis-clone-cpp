#include "network/server.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <pthread.h>
#include <sys/select.h>
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

#include "network/redis_utils.h"

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

/**
 * Handle individual client connection in separate thread
 * Blocks signals and processes commands until client disconnects
 */
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

        // Parse complete command (handle both \r\n and \n)
        size_t pos = command_buffer.find("\n");
        if (pos != std::string::npos) {
            std::string complete_command = command_buffer.substr(0, pos);
            command_buffer.erase(0, pos + 1);

            // Remove carriage return if present
            if (!complete_command.empty() && complete_command.back() == '\r') {
                complete_command.pop_back();
            }

            if (!complete_command.empty()) {
                // Process QUIT command
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
    close(client_fd);
}

/**
 * Process Redis command with thread-safe database access
 */
std::string RedisServer::process_command(const std::string& command) {
    redis_utils::CommandParts parts = redis_utils::extract_command(command);

    std::lock_guard<std::mutex> lock(db_mutex_);

    // Process commands using the Database API
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

void RedisServer::send_command(int client_fd, const std::string& response) {
    if (send(client_fd, response.c_str(), response.size(), 0) < 0) {
        std::cerr << "Failed to send response to client: " << std::string(strerror(errno))
                  << std::endl;
    }
}

RedisServerEventLoop::RedisServerEventLoop(int port) : server_fd_(-1) {
    // Create server socket
    server_fd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd_ < 0) {
        throw std::runtime_error("Failed to create socket");
    }

    // Allow port reuse
    int opt = 1;
    setsockopt(server_fd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    // Bind to port
    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    if (bind(server_fd_, reinterpret_cast<struct sockaddr*>(&address), sizeof(address)) < 0) {
        close(server_fd_);
        throw std::runtime_error("Failed to bind to port " + std::to_string(port));
    }

    if (listen(server_fd_, SOMAXCONN) < 0) {
        close(server_fd_);
        throw std::runtime_error("Failed to listen");
    }

    std::cout << "Event loop server listening on port " << port << std::endl;
}

/**
 * Accept new client connection and add to client map
 */
void RedisServerEventLoop::accept_new_connections() {
    int client_fd = accept(server_fd_, nullptr, nullptr);
    if (client_fd < 0) {
        std::cerr << "Failed to accept connection" << std::endl;
        return;
    }
    ClientState new_client;
    new_client.fd = client_fd;
    new_client.read_buffer = "";
    new_client.write_buffer = "";

    clients_[client_fd] = new_client;
}

/**
 * Process incoming data from client, handle command parsing and buffering
 */
void RedisServerEventLoop::handle_client_data(int client_fd) {
    char buffer[1024];
    ssize_t bytes_read = recv(client_fd, buffer, sizeof(buffer) - 1, MSG_DONTWAIT);

    if (bytes_read <= 0) {
        std::cout << "Client " << client_fd << " disconnected" << std::endl;
        clients_[client_fd].should_disconnect = true;
        return;
    }

    // Block 2: Add data to client's read buffer
    buffer[bytes_read] = '\0';  // Null-terminate the string
    clients_[client_fd].read_buffer += buffer;

    std::cout << "Received " << bytes_read << " bytes from client " << client_fd << ": '" << buffer
              << "'" << std::endl;
    std::cout << "Client " << client_fd << " buffer now: '" << clients_[client_fd].read_buffer
              << "'" << std::endl;

    // Block 3: Process complete commands
    size_t pos;
    while ((pos = clients_[client_fd].read_buffer.find('\n')) != std::string::npos) {
        std::string complete_command = clients_[client_fd].read_buffer.substr(0, pos);

        clients_[client_fd].read_buffer.erase(0, pos + 1);

        if (!complete_command.empty() && complete_command.back() == '\r') {
            complete_command.pop_back();
        }

        if (!complete_command.empty()) {
            std::cout << "Processing command: '" << complete_command << "'" << std::endl;

            // Handle QUIT command
            redis_utils::CommandParts parts = redis_utils::extract_command(complete_command);
            if (parts.command == "QUIT") {
                clients_[client_fd].write_buffer += "+OK\r\n";
                clients_[client_fd].should_disconnect = true;
            } else {
                std::string response = process_command(complete_command);
                clients_[client_fd].write_buffer += response;
            }
        }
    }
}

std::string RedisServerEventLoop::process_command(const std::string& command) {
    redis_utils::CommandParts parts = redis_utils::extract_command(command);
    return redis_utils::process_command_with_store(parts, data_);
}

/**
 * Main event loop using select() for I/O multiplexing
 * Handles multiple clients without blocking on any single connection
 */
void RedisServerEventLoop::run() {
    while (g_running) {
        fd_set read_fds;
        FD_ZERO(&read_fds);

        // Monitor server socket for new connections
        FD_SET(server_fd_, &read_fds);
        int max_fd = server_fd_;

        // Monitor all client sockets for data
        for (const auto& [client_fd, client_state] : clients_) {
            FD_SET(client_fd, &read_fds);
            max_fd = std::max(max_fd, client_fd);
        }

        // Wait for socket activity
        int activity = select(max_fd + 1, &read_fds, nullptr, nullptr, nullptr);

        if (activity < 0) {
            if (errno == EINTR) {
                continue;  // Signal interrupted, check running flag
            }
            std::cerr << "select() error: " << strerror(errno) << std::endl;
            break;
        }

        // Accept new connections
        if (FD_ISSET(server_fd_, &read_fds)) {
            accept_new_connections();
        }

        // Process client data
        for (const auto& [client_fd, client_state] : clients_) {
            if (FD_ISSET(client_fd, &read_fds)) {
                handle_client_data(client_fd);
            }
        }

        // Send responses and handle disconnections
        std::vector<int> clients_to_disconnect;
        for (auto& [client_fd, client_state] : clients_) {
            // Send pending data
            if (!client_state.write_buffer.empty()) {
                ssize_t bytes_sent = send(client_fd, client_state.write_buffer.c_str(),
                                          client_state.write_buffer.size(), MSG_DONTWAIT);
                if (bytes_sent > 0) {
                    client_state.write_buffer.erase(0, bytes_sent);
                }
            }

            // Queue disconnections
            if (client_state.should_disconnect && client_state.write_buffer.empty()) {
                clients_to_disconnect.push_back(client_fd);
            }
        }

        // Clean up disconnected clients
        for (int client_fd : clients_to_disconnect) {
            close(client_fd);
            clients_.erase(client_fd);
        }
    }

    // Cleanup on shutdown
    for (auto& [client_fd, client_state] : clients_) {
        close(client_fd);
    }
    close(server_fd_);
}

}  // namespace network
}  // namespace redis_clone