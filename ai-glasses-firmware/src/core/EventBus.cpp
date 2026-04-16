#include "EventBus.h"

namespace core {

void EventBus::subscribe(const std::string& topic, Handler handler) {
    std::lock_guard<std::mutex> lock(mutex_);
    handlers_[topic].push_back(std::move(handler));
}

void EventBus::publish(const std::string& topic, const std::string& payload) {
    std::vector<Handler> snapshot;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = handlers_.find(topic);
        if (it != handlers_.end()) snapshot = it->second;
    }
    for (auto& h : snapshot) {
        h(topic, payload);
    }
}

} // namespace core
