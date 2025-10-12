#include "network/server.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>

#include <cerrno>
#include <csignal>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <stdexcept>

#include "network/redis_utils.h"

extern volatile sig_atomic_t g_running;

namespace redis_clone {
namespace network {

// Global pointer for signal handler access
static RedisServer* g_server_instance = nullptr;

}  // namespace network
}  // namespace redis_clone

// SIGCHLD handler to clean up background save processes
void handle_child_exit(int sig) {
    int status;
    pid_t pid;

    // Reap all finished child processes (non-blocking)
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        if (WIFEXITED(status)) {
            std::cout << "Background operation completed (PID: " << pid
                      << ", exit code: " << WEXITSTATUS(status) << ")" << std::endl;

            // Check if this was an AOF rewrite process
            if (redis_clone::network::g_server_instance) {
                redis_clone::network::g_server_instance->handle_aof_rewrite_completion(pid);
            }
        } else {
            std::cout << "Background operation failed (PID: " << pid << ")" << std::endl;
        }
    }
}

// Helper function to check if file exists
bool file_exists(const std::string& filename) {
    std::ifstream file(filename);
    return file.good();
}

namespace redis_clone {
namespace network {

RedisServer::RedisServer(int port) : server_fd_(-1) {
    server_start_time_ = std::chrono::steady_clock::now();
    last_save_time_ = server_start_time_;

    // Set global instance for signal handler
    g_server_instance = this;

    // Redis-style recovery: AOF takes precedence over RDB
    if (aof_enabled && file_exists("data/appendonly.aof")) {
        std::cout << "Loading data from AOF file ..." << std::endl;
        load_aof_from_file();
    } else if (file_exists("data/dump.json")) {
        std::cout << "Loading data from snapshot ..." << std::endl;
        load_snapshot_from_file();
    } else {
        std::cout << "No persistence files found, starting with empty database" << std::endl;
    }

    signal(SIGCHLD, handle_child_exit);

    // Initialize AOF
    last_fsync_time_ = server_start_time_;
    if (aof_enabled) {
        aof_file_.open("data/appendonly.aof", std::ios::app);
        if (!aof_file_.is_open()) {
            std::cerr << "Warning: Could not open AOF file for writing" << std::endl;
            aof_enabled = false;
        } else {
            std::cout << "AOF logging enabled" << std::endl;
        }
    }

    // Create and configure server socket
    server_fd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd_ < 0) {
        throw std::runtime_error("Failed to create socket");
    }

