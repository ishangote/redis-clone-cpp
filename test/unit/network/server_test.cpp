#include "network/server.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <csignal>

// Define the global running flag for tests
volatile sig_atomic_t g_running = 1;

#include <thread>

#include "gtest/gtest.h"

namespace {

class RedisServerTest : public ::testing::Test {
   protected:
    void SetUp() override {
        // Start server in a separate thread
        server_thread_ = std::thread([this]() { server_.run(); });
    }

    void TearDown() override {
        // Cleanup and stop server
        if (server_thread_.joinable()) {
            // Send shutdown signal
            g_running = 0;
            // Connect briefly to unblock accept()
            int sock = socket(AF_INET, SOCK_STREAM, 0);
            if (sock >= 0) {
                struct sockaddr_in addr{};
                addr.sin_family = AF_INET;
                addr.sin_port = htons(test_port_);
                addr.sin_addr.s_addr = inet_addr("127.0.0.1");
                connect(sock, (struct sockaddr*)&addr, sizeof(addr));
                close(sock);
            }
            server_thread_.join();
        }
        g_running = 1;  // Reset for next test
    }

    // Helper function to send command and get response
    std::string send_command(const std::string& command) {
        int sock = socket(AF_INET, SOCK_STREAM, 0);
        EXPECT_GE(sock, 0) << "Failed to create socket";

        struct sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_port = htons(test_port_);
        addr.sin_addr.s_addr = inet_addr("127.0.0.1");

        EXPECT_EQ(connect(sock, (struct sockaddr*)&addr, sizeof(addr)), 0) << "Failed to connect";

        send(sock, command.c_str(), command.length(), 0);

        char buffer[1024] = {0};
        ssize_t bytes_read = recv(sock, buffer, sizeof(buffer) - 1, 0);
        EXPECT_GT(bytes_read, 0) << "Failed to receive response";

        close(sock);
        return std::string(buffer, bytes_read);
    }

    const int test_port_ = 6380;  // Use different port than main server
    redis_clone::network::RedisServer server_{test_port_};
    std::thread server_thread_;
};

TEST_F(RedisServerTest, SetAndGetCommand) {
    // Set a value
    auto set_response = send_command("SET test_key test_value");
    EXPECT_EQ(set_response, "+OK\r\n");

    // Get the value back
    auto get_response = send_command("GET test_key");
    EXPECT_EQ(get_response, "$10\r\ntest_value\r\n");
}

TEST_F(RedisServerTest, GetNonexistentKey) {
    auto response = send_command("GET nonexistent");
    EXPECT_EQ(response, "$-1\r\n");
}

TEST_F(RedisServerTest, DelCommand) {
    // Set and verify a value
    send_command("SET del_key del_value");
    auto get_response = send_command("GET del_key");
    EXPECT_EQ(get_response, "$9\r\ndel_value\r\n");

    // Delete the key
    auto del_response = send_command("DEL del_key");
    EXPECT_EQ(del_response, ":1\r\n");

    // Verify key is gone
    auto get_after_del = send_command("GET del_key");
    EXPECT_EQ(get_after_del, "$-1\r\n");
}

TEST_F(RedisServerTest, ExistsCommand) {
    // Check non-existent key
    auto not_exists = send_command("EXISTS missing_key");
    EXPECT_EQ(not_exists, ":0\r\n");

    // Set and check existing key
    send_command("SET exists_key value");
    auto exists = send_command("EXISTS exists_key");
    EXPECT_EQ(exists, ":1\r\n");
}

TEST_F(RedisServerTest, InvalidCommand) {
    auto response = send_command("INVALID_CMD");
    EXPECT_EQ(response, "-ERR unknown command 'INVALID_CMD'\r\n");
}

TEST_F(RedisServerTest, SetWithMissingValue) {
    auto response = send_command("SET key");
    EXPECT_EQ(response, "-ERR wrong number of arguments for 'set' command\r\n");
}

TEST_F(RedisServerTest, GetWithMissingKey) {
    auto response = send_command("GET");
    EXPECT_EQ(response, "-ERR wrong number of arguments for 'get' command\r\n");
}

}  // namespace