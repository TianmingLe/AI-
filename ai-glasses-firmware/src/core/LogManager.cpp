#include "LogManager.h"

#include <chrono>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <sstream>

namespace core {

std::mutex LogManager::mutex_;
LogLevel LogManager::level_ = LogLevel::Info;

void LogManager::setLevel(LogLevel level) {
    std::lock_guard<std::mutex> lock(mutex_);
    level_ = level;
}

LogLevel LogManager::getLevel() {
    std::lock_guard<std::mutex> lock(mutex_);
    return level_;
}

void LogManager::debug(const std::string& msg) { log(LogLevel::Debug, msg); }
void LogManager::info(const std::string& msg) { log(LogLevel::Info, msg); }
void LogManager::warn(const std::string& msg) { log(LogLevel::Warn, msg); }
void LogManager::error(const std::string& msg) { log(LogLevel::Error, msg); }

static const char* toString(LogLevel level) {
    switch (level) {
        case LogLevel::Debug: return "DEBUG";
        case LogLevel::Info: return "INFO";
        case LogLevel::Warn: return "WARN";
        case LogLevel::Error: return "ERROR";
    }
    return "INFO";
}

void LogManager::log(LogLevel level, const std::string& msg) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (static_cast<int>(level) < static_cast<int>(level_)) {
        return;
    }

    auto now = std::chrono::system_clock::now();
    auto t = std::chrono::system_clock::to_time_t(now);
    std::tm tm{};
    localtime_r(&t, &tm);

    std::ostringstream ts;
    ts << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");

    std::cerr << "[" << ts.str() << "] [" << toString(level) << "] " << msg << "\n";
}

} // namespace core
