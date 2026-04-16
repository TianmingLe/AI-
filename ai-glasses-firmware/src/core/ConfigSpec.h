#pragma once

#include "ConfigManager.h"

#include <string>
#include <vector>

namespace core {

struct ConfigIssue {
    enum class Level { Warn, Error };
    Level level;
    std::string key;
    std::string message;
};

class ConfigSpec {
public:
    static std::vector<ConfigIssue> validate(const ConfigManager& cfg);
};

} // namespace core

