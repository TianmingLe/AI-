#pragma once

#include <functional>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace core {

class EventBus {
public:
    using Handler = std::function<void(const std::string& topic, const std::string& payload)>;

    void subscribe(const std::string& topic, Handler handler);
    void publish(const std::string& topic, const std::string& payload);

private:
    std::mutex mutex_;
    std::unordered_map<std::string, std::vector<Handler>> handlers_;
};

} // namespace core
