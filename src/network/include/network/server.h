#pragma once

#include <chrono>
#include <fstream>
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

    // Public methods for signal handler access
    void handle_aof_rewrite_completion(pid_t pid);

   private:
    int server_fd_;
    std::unordered_map<std::string, std::string> data_;

    // Persistence tracking
    int changes_since_save = 0;
    std::chrono::steady_clock::time_point last_save_time_;
    std::chrono::steady_clock::time_point server_start_time_;

    // AOF persistence
    bool aof_enabled = true;
    std::ofstream aof_file_;
    enum class FsyncPolicy { ALWAYS, EVERYSEC, NO } fsync_policy_ = FsyncPolicy::EVERYSEC;
    std::chrono::steady_clock::time_point last_fsync_time_;

    // AOF auto-rewrite configuration
    size_t aof_last_rewrite_size_ = 0;                     // Size of AOF after last rewrite
    size_t aof_auto_rewrite_percentage_ = 100;             // Rewrite when AOF is 2x size
    size_t aof_auto_rewrite_min_size_ = 64 * 1024 * 1024;  // 64MB minimum

    // Background process tracking
    pid_t aof_rewrite_pid_ = -1;  // Track AOF rewrite process PID

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

    // AOF persistence operations
    void append_to_aof(const std::string& command);
    void fsync_aof_if_needed();
    void load_aof_from_file();
    void rewrite_aof_internal();
    std::string background_rewrite_aof();  // For BGREWRITEAOF command

    // AOF auto-rewrite helpers
    size_t get_aof_file_size();
    bool should_auto_rewrite_aof();
    void reopen_aof_after_rewrite();
};

}  // namespace network
}  // namespace redis_clone