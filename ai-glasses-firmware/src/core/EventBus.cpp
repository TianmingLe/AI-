#include "EventBus.h"

#include "LogManager.h"

#include <exception>
#include <utility>

namespace core {

EventBus::EventBus(size_t max_queue_size) : max_queue_size_(max_queue_size) {
    worker_ = std::thread([this] { workerLoop(); });
}

EventBus::~EventBus() {
    stopping_.store(true);
    cv_.notify_all();
    if (worker_.joinable()) worker_.join();
}

void EventBus::subscribe(const std::string& topic, Handler handler) {
    std::lock_guard<std::mutex> lock(mutex_);
    handlers_[topic].push_back(std::move(handler));
}

void EventBus::publish(const std::string& topic, const std::string& payload) {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (stopping_.load()) return;
        if (queue_.size() >= max_queue_size_) {
            queue_.pop_front();
            size_t dropped = dropped_.fetch_add(1) + 1;
            if (dropped == 1 || dropped % 100 == 0) {
                LOG_WARN("EventBus: queue overflow, dropped=" + std::to_string(dropped));
            }
        }
        queue_.push_back(Event{topic, payload});
    }
    cv_.notify_one();
}

void EventBus::workerLoop() {
    for (;;) {
        Event e;
        {
            std::unique_lock<std::mutex> lock(mutex_);
            cv_.wait(lock, [&] { return stopping_.load() || !queue_.empty(); });
            if (queue_.empty() && stopping_.load()) {
                return;
            }
            e = std::move(queue_.front());
            queue_.pop_front();
        }

        std::vector<Handler> snapshot;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            auto it = handlers_.find(e.topic);
            if (it != handlers_.end()) snapshot = it->second;
        }

        for (auto& h : snapshot) {
            try {
                h(e.topic, e.payload);
            } catch (const std::exception& ex) {
                LOG_ERROR("EventBus: handler exception topic=" + e.topic + " what=" + std::string(ex.what()));
            } catch (...) {
                LOG_ERROR("EventBus: handler unknown exception topic=" + e.topic);
            }
        }
    }
}

} // namespace core
