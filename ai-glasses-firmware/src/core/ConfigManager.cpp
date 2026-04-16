#include "ConfigManager.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <sstream>

namespace core {

static inline void ltrim(std::string& s) {
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) { return !std::isspace(ch); }));
}

static inline void rtrim(std::string& s) {
    s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) { return !std::isspace(ch); }).base(), s.end());
}

static inline void trim(std::string& s) {
    ltrim(s);
    rtrim(s);
}

ConfigManager::ConfigManager(const std::string& path) {
    if (!path.empty()) {
        load(path);
    }
}

bool ConfigManager::load(const std::string& path) {
    std::ifstream in(path);
    if (!in.is_open()) {
        return false;
    }

    std::string line;
    while (std::getline(in, line)) {
        trim(line);
        if (line.empty()) continue;
        if (line[0] == '#') continue;

        auto pos = line.find('=');
        if (pos == std::string::npos) continue;

        std::string key = line.substr(0, pos);
        std::string value = line.substr(pos + 1);
        trim(key);
        trim(value);
        if (key.empty()) continue;
        kv_[key] = value;
    }

    return true;
}

std::string ConfigManager::getString(const std::string& key, const std::string& default_value) const {
    auto it = kv_.find(key);
    if (it == kv_.end()) return default_value;
    return it->second;
}

std::optional<int> ConfigManager::getInt(const std::string& key) const {
    auto it = kv_.find(key);
    if (it == kv_.end()) return std::nullopt;
    try {
        return std::stoi(it->second);
    } catch (...) {
        return std::nullopt;
    }
}

std::optional<double> ConfigManager::getFloat(const std::string& key) const {
    auto it = kv_.find(key);
    if (it == kv_.end()) return std::nullopt;
    try {
        return std::stod(it->second);
    } catch (...) {
        return std::nullopt;
    }
}

} // namespace core
