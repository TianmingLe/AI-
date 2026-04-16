#include "EventBus.h"

#include "LogManager.h"

#include <exception>

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
        try {
            h(topic, payload);
        } catch (const std::exception& e) {
            LOG_ERROR("EventBus: handler exception topic=" + topic + " what=" + std::string(e.what()));
        } catch (...) {
            LOG_ERROR("EventBus: handler unknown exception topic=" + topic);
        }
    }
}

} // namespace core
