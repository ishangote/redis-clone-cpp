#pragma once

#include <mutex>
#include <string>
#include <unordered_map>

#include "network/redis_utils.h"
#include "storage/database.h"

namespace redis_clone {
namespace network {

class RedisServer {
   public:
    /**
     * @brief Constructs a Redis server instance
     * @param port The port number to listen on
     * @throws std::runtime_error if server initialization fails
     */
    explicit RedisServer(int port);

    /**
     * @brief Starts the server's main loop
     * @throws std::runtime_error if server fails to start or encounters critical error
     */
    void run();

    // Prevent copying and assignment
    RedisServer(const RedisServer&) = delete;
    RedisServer& operator=(const RedisServer&) = delete;

   private:
    int port_;
    int server_fd_;  // Server socket file descriptor
    redis_clone::storage::Database db_;
    std::mutex db_mutex_;

    /**
     * @brief Handles individual client connections
     * @param client_fd Client socket file descriptor
     */
    void handle_client(int client_fd);

    /**
     * @brief Processes Redis commands
     * @param command The command string to process
     * @return Response string in RESP format
     */
    std::string process_command(const std::string& command);

    /**
     * @brief Initializes the server socket
     * @throws std::runtime_error if initialization fails
     */
    void initialize_server();

    /**
     * @brief Sends response to client
     * @param client_fd Client socket file descriptor
     * @param response Response string to send
     */
    void send_command(int client_fd, const std::string& response);
};

class RedisServerEventLoop {
   private:
    int server_fd_;
    std::unordered_map<std::string, std::string> data_;
    int changes_since_save = 0;
    std::chrono::steady_clock::time_point last_save_time_;
    std::chrono::steady_clock::time_point server_start_time_;

    struct ClientState {
        int fd;
        std::string read_buffer;         // Accumulate incoming data
        std::string write_buffer;        // Queue outgoing responses
        bool should_disconnect = false;  // Flag for graceful disconnection
    };

    std::unordered_map<int, ClientState> clients_;  // fd -> client state

   public:
    RedisServerEventLoop(int port);
    void run();  // Main event loop

   private:
    void accept_new_connections();
    void handle_client_data(int client_fd);
    void process_complete_commands(int client_fd);
    void send_pending_response();
    std::string process_command(const std::string& command);
    bool should_save_snapshot();      // Check Redis-style save conditions
    void save_snapshot_to_file();     // Save data_ to JSON file
    std::string background_save();    // Background save for BGSAVE command
    void background_save_internal();  // Background save for automatic triggers
    void load_snapshot_from_file();   // Load JSON file into data_
};

}  // namespace network
}  // namespace redis_clone