#pragma once

#include <chrono>
#include <string>
#include <unordered_map>

namespace redis_clone {
namespace network {

/**
 * Event-driven Redis server with persistence support
 *
 * Uses select() for I/O multiplexing and fork() for background saves.
 * This is the main Redis-like implementation for distributed systems learning.
 */
class RedisServer {
   public:
    explicit RedisServer(int port);
    void run();

   private:
    int server_fd_;
    std::unordered_map<std::string, std::string> data_;

    // Persistence tracking
    int changes_since_save = 0;
    std::chrono::steady_clock::time_point last_save_time_;
    std::chrono::steady_clock::time_point server_start_time_;

    struct ClientState {
        int fd;
        std::string read_buffer;   // Accumulated incomplete commands
        std::string write_buffer;  // Queued responses
        bool should_disconnect = false;
    };

    std::unordered_map<int, ClientState> clients_;

    // Network operations
    void accept_new_connections();
    void handle_client_data(int client_fd);
    std::string process_command(const std::string& command);

    // Persistence operations
    bool should_save_snapshot();
    void save_snapshot_to_file();
    std::string background_save();    // For BGSAVE command
    void background_save_internal();  // For automatic saves
    void load_snapshot_from_file();
};

}  // namespace network
}  // namespace redis_clone