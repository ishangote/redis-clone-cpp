#pragma once

#include <string>

namespace redis_clone {
namespace network {
namespace redis_utils {

/**
 * Parsed Redis command components
 */
struct CommandParts {
    std::string command;
    std::string key;
    std::string value;
};

/**
 * Parse Redis command string into components
 */
CommandParts extract_command(const std::string& input);

/**
 * Process Redis command and return RESP formatted response
 */
template <typename DataStore>
std::string process_command_with_store(const CommandParts& parts, DataStore& data);

}  // namespace redis_utils
}  // namespace network
}  // namespace redis_clone