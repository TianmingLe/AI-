#include "ConfigSpec.h"

#include "LogManager.h"

#include <algorithm>
#include <cctype>

namespace core {

static std::string lower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return s;
}

static bool starts_with(const std::string& s, const std::string& prefix) {
    return s.rfind(prefix, 0) == 0;
}

static void addIssue(std::vector<ConfigIssue>& out, ConfigIssue::Level lvl, const std::string& key, const std::string& msg) {
    out.push_back(ConfigIssue{lvl, key, msg});
}

static void validateEnumLogLevel(const ConfigManager& cfg, std::vector<ConfigIssue>& out) {
    std::string v = lower(cfg.getString("log_level", "info"));
    if (v == "debug" || v == "info" || v == "warn" || v == "error") return;
    addIssue(out, ConfigIssue::Level::Warn, "log_level", "invalid value: " + v + " (fallback=info)");
}

static void validateSizeRange(const ConfigManager& cfg, std::vector<ConfigIssue>& out, const std::string& key, size_t def, size_t minv, size_t maxv) {
    auto v = cfg.getSizeT(key);
    if (!v.has_value()) return;
    if (*v < minv || *v > maxv) {
        addIssue(out, ConfigIssue::Level::Error, key, "out of range: " + std::to_string(*v) + " (expect " + std::to_string(minv) +
                                                     ".." + std::to_string(maxv) + ")");
    }
}

static void validateIntRange(const ConfigManager& cfg, std::vector<ConfigIssue>& out, const std::string& key, int minv, int maxv) {
    auto v = cfg.getInt(key);
    if (!v.has_value()) return;
    if (*v < minv || *v > maxv) {
        addIssue(out, ConfigIssue::Level::Error, key, "out of range: " + std::to_string(*v) + " (expect " + std::to_string(minv) +
                                                     ".." + std::to_string(maxv) + ")");
    }
}

static void validateFloatPositive(const ConfigManager& cfg, std::vector<ConfigIssue>& out, const std::string& key) {
    auto v = cfg.getFloat(key);
    if (!v.has_value()) return;
    if (*v <= 0.0) {
        addIssue(out, ConfigIssue::Level::Error, key, "must be > 0");
    }
}

static void validateBooleanStrings(const ConfigManager& cfg, std::vector<ConfigIssue>& out, const std::string& key, const std::string& def) {
    std::string v = lower(cfg.getString(key, def));
    if (v == "true" || v == "false") return;
    addIssue(out, ConfigIssue::Level::Warn, key, "invalid boolean string: " + v + " (expect true/false)");
}

static void validateHttpTlsPolicy(const ConfigManager& cfg, std::vector<ConfigIssue>& out) {
    validateBooleanStrings(cfg, out, "http_require_https", "false");
    validateBooleanStrings(cfg, out, "http_verify_peer", "true");
    validateBooleanStrings(cfg, out, "http_verify_host", "true");

    bool require_https = lower(cfg.getString("http_require_https", "false")) == "true";
    bool verify_peer = lower(cfg.getString("http_verify_peer", "true")) == "true";
    bool verify_host = lower(cfg.getString("http_verify_host", "true")) == "true";

    if (require_https) {
        if (!verify_peer) addIssue(out, ConfigIssue::Level::Error, "http_verify_peer", "must be true when http_require_https=true");
        if (!verify_host) addIssue(out, ConfigIssue::Level::Error, "http_verify_host", "must be true when http_require_https=true");
    } else {
        if (!verify_peer) addIssue(out, ConfigIssue::Level::Warn, "http_verify_peer", "verify_peer disabled");
        if (!verify_host) addIssue(out, ConfigIssue::Level::Warn, "http_verify_host", "verify_host disabled");
    }

    std::string als = cfg.getString("als_http_endpoint", "http://api.factory-scada.local/sensor/als");
    if (!(starts_with(als, "http://") || starts_with(als, "https://"))) {
        addIssue(out, ConfigIssue::Level::Error, "als_http_endpoint", "must start with http:// or https://");
        return;
    }
    if (require_https && starts_with(als, "http://")) {
        addIssue(out, ConfigIssue::Level::Error, "als_http_endpoint", "http is not allowed when http_require_https=true");
    }
}

static void validateEventBusPolicy(const ConfigManager& cfg, std::vector<ConfigIssue>& out) {
    size_t max_q = cfg.getSizeT("eventbus_max_queue_size").value_or(1024);
    validateSizeRange(cfg, out, "eventbus_max_queue_size", 1024, 1, 1000000);
    validateSizeRange(cfg, out, "eventbus_worker_count", 1, 1, 64);
    validateSizeRange(cfg, out, "eventbus_max_per_topic", 256, 1, max_q);

    auto max_per_topic = cfg.getSizeT("eventbus_max_per_topic");
    if (max_per_topic.has_value() && *max_per_topic > max_q) {
        addIssue(out, ConfigIssue::Level::Error, "eventbus_max_per_topic", "must be <= eventbus_max_queue_size");
    }
}

static void validatePorts(const ConfigManager& cfg, std::vector<ConfigIssue>& out) {
    validateIntRange(cfg, out, "web_port", 1, 65535);
    validateIntRange(cfg, out, "udp_discovery_port", 1, 65535);
}

static void validateKalman(const ConfigManager& cfg, std::vector<ConfigIssue>& out) {
    validateFloatPositive(cfg, out, "kf_imu_q");
    validateFloatPositive(cfg, out, "kf_imu_r");
    validateFloatPositive(cfg, out, "kf_gps_q");
    validateFloatPositive(cfg, out, "kf_gps_r");
}

std::vector<ConfigIssue> ConfigSpec::validate(const ConfigManager& cfg) {
    std::vector<ConfigIssue> issues;
    validateEnumLogLevel(cfg, issues);
    validateEventBusPolicy(cfg, issues);
    validatePorts(cfg, issues);
    validateHttpTlsPolicy(cfg, issues);
    validateKalman(cfg, issues);
    return issues;
}

} // namespace core