    int opt = 1;
    setsockopt(server_fd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

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

    std::cout << "Redis server listening on port " << port << std::endl;
}

void RedisServer::accept_new_connections() {
    int client_fd = accept(server_fd_, nullptr, nullptr);
    if (client_fd < 0) {
        std::cerr << "Failed to accept connection" << std::endl;
        return;
    }

    ClientState new_client;
    new_client.fd = client_fd;
    clients_[client_fd] = new_client;
}

void RedisServer::handle_client_data(int client_fd) {
    char buffer[1024];
    ssize_t bytes_read = recv(client_fd, buffer, sizeof(buffer) - 1, MSG_DONTWAIT);

    if (bytes_read <= 0) {
        std::cout << "Client " << client_fd << " disconnected" << std::endl;
        clients_[client_fd].should_disconnect = true;
        return;
    }

    buffer[bytes_read] = '\0';
    clients_[client_fd].read_buffer += buffer;

    std::cout << "Received " << bytes_read << " bytes from client " << client_fd << std::endl;

    // Extract and process complete commands (delimited by \n)
    size_t pos;
    while ((pos = clients_[client_fd].read_buffer.find('\n')) != std::string::npos) {
        std::string complete_command = clients_[client_fd].read_buffer.substr(0, pos);
        clients_[client_fd].read_buffer.erase(0, pos + 1);

        // Strip carriage return if present
        if (!complete_command.empty() && complete_command.back() == '\r') {
            complete_command.pop_back();
        }

        if (!complete_command.empty()) {
            std::cout << "Processing command: '" << complete_command << "'" << std::endl;

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

std::string RedisServer::process_command(const std::string& command) {
    redis_utils::CommandParts parts = redis_utils::extract_command(command);
    std::string response = redis_utils::process_command_with_store(parts, data_);

    if (parts.command == "BGSAVE") {
        return background_save();
    }

    if (parts.command == "BGREWRITEAOF") {
        return background_rewrite_aof();
    }

    // Count successful write operations for persistence triggers
    if ((response[0] == '+' && response.substr(0, 3) == "+OK") ||
        (response[0] == ':' && response[1] == '1')) {
        if (parts.command == "SET" || parts.command == "DEL") {
            // Write to AOF first (write-ahead logging)
            append_to_aof(command);
            changes_since_save++;
        }
    }

    return response;
}

void RedisServer::run() {
    while (g_running) {
        fd_set read_fds;
        FD_ZERO(&read_fds);

        FD_SET(server_fd_, &read_fds);
        int max_fd = server_fd_;

        for (const auto& [client_fd, client_state] : clients_) {
            FD_SET(client_fd, &read_fds);
            max_fd = std::max(max_fd, client_fd);
        }

        int activity = select(max_fd + 1, &read_fds, nullptr, nullptr, nullptr);

        if (activity < 0) {
            if (errno == EINTR) {
                continue;  // Signal interrupted, check g_running flag
            }
            std::cerr << "select() error: " << strerror(errno) << std::endl;
            break;
        }

        if (FD_ISSET(server_fd_, &read_fds)) {
            accept_new_connections();
        }

        for (const auto& [client_fd, client_state] : clients_) {
            if (FD_ISSET(client_fd, &read_fds)) {
                handle_client_data(client_fd);
            }
        }

        // Send pending responses and handle disconnections
        std::vector<int> clients_to_disconnect;
        for (auto& [client_fd, client_state] : clients_) {
            if (!client_state.write_buffer.empty()) {
                ssize_t bytes_sent = send(client_fd, client_state.write_buffer.c_str(),
                                          client_state.write_buffer.size(), MSG_DONTWAIT);
                if (bytes_sent > 0) {
                    client_state.write_buffer.erase(0, bytes_sent);
                }
            }

            if (client_state.should_disconnect && client_state.write_buffer.empty()) {
                clients_to_disconnect.push_back(client_fd);
            }
        }

        for (int client_fd : clients_to_disconnect) {
            close(client_fd);
            clients_.erase(client_fd);
        }

        // Check if automatic save conditions are met
        if (should_save_snapshot()) {
            background_save_internal();
            changes_since_save = 0;
            last_save_time_ = std::chrono::steady_clock::now();
        }

        // Check if AOF needs fsync (for EVERYSEC policy)
        fsync_aof_if_needed();
    }

    // Cleanup on shutdown
    for (auto& [client_fd, client_state] : clients_) {
        close(client_fd);
    }
    close(server_fd_);
}

bool RedisServer::should_save_snapshot() {
    auto now = std::chrono::steady_clock::now();
    auto seconds_since_last_save =
        std::chrono::duration_cast<std::chrono::seconds>(now - last_save_time_).count();

    // Redis-style save conditions: "save <seconds> <changes>"
    if (seconds_since_last_save >= 900 && changes_since_save > 1) return true;  // 15 min + 1 change
    if (seconds_since_last_save >= 300 && changes_since_save >= 10)
        return true;  // 5 min + 10 changes
    if (seconds_since_last_save >= 60 && changes_since_save >= 10000)
        return true;  // 1 min + 10k changes

    return false;
}

void RedisServer::save_snapshot_to_file() {
    const std::string temp_file = "data/dump.json.tmp";
    const std::string final_file = "data/dump.json";

    std::ofstream file(temp_file);
    if (!file.is_open()) {
        std::cerr << "Error: Could not open " << temp_file << " for writing" << std::endl;
        return;
    }

    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto tm = *std::gmtime(&time_t);

    // Write JSON snapshot with metadata
    file << "{\n";
    file << "  \"metadata\": {\n";
    file << "    \"version\": \"1.0\",\n";
    file << "    \"timestamp\": \"";
    file << std::put_time(&tm, "%Y-%m-%dT%H:%M:%SZ");
    file << "\",\n";
    file << "    \"key_count\": " << data_.size() << "\n";
    file << "  },\n";
    file << "  \"data\": {\n";

    bool first = true;
    for (const auto& [key, value] : data_) {
        if (!first) file << ",\n";
        file << "    \"" << key << "\": \"" << value << "\"";
        first = false;
    }

    file << "\n  }\n}\n";
    file.flush();
    file.close();

    // Atomic rename ensures data integrity (even if interrupted)
    if (std::rename(temp_file.c_str(), final_file.c_str()) != 0) {
        std::cerr << "Error: Failed to rename " << temp_file << " to " << final_file << std::endl;
        std::remove(temp_file.c_str());
        return;
    }

    std::cout << "Snapshot saved: " << data_.size() << " keys written to " << final_file
              << std::endl;
}

void RedisServer::load_snapshot_from_file() {
    std::ifstream file("data/dump.json");
    if (!file.is_open()) {
        std::cout << "No existing snapshot found, starting with empty database" << std::endl;
        return;
    }

    std::string line;
    bool in_data_section = false;
    int loaded_count = 0;

    while (std::getline(file, line)) {
        if (line.find("\"data\":") != std::string::npos) {
            in_data_section = true;
            continue;
        }

        if (!in_data_section) continue;

        // End of data section
        if (line.find("}") != std::string::npos && line.find("\"") == std::string::npos) {
            break;
        }

        // Simple JSON parsing for "key": "value" lines
        size_t key_start = line.find("\"");
        if (key_start == std::string::npos) continue;

        size_t key_end = line.find("\"", key_start + 1);
        if (key_end == std::string::npos) continue;

        size_t value_start = line.find("\"", key_end + 1);
        if (value_start == std::string::npos) continue;

        size_t value_end = line.find("\"", value_start + 1);
        if (value_end == std::string::npos) continue;

        std::string key = line.substr(key_start + 1, key_end - key_start - 1);
        std::string value = line.substr(value_start + 1, value_end - value_start - 1);

        data_[key] = value;
        loaded_count++;
    }

    file.close();
    std::cout << "Loaded " << loaded_count << " keys from snapshot" << std::endl;
}

std::string RedisServer::background_save() {
    pid_t pid = fork();

    if (pid == 0) {
        // Child process: save and exit
        save_snapshot_to_file();
        exit(0);
    } else if (pid > 0) {
        // Parent process: continue serving
        std::cout << "Background save started (PID: " << pid << ")" << std::endl;
        return "+Background saving started\r\n";
    } else {
        return "-ERR Background save failed\r\n";
    }
}

void RedisServer::background_save_internal() {
    pid_t pid = fork();

    if (pid == 0) {
        // Child process: save and exit
        save_snapshot_to_file();
        exit(0);
    } else if (pid > 0) {
        // Parent process: log and continue
        std::cout << "Automatic background save started (PID: " << pid << ")" << std::endl;
    } else {
        // Fork failed: log error but don't crash server
        std::cerr << "Failed to fork for automatic save: " << strerror(errno) << std::endl;
    }
}

void RedisServer::append_to_aof(const std::string& command) {
    if (!aof_enabled || !aof_file_.is_open()) {
        return;
    }

    // Write command to AOF file with newline
    aof_file_ << command << std::endl;

    // Handle fsync policy
    if (fsync_policy_ == FsyncPolicy::ALWAYS) {
        aof_file_.flush();  // Force write to OS buffer
        // Note: For cross-platform fsync, we'll handle this in fsync_aof_if_needed()
    }

    // Check if AOF needs auto-rewriting (every 100 commands to avoid excessive checking)
    static int command_count = 0;
    if (++command_count % 100 == 0 && should_auto_rewrite_aof()) {
        std::cout << "AOF file grew too large, triggering background rewrite..." << std::endl;
        background_rewrite_aof();
        // Update the baseline size after rewrite is started
        aof_last_rewrite_size_ = get_aof_file_size();
    }
}

void RedisServer::fsync_aof_if_needed() {
    if (!aof_enabled || !aof_file_.is_open()) {
        return;
    }

    auto now = std::chrono::steady_clock::now();
    auto seconds_since_fsync =
        std::chrono::duration_cast<std::chrono::seconds>(now - last_fsync_time_).count();

    if (fsync_policy_ == FsyncPolicy::EVERYSEC && seconds_since_fsync >= 1) {
        aof_file_.flush();  // Force to OS buffer
        last_fsync_time_ = now;
        std::cout << "AOF fsync performed" << std::endl;
    }
    // FsyncPolicy::NO means we never explicitly fsync - let OS decide
}

void RedisServer::load_aof_from_file() {
    std::ifstream aof_file("data/appendonly.aof");
    if (!aof_file.is_open()) {
        std::cout << "No existing AOF file found" << std::endl;
        return;
    }

    std::string command_line;
    int commands_replayed = 0;

    std::cout << "Loading AOF file ..." << std::endl;

    while (std::getline(aof_file, command_line)) {
        if (!command_line.empty()) {
            // Execute the command to rebuild database state
            redis_utils::CommandParts parts = redis_utils::extract_command(command_line);
            redis_utils::process_command_with_store(parts, data_);
            commands_replayed++;
        }
    }

    aof_file.close();
    std::cout << "AOF recovery complete: " << commands_replayed << " commands replayed"
              << std::endl;
}

std::string RedisServer::background_rewrite_aof() {
    pid_t pid = fork();

    if (pid == 0) {
        // Child process: rewrite AOF and exit
        rewrite_aof_internal();
        exit(0);
    } else if (pid > 0) {
        // Parent process: continue serving
        aof_rewrite_pid_ = pid;  // Store PID for tracking
        std::cout << "Background AOF rewrite started (PID: " << pid << ")" << std::endl;
        return "+Background AOF rewrite started\r\n";
    } else {
        return "-ERR Background AOF rewrite failed\r\n";
    }
}

void RedisServer::rewrite_aof_internal() {
    const std::string temp_aof = "data/appendonly.aof.tmp";
    const std::string final_aof = "data/appendonly.aof";

    std::ofstream new_aof(temp_aof);
    if (!new_aof.is_open()) {
        std::cerr << "Error: Could not open " << temp_aof << " for writing" << std::endl;
        return;
    }

    // Generate minimal command set from current database state
    for (const auto& [key, value] : data_) {
        new_aof << "SET " << key << " " << value << std::endl;
    }

    new_aof.flush();
    new_aof.close();

    // Atomic replace of old AOF with new compact AOF
    if (std::rename(temp_aof.c_str(), final_aof.c_str()) != 0) {
        std::cerr << "Error: Failed to rename " << temp_aof << " to " << final_aof << std::endl;
        std::remove(temp_aof.c_str());
        return;
    }

    std::cout << "AOF rewrite completed: " << data_.size() << " keys written to new AOF"
              << std::endl;
}

// Get current AOF file size
size_t RedisServer::get_aof_file_size() {
    std::ifstream file("data/appendonly.aof", std::ios::ate | std::ios::binary);
    return file.tellg();
}

// Check if AOF should be rewritten based on size thresholds
bool RedisServer::should_auto_rewrite_aof() {
    if (!aof_enabled) return false;

    size_t current_size = get_aof_file_size();

    // Don't rewrite if file is smaller than minimum size
    if (current_size < aof_auto_rewrite_min_size_) {
        return false;
    }

    // Calculate size increase percentage
    if (aof_last_rewrite_size_ == 0) {
        // First time - set baseline
        aof_last_rewrite_size_ = current_size;
        return false;
    }

    size_t size_increase = ((current_size - aof_last_rewrite_size_) * 100) / aof_last_rewrite_size_;
    return size_increase >= aof_auto_rewrite_percentage_;
}

// Handle completion of AOF rewrite process
void RedisServer::handle_aof_rewrite_completion(pid_t pid) {
    if (aof_rewrite_pid_ == pid) {
        reopen_aof_after_rewrite();
        aof_rewrite_pid_ = -1;  // Reset
    }
}

// Reopen AOF file after background rewrite
void RedisServer::reopen_aof_after_rewrite() {
    if (!aof_enabled) return;

    // Close old file handle
    if (aof_file_.is_open()) {
        aof_file_.close();
    }

    // Reopen AOF file (now points to rewritten file)
    aof_file_.open("data/appendonly.aof", std::ios::app);
    if (!aof_file_.is_open()) {
        std::cerr << "Error: Could not reopen AOF file after rewrite" << std::endl;
        aof_enabled = false;  // Disable AOF if we can't reopen
    } else {
        std::cout << "AOF file reopened after rewrite" << std::endl;
        // Update baseline size after successful reopen
        aof_last_rewrite_size_ = get_aof_file_size();
    }
}

}  // namespace network
}  // namespace redis_clone