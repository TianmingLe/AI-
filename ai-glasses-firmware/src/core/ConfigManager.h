#pragma once

#include <optional>
#include <string>
#include <unordered_map>

namespace core {

class ConfigManager {
public:
    explicit ConfigManager(const std::string& path = "");

    bool load(const std::string& path);

    std::string getString(const std::string& key, const std::string& default_value = "") const;
    std::optional<int> getInt(const std::string& key) const;
    std::optional<double> getFloat(const std::string& key) const;

private:
    std::unordered_map<std::string, std::string> kv_;
};

} // namespace core
