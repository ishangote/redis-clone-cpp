#include "network/redis_utils.h"

#include <cctype>
#include <sstream>
#include <unordered_map>

namespace redis_clone {
namespace network {
namespace redis_utils {

/**
 * Parse Redis command string into structured components
 */
CommandParts extract_command(const std::string& input) {
    std::istringstream iss(input);
    std::string cmd, key, value;

    iss >> cmd;
    if (iss >> key) {
        iss >> value;
    }

    // Normalize command to uppercase
    for (char& c : cmd) {
        c = std::toupper(c);
    }

    return {cmd, key, value};
}

/**
 * Redis command processor for std::unordered_map storage
 */
template <>
std::string process_command_with_store<std::unordered_map<std::string, std::string>>(
    const CommandParts& parts, std::unordered_map<std::string, std::string>& data) {
    if (parts.command == "SET") {
        if (parts.key.empty() || parts.value.empty()) {
            return "-ERR wrong number of arguments for 'set' command\r\n";
        }
        data[parts.key] = parts.value;
        return "+OK\r\n";
    } else if (parts.command == "GET") {
        if (parts.key.empty()) {
            return "-ERR wrong number of arguments for 'get' command\r\n";
        }
        auto it = data.find(parts.key);
        if (it != data.end()) {
            return "$" + std::to_string(it->second.size()) + "\r\n" + it->second + "\r\n";
        }
        return "$-1\r\n";
    } else if (parts.command == "DEL") {
        if (parts.key.empty()) {
            return "-ERR wrong number of arguments for 'del' command\r\n";
        }
        size_t deleted = data.erase(parts.key);
        return ":" + std::to_string(deleted) + "\r\n";
    } else if (parts.command == "EXISTS") {
        if (parts.key.empty()) {
            return "-ERR wrong number of arguments for 'exists' command\r\n";
        }
        bool exists = data.find(parts.key) != data.end();
        return ":" + std::to_string(exists ? 1 : 0) + "\r\n";
    } else if (parts.command == "QUIT") {
        return "+OK\r\n";
    } else if (parts.command == "BGSAVE") {
        return "+BGSAVE\r\n";
    }

    return "-ERR unknown command '" + parts.command + "'\r\n";
}

}  // namespace redis_utils
}  // namespace network
}  // namespace redis_clone