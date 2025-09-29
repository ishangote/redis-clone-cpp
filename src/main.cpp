#include <unistd.h>

#include <csignal>
#include <cstdlib>
#include <iostream>
#include <string>

#include "network/server.h"

// Global variable for signal handling
volatile sig_atomic_t g_running = 1;

namespace {

void signal_handler(int signal) {
    if (signal == SIGINT || signal == SIGTERM) {
        std::cout << "\nShutting down gracefully..." << std::endl;
        g_running = 0;
    }
}

void setup_signal_handlers() {
    struct sigaction sa;
    sa.sa_handler = signal_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    if (sigaction(SIGINT, &sa, nullptr) == -1) {
        throw std::runtime_error("Failed to set up SIGINT handler");
    }
    if (sigaction(SIGTERM, &sa, nullptr) == -1) {
        throw std::runtime_error("Failed to set up SIGTERM handler");
    }
}

int get_port_from_env() {
    if (const char* env_port = std::getenv("REDIS_CLONE_PORT")) {
        try {
            return std::stoi(env_port);
        } catch (const std::exception&) {
            // Fall back to default if invalid
        }
    }
    return 6379;  // Default Redis port
}
}  // namespace

int main(int argc, char* argv[]) {
    try {
        setup_signal_handlers();

        // Parse port from command line or environment
        int port = get_port_from_env();
        if (argc > 1) {
            try {
                port = std::stoi(argv[1]);
                if (port <= 0 || port > 65535) {
                    throw std::out_of_range("Port number out of range");
                }
            } catch (const std::exception& e) {
                std::cerr << "Invalid port number: " << argv[1] << "\n"
                          << "Usage: " << argv[0] << " [port]\n"
                          << "Using default port: " << port << "\n\n";
            }
        }

        std::cout << "Redis Clone Server v0.1.0\n"
                  << "Port: " << port << "\n"
                  << "PID: " << getpid() << "\n"
                  << "--------------------------------\n";

        redis_clone::network::RedisServer server(port);
        std::cout << "Server is ready to accept connections\n";

        server.run();
        return 0;

    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << "\n";
        return 1;
    } catch (...) {
        std::cerr << "Unknown fatal error occurred\n";
        return 2;
    }
}
