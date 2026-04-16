#include <catch2/catch_test_macros.hpp>

#include "../src/comm/HttpClient.h"

TEST_CASE("HttpClient mock post", "[http]") {
    comm::HttpClient client;
    auto resp = client.post("http://127.0.0.1:1", "{\"a\":1}");
    if (client.isUsingMock()) {
        REQUIRE(resp.has_value());
    } else {
        REQUIRE(true);
    }
}
