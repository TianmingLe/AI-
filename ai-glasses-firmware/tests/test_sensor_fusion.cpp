#include <catch2/catch_test_macros.hpp>

#include <cmath>

#include "../src/core/ConfigManager.h"
#include "../src/perception/SensorFusion.h"

static bool within(double a, double b, double eps) {
    return std::abs(a - b) <= eps;
}

TEST_CASE("SensorFusion smooth IMU", "[fusion]") {
    core::ConfigManager cfg;
    perception::SensorFusion fusion(cfg);

    perception::ImuData d1{10.0f, 20.0f, 30.0f};
    auto f1 = fusion.fuseImu(d1);
    REQUIRE(within(f1.pitch, 10.0, 1e-6));

    perception::ImuData d2{50.0f, 20.0f, 30.0f};
    auto f2 = fusion.fuseImu(d2);
    REQUIRE(f2.pitch > 10.0f);
    REQUIRE(f2.pitch < 50.0f);
}

TEST_CASE("SensorFusion smooth GPS", "[fusion]") {
    core::ConfigManager cfg;
    perception::SensorFusion fusion(cfg);

    perception::GpsData g1{37.7749, -122.4194, 10.0, 8};
    auto f1 = fusion.fuseGps(g1);
    REQUIRE(within(f1.latitude, g1.latitude, 1e-9));

    perception::GpsData g2{37.7800, -122.4200, 10.0, 8};
    auto f2 = fusion.fuseGps(g2);
    REQUIRE(f2.latitude > g1.latitude);
    REQUIRE(f2.latitude < g2.latitude);
}
