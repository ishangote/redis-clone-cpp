#pragma once

#include <string>

namespace redis_clone {
namespace network {
namespace redis_utils {

/**
 * @brief Represents parsed components of a Redis command
 */
struct CommandParts {
    std::string command;
    std::string key;
    std::string value;
};

/**
 * @brief Extracts command parts from a Redis command string
 * @param input The raw command string
 * @return CommandParts struct with parsed command, key, and value
 */
CommandParts extract_command(const std::string& input);

/**
 * @brief Processes a Redis command and returns appropriate response
 * @param parts The parsed command parts
 * @param data Reference to the key-value store
 * @return Response string in RESP format
 */
template <typename DataStore>
std::string process_command_with_store(const CommandParts& parts, DataStore& data);

}  // namespace redis_utils
}  // namespace network
}  // namespace redis_clone