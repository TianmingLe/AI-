#include <catch2/catch_test_macros.hpp>

#include "../src/core/EventBus.h"

#include <atomic>
#include <stdexcept>

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
    REQUIRE(calls.load() == 2);
}
