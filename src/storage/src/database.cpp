#include "storage/database.h"

namespace redis_clone {
namespace storage {

void Database::set(const std::string& key, const std::string& value) { data_[key] = value; }

std::optional<std::string> Database::get(const std::string& key) const {
    auto it = data_.find(key);
    if (it != data_.end()) {
        return it->second;
    }
    return std::nullopt;
}

bool Database::del(const std::string& key) { return data_.erase(key) > 0; }

bool Database::exists(const std::string& key) const { return data_.find(key) != data_.end(); }

}  // namespace storage
}  // namespace redis_clone