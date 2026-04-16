#include "EventBus.h"

#include "LogManager.h"

#include <exception>
#include <utility>

namespace core {

EventBus::EventBus(size_t max_queue_size, size_t worker_count, size_t max_per_topic)
    : max_queue_size_(max_queue_size), max_per_topic_(max_per_topic) {
    if (worker_count == 0) worker_count = 1;
    workers_.reserve(worker_count);
    for (size_t i = 0; i < worker_count; i++) {
        workers_.push_back(std::thread([this] { workerLoop(); }));
    }
}

EventBus::~EventBus() {
    stopping_.store(true);
    cv_.notify_all();
    for (auto& t : workers_) {
        if (t.joinable()) t.join();
    }
}

void EventBus::subscribe(const std::string& topic, Handler handler) {
    std::lock_guard<std::mutex> lock(mutex_);
    handlers_[topic].push_back(std::move(handler));
}

void EventBus::publish(const std::string& topic, const std::string& payload) {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (stopping_.load()) return;

        auto& topic_count = topic_counts_[topic];
        if (topic_count >= max_per_topic_) {
            for (auto it = queue_.begin(); it != queue_.end(); ++it) {
                if (it->topic == topic) {
                    queue_.erase(it);
                    topic_count--;
                    size_t dropped = dropped_.fetch_add(1) + 1;
                    if (dropped == 1 || dropped % 100 == 0) {
                        LOG_WARN("EventBus: per-topic overflow, dropped=" + std::to_string(dropped) + " topic=" + topic);
                    }
                    break;
                }
            }
        }

        if (queue_.size() >= max_queue_size_) {
            auto drop_topic = queue_.front().topic;
            queue_.pop_front();
            auto it = topic_counts_.find(drop_topic);
            if (it != topic_counts_.end() && it->second > 0) it->second--;
            size_t dropped = dropped_.fetch_add(1) + 1;
            if (dropped == 1 || dropped % 100 == 0) {
                LOG_WARN("EventBus: queue overflow, dropped=" + std::to_string(dropped));
            }
        }
        queue_.push_back(Event{topic, payload});
        topic_count++;
    }
    cv_.notify_one();
}

size_t EventBus::dropped() const {
    return dropped_.load();
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
            auto it = topic_counts_.find(e.topic);
            if (it != topic_counts_.end() && it->second > 0) it->second--;
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
