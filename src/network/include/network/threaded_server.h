#pragma once

#include <mutex>
#include <string>

#include "storage/database.h"

namespace redis_clone {
namespace network {

/**
 * Multi-threaded Redis server implementation
 *
 * Alternative implementation using one thread per client and the storage layer.
 * Demonstrates different concurrency patterns but lacks persistence features.
 */
class ThreadedRedisServer {
   public:
    explicit ThreadedRedisServer(int port);
    void run();

    ThreadedRedisServer(const ThreadedRedisServer&) = delete;
    ThreadedRedisServer& operator=(const ThreadedRedisServer&) = delete;

   private:
    int port_;
    int server_fd_;
    redis_clone::storage::Database db_;
    std::mutex db_mutex_;  // Protects database access across threads

    void handle_client(int client_fd);
    std::string process_command(const std::string& command);
    void initialize_server();
    void send_command(int client_fd, const std::string& response);
};

}  // namespace network
}  // namespace redis_clone