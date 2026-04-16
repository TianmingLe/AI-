#include <catch2/catch_test_macros.hpp>

#include "../src/comm/HttpClient.h"

#include <thread>

TEST_CASE("HttpClient concurrent calls do not deadlock", "[http]") {
    comm::HttpClient client;

    std::thread t1([&] { (void)client.get("http://127.0.0.1:1"); });
    std::thread t2([&] { (void)client.post("http://127.0.0.1:1", "{\"x\":1}"); });

    t1.join();
    t2.join();

    REQUIRE(true);
}
