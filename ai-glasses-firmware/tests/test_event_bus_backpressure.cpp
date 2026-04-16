#include <catch2/catch_test_macros.hpp>

#include "../src/core/EventBus.h"

#include <chrono>
#include <thread>

TEST_CASE("EventBus drops when overwhelmed", "[eventbus]") {
    core::EventBus bus(32, 1, 4);

    bus.subscribe("hot", [&](const std::string&, const std::string&) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    });

    for (int i = 0; i < 200; i++) {
        bus.publish("hot", std::to_string(i));
    }

    auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(800);
    while (bus.dropped() == 0 && std::chrono::steady_clock::now() < deadline) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    REQUIRE(bus.dropped() > 0);
}
