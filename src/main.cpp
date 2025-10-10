#include <unistd.h>

#include <csignal>
#include <cstdlib>
#include <iostream>
#include <string>

#include "network/server.h"
#include "network/threaded_server.h"

// Global variable for signal handling
volatile sig_atomic_t g_running = 1;

enum class ServerMode { EVENT_LOOP, MULTI_THREADED };

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
            // Fallback to default port
        }
    }
    return 6379;  // Default Redis port
}

void print_usage(const char* program_name) {
    std::cout << "Usage: " << program_name << " [OPTIONS]\n"
              << "Options:\n"
              << "  --mode=<type>     Server mode: 'eventloop' (default) or 'threaded'\n"
              << "  --port=<number>   Port number (default: 6379)\n"
              << "  -h, --help        Show this help message\n"
              << "\nExamples:\n"
              << "  " << program_name << "                    # Event loop server on port 6379\n"
              << "  " << program_name << " --mode=threaded    # Multi-threaded server\n"
              << "  " << program_name << " --port=8080        # Custom port\n";
}

struct ServerConfig {
    ServerMode mode = ServerMode::EVENT_LOOP;
    int port = 6379;
};

ServerConfig parse_arguments(int argc, char* argv[]) {
    ServerConfig config;
    config.port = get_port_from_env();

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];

        if (arg == "-h" || arg == "--help") {
            print_usage(argv[0]);
            exit(0);
        } else if (arg.substr(0, 7) == "--mode=") {
            std::string mode = arg.substr(7);
            if (mode == "eventloop") {
                config.mode = ServerMode::EVENT_LOOP;
            } else if (mode == "threaded") {
                config.mode = ServerMode::MULTI_THREADED;
            } else {
                throw std::invalid_argument("Invalid mode: " + mode +
                                            ". Use 'eventloop' or 'threaded'");
            }
        } else if (arg.substr(0, 7) == "--port=") {
            config.port = std::stoi(arg.substr(7));
            if (config.port <= 0 || config.port > 65535) {
                throw std::out_of_range("Port number out of range");
            }
        } else {
            // Try to parse as port number for backward compatibility
            try {
                config.port = std::stoi(arg);
                if (config.port <= 0 || config.port > 65535) {
                    throw std::out_of_range("Port number out of range");
                }
            } catch (const std::exception&) {
                throw std::invalid_argument("Unknown argument: " + arg);
            }
        }
    }

    return config;
}
}  // namespace

int main(int argc, char* argv[]) {
    try {
        setup_signal_handlers();

        ServerConfig config = parse_arguments(argc, argv);

        const char* mode_name =
            (config.mode == ServerMode::EVENT_LOOP) ? "Event Loop" : "Multi-threaded";

        std::cout << "Redis Clone Server v0.1.0\n"
                  << "Mode: " << mode_name << "\n"
                  << "Port: " << config.port << "\n"
                  << "PID: " << getpid() << "\n"
                  << "--------------------------------\n";

        if (config.mode == ServerMode::EVENT_LOOP) {
            redis_clone::network::RedisServer server(config.port);
            std::cout << "Event loop server ready to accept connections\n";
            server.run();
        } else {
            redis_clone::network::ThreadedRedisServer server(config.port);
            std::cout << "Multi-threaded server ready to accept connections\n";
            server.run();
        }

        return 0;

    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << "\n";
        return 1;
    } catch (...) {
        std::cerr << "Unknown fatal error occurred\n";
        return 2;
    }
}
