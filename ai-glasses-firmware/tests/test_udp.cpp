#include <catch2/catch_test_macros.hpp>

#include "../src/comm/UdpBroadcaster.h"

TEST_CASE("UdpBroadcaster start and broadcast", "[udp]") {
    comm::UdpBroadcaster udp(9999);
    REQUIRE(udp.start());
    udp.broadcast("{\"test\":true}");
    udp.stop();
    REQUIRE(true);
}
