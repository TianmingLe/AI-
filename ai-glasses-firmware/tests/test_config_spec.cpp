#include <catch2/catch_test_macros.hpp>

#include "../src/core/ConfigManager.h"
#include "../src/core/ConfigSpec.h"

#include <cstdio>
#include <fstream>

static bool hasIssue(const std::vector<core::ConfigIssue>& issues, core::ConfigIssue::Level lvl, const char* key) {
    for (const auto& i : issues) {
        if (i.level == lvl && i.key == key) return true;
    }
    return false;
}

TEST_CASE("ConfigSpec validates schema", "[config]") {
    const char* path = "/tmp/ai_glasses_test_config_spec.env";
    {
        std::ofstream out(path);
        out << "log_level=bogus\n";
        out << "eventbus_worker_count=0\n";
        out << "web_port=70000\n";
        out << "http_require_https=true\n";
        out << "http_verify_peer=false\n";
        out << "http_verify_host=false\n";
        out << "als_http_endpoint=http://example.com/x\n";
        out << "kf_imu_q=-1\n";
    }

    core::ConfigManager cfg(path);
    REQUIRE(cfg.isLoaded());

    auto issues = core::ConfigSpec::validate(cfg);

    REQUIRE(hasIssue(issues, core::ConfigIssue::Level::Warn, "log_level"));
    REQUIRE(hasIssue(issues, core::ConfigIssue::Level::Error, "eventbus_worker_count"));
    REQUIRE(hasIssue(issues, core::ConfigIssue::Level::Error, "web_port"));
    REQUIRE(hasIssue(issues, core::ConfigIssue::Level::Error, "http_verify_peer"));
    REQUIRE(hasIssue(issues, core::ConfigIssue::Level::Error, "http_verify_host"));
    REQUIRE(hasIssue(issues, core::ConfigIssue::Level::Error, "als_http_endpoint"));
    REQUIRE(hasIssue(issues, core::ConfigIssue::Level::Error, "kf_imu_q"));

    std::remove(path);
}

