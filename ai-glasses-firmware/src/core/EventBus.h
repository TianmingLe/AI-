#pragma once

#include <atomic>
#include <condition_variable>
#include <cstddef>
#include <deque>
#include <functional>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

namespace core {

class EventBus {
public:
    using Handler = std::function<void(const std::string& topic, const std::string& payload)>;

    explicit EventBus(size_t max_queue_size = 1024, size_t worker_count = 1, size_t max_per_topic = 256);
    ~EventBus();

    EventBus(const EventBus&) = delete;
    EventBus& operator=(const EventBus&) = delete;

    void subscribe(const std::string& topic, Handler handler);
    void publish(const std::string& topic, const std::string& payload);
    size_t dropped() const;

private:
    struct Event {
        std::string topic;
        std::string payload;
    };

    void workerLoop();

    std::mutex mutex_;
    std::unordered_map<std::string, std::vector<Handler>> handlers_;

    std::condition_variable cv_;
    std::deque<Event> queue_;
    const size_t max_queue_size_;
    const size_t max_per_topic_;
    std::atomic<bool> stopping_{false};
    std::atomic<size_t> dropped_{0};
    std::unordered_map<std::string, size_t> topic_counts_;
    std::vector<std::thread> workers_;
};

} // namespace core
