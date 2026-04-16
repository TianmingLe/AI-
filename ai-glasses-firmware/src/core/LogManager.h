#pragma once

#include <mutex>
#include <string>

namespace core {

enum class LogLevel {
    Debug = 0,
    Info = 1,
    Warn = 2,
    Error = 3
};

class LogManager {
public:
    static void setLevel(LogLevel level);
    static LogLevel getLevel();

    static void debug(const std::string& msg);
    static void info(const std::string& msg);
    static void warn(const std::string& msg);
    static void error(const std::string& msg);

private:
    static void log(LogLevel level, const std::string& msg);
    static std::mutex mutex_;
    static LogLevel level_;
};

} // namespace core

#define LOG_DEBUG(msg) core::LogManager::debug((msg))
#define LOG_INFO(msg) core::LogManager::info((msg))
#define LOG_WARN(msg) core::LogManager::warn((msg))
#define LOG_ERROR(msg) core::LogManager::error((msg))
