#include <catch2/catch_test_macros.hpp>

#include "../src/core/ConfigManager.h"

#include <cstdio>
#include <fstream>

TEST_CASE("ConfigManager load and parse", "[config]") {
    {
        core::ConfigManager cfg("/tmp/does_not_exist.env");
        REQUIRE(!cfg.isLoaded());
        REQUIRE(!cfg.lastError().empty());
    }

    const char* path = "/tmp/ai_glasses_test_config.env";
    {
        std::ofstream out(path);
        out << "a=1\n";
        out << "b=hello\n";
        out << "bad_int=xx\n";
        out << "sz=10\n";
        out << "sz_bad=-1\n";
        out << "pi=3.14\n";
    }

    core::ConfigManager cfg(path);
    REQUIRE(cfg.isLoaded());
    REQUIRE(cfg.getString("b", "") == "hello");
    REQUIRE(cfg.getInt("a").has_value());
    REQUIRE(cfg.getInt("a").value() == 1);
    REQUIRE(!cfg.getInt("bad_int").has_value());
    REQUIRE(cfg.getFloat("pi").has_value());
    REQUIRE(cfg.getSizeT("sz").has_value());
    REQUIRE(cfg.getSizeT("sz").value() == 10);
    REQUIRE(!cfg.getSizeT("sz_bad").has_value());

    std::remove(path);
}
