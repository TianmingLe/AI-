#include <catch2/catch_test_macros.hpp>

#include "../src/core/EventBus.h"

#include <atomic>
#include <chrono>
#include <stdexcept>
#include <thread>

TEST_CASE("EventBus isolates handler exceptions", "[eventbus]") {
    core::EventBus bus;
    std::atomic<int> calls{0};

    bus.subscribe("t", [&](const std::string&, const std::string&) {
        calls.fetch_add(1);
        throw std::runtime_error("boom");
    });

    bus.subscribe("t", [&](const std::string&, const std::string&) {
        calls.fetch_add(1);
    });

    bool threw = false;
    try {
        bus.publish("t", "x");
    } catch (...) {
        threw = true;
    }
    REQUIRE(!threw);

    auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(500);
    while (calls.load() < 2 && std::chrono::steady_clock::now() < deadline) {
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
    REQUIRE(calls.load() == 2);
}
