#pragma once

#include <string>

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
};

}  // namespace network
}  // namespace redis_clone