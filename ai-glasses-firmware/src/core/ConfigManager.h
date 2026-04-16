#pragma once

#include <cstddef>
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
    std::optional<size_t> getSizeT(const std::string& key) const;
    bool isLoaded() const;
    std::string lastError() const;
    std::string path() const;

private:
    std::unordered_map<std::string, std::string> kv_;
    std::string path_;
    std::string last_error_;
    bool loaded_{false};
};

} // namespace core
