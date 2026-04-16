#include <catch2/catch_test_macros.hpp>

#include "../src/perception/GpsSensor.h"

TEST_CASE("GpsSensor mock", "[gps]") {
    perception::GpsSensor gps("mock");
    REQUIRE(gps.isUsingMock());
    auto d1 = gps.readLocation();
    REQUIRE(d1.has_value());
    REQUIRE(d1->latitude != 0.0);
    REQUIRE(d1->longitude != 0.0);
    auto d2 = gps.readLocation();
    REQUIRE(d2.has_value());
    REQUIRE((d1->latitude != d2->latitude || d1->longitude != d2->longitude));
}
