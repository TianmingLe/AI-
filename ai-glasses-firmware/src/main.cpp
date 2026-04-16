#include "comm/BleClient.h"
#include "comm/CoapClient.h"
#include "comm/HttpClient.h"
#include "comm/MqttClient.h"
#include "comm/NetworkUtils.h"
#include "comm/NtpClient.h"
#include "comm/OpcUaClient.h"
#include "comm/UdpBroadcaster.h"
#include "comm/WebServer.h"
#include "core/ConfigManager.h"
#include "core/EventBus.h"
#include "core/LogManager.h"
#include "perception/AlsSensor.h"
#include "perception/AudioInput.h"
#include "perception/GpsSensor.h"
#include "perception/ImuSensor.h"
#include "perception/PowerSensor.h"
#include "perception/SensorFusion.h"
#include "render/ArRenderer.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cctype>
#include <condition_variable>
#include <csignal>
#include <cstring>
#include <errno.h>
#include <fcntl.h>
#include <mutex>
#include <pthread.h>
#include <signal.h>
#include <sys/signalfd.h>
#include <thread>
#include <unistd.h>

static std::atomic<bool> running{true};
static std::mutex shutdown_mutex;
static std::condition_variable shutdown_cv;

static int pipe_write_fd = -1;

static void self_pipe_handler(int signo) {
    if (pipe_write_fd < 0) return;
    unsigned char b = static_cast<unsigned char>(signo);
    (void)!write(pipe_write_fd, &b, 1);
}

static void sensor_thread(
    comm::BleClient& ble,
    perception::ImuSensor& imu,
    perception::PowerSensor& power,
    perception::AlsSensor& als,
    perception::GpsSensor& gps,
    comm::HttpClient& http,
    perception::SensorFusion& fusion,
    const std::string& als_http_endpoint,
    core::EventBus& bus
) {
    int power_counter = 0;
    int als_counter = 0;
    int gps_counter = 0;

    while (running.load()) {
        std::string ble_data = ble.readSensorData();
        if (!ble_data.empty() && ble_data != "{}") {
            bus.publish("sensor/ble", ble_data);
        }

        auto raw_imu = imu.readData();
        auto fused_imu = fusion.fuseImu(raw_imu);
        bus.publish(
            "sensor/imu",
            "{\"pitch\":" + std::to_string(fused_imu.pitch) + ",\"yaw\":" + std::to_string(fused_imu.yaw) +
                ",\"roll\":" + std::to_string(fused_imu.roll) + "}"
        );

        if (++gps_counter >= 10) {
            gps_counter = 0;
            auto opt_gps = gps.readLocation();
            if (opt_gps.has_value()) {
                auto fused_gps = fusion.fuseGps(*opt_gps);
                bus.publish(
                    "sensor/gps",
                    "{\"lat\":" + std::to_string(fused_gps.latitude) + ",\"lon\":" + std::to_string(fused_gps.longitude) +
                        ",\"alt\":" + std::to_string(fused_gps.altitude) + ",\"sats\":" + std::to_string(fused_gps.satellites) +
                        "}"
                );
            }
        }

        if (++power_counter >= 30) {
            power_counter = 0;
            auto p = power.readPower();
            bus.publish(
                "sensor/power",
                "{\"battery_percent\":" + std::to_string(p.battery_percent) + ",\"voltage\":" + std::to_string(p.voltage) + "}"
            );
        }

        if (++als_counter >= 20) {
            als_counter = 0;
            float lux = als.readLux();
            std::string payload = "{\"lux\":" + std::to_string(lux) + "}";
            bus.publish("sensor/als", payload);
            auto resp = http.post(als_http_endpoint, payload);
            if (!resp.has_value() && !http.isUsingMock()) {
                LOG_WARN("ALS POST failed: " + http.getLastError());
            }
        }

        std::unique_lock<std::mutex> lock(shutdown_mutex);
        shutdown_cv.wait_for(lock, std::chrono::milliseconds(100), [] { return !running.load(); });
    }
}

static void audio_thread(perception::AudioInput& audio, comm::CoapClient& coap, core::EventBus& bus) {
    audio.start();
    while (running.load()) {
        auto chunk = audio.readChunk();
        if (!chunk.empty()) {
            bus.publish("audio/chunk", "{\"samples\":" + std::to_string(chunk.size()) + "}");
            coap.send("/audio", std::string(reinterpret_cast<const char*>(chunk.data()), chunk.size() * sizeof(int16_t)));
        }
        std::unique_lock<std::mutex> lock(shutdown_mutex);
        shutdown_cv.wait_for(lock, std::chrono::milliseconds(50), [] { return !running.load(); });
    }
    audio.stop();
}

static void discovery_thread(comm::UdpBroadcaster& udp, core::ConfigManager& config) {
    std::string device_id = config.getString("device_id", "AIGlasses_001");
    std::string iface = config.getString("network_interface", "eth0");
    while (running.load()) {
        std::string ip = comm::NetworkUtils::getLocalIP(iface);
        std::string payload = "{\"device\":\"" + device_id + "\",\"ip\":\"" + ip + "\",\"status\":\"online\"}";
        udp.broadcast(payload);
        std::unique_lock<std::mutex> lock(shutdown_mutex);
        shutdown_cv.wait_for(lock, std::chrono::seconds(5), [] { return !running.load(); });
    }
}

int main() {
    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGINT);
    sigaddset(&mask, SIGTERM);

    int shutdown_fd = -1;
    bool use_signalfd = false;

    if (pthread_sigmask(SIG_BLOCK, &mask, nullptr) == 0) {
        shutdown_fd = signalfd(-1, &mask, SFD_CLOEXEC);
        if (shutdown_fd >= 0) {
            use_signalfd = true;
        } else {
            (void)pthread_sigmask(SIG_UNBLOCK, &mask, nullptr);
        }
    }

    if (!use_signalfd) {
        int fds[2];
        if (pipe2(fds, O_CLOEXEC | O_NONBLOCK) != 0) {
            LOG_ERROR("Failed to set up shutdown pipe: " + std::string(std::strerror(errno)));
            return 1;
        }
        shutdown_fd = fds[0];
        pipe_write_fd = fds[1];

        struct sigaction sa {};
        sa.sa_handler = self_pipe_handler;
        sigemptyset(&sa.sa_mask);
        sa.sa_flags = 0;
        if (sigaction(SIGINT, &sa, nullptr) != 0 || sigaction(SIGTERM, &sa, nullptr) != 0) {
            LOG_ERROR("Failed to install signal handler: " + std::string(std::strerror(errno)));
            return 1;
        }
    }

    core::ConfigManager config("config.env");
    if (!config.isLoaded()) {
        LOG_WARN("Config load failed path=" + config.path() + " err=" + config.lastError());
    }

    std::string log_level = config.getString("log_level", "info");
    std::transform(log_level.begin(), log_level.end(), log_level.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    if (log_level == "debug") {
        core::LogManager::setLevel(core::LogLevel::Debug);
    } else if (log_level == "info") {
        core::LogManager::setLevel(core::LogLevel::Info);
    } else if (log_level == "warn") {
        core::LogManager::setLevel(core::LogLevel::Warn);
    } else if (log_level == "error") {
        core::LogManager::setLevel(core::LogLevel::Error);
    } else {
        LOG_WARN("Invalid log_level=" + log_level);
    }
    core::EventBus bus;

    std::string mqtt_broker = config.getString("mqtt_broker", "tcp://localhost:1883");
    std::string mqtt_client_id = config.getString("mqtt_client_id", "ai_glasses_001");
    comm::MqttClient mqtt(mqtt_broker, mqtt_client_id);
    mqtt.connect();

    comm::CoapClient coap(config.getString("coap_endpoint", "coap://localhost"));
    coap.connect();

    comm::WebServer web(config.getInt("web_port").value_or(8080));
    web.start();

    comm::OpcUaClient opcua(config.getString("opcua_endpoint", "opc.tcp://localhost:4840"));
    opcua.connect();

    comm::NtpClient ntp(config.getString("ntp_server", "pool.ntp.org"));
    (void)ntp.syncTime();

    comm::BleClient ble(config.getString("ble_device", "mock"));
    ble.connect();

    perception::ImuSensor imu(config.getString("imu_device", "mock"));
    perception::PowerSensor power(config.getString("power_device", "mock"));
    perception::AlsSensor als(config.getString("als_device", "mock"));
    perception::GpsSensor gps(config.getString("gps_device", "mock"));
    perception::AudioInput audio(config.getString("audio_device", "mock"));
    perception::SensorFusion fusion(config);
    comm::HttpClient http;

    comm::UdpBroadcaster udp(config.getInt("udp_discovery_port").value_or(9999));
    udp.start();

    render::ArRenderer renderer;

    bus.subscribe("sensor/imu", [&](const std::string&, const std::string& payload) {
        mqtt.publish("ai_glasses/imu", payload);
        opcua.writeNode("ns=2;s=imu", payload);
    });
    bus.subscribe("sensor/gps", [&](const std::string&, const std::string& payload) {
        mqtt.publish("ai_glasses/gps", payload);
    });
    bus.subscribe("sensor/als", [&](const std::string&, const std::string& payload) {
        mqtt.publish("ai_glasses/als", payload);
        renderer.renderOverlay(payload);
    });

    std::string als_http_endpoint = config.getString("als_http_endpoint", "http://api.factory-scada.local/sensor/als");

    std::thread t_shutdown([&] {
        if (use_signalfd) {
            while (running.load()) {
                signalfd_siginfo si {};
                ssize_t n = read(shutdown_fd, &si, sizeof(si));
                if (n == sizeof(si)) {
                    running.store(false);
                    shutdown_cv.notify_all();
                    break;
                }
                if (n < 0 && errno == EINTR) continue;
            }
        } else {
            while (running.load()) {
                unsigned char b = 0;
                ssize_t n = read(shutdown_fd, &b, 1);
                if (n > 0) {
                    running.store(false);
                    shutdown_cv.notify_all();
                    break;
                }
                if (n < 0 && (errno == EAGAIN || errno == EINTR)) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
                    continue;
                }
            }
        }
    });

    std::thread t_sensor(
        sensor_thread,
        std::ref(ble),
        std::ref(imu),
        std::ref(power),
        std::ref(als),
        std::ref(gps),
        std::ref(http),
        std::ref(fusion),
        std::ref(als_http_endpoint),
        std::ref(bus)
    );
    std::thread t_audio(audio_thread, std::ref(audio), std::ref(coap), std::ref(bus));
    std::thread t_disc(discovery_thread, std::ref(udp), std::ref(config));

    {
        std::unique_lock<std::mutex> lock(shutdown_mutex);
        shutdown_cv.wait(lock, [] { return !running.load(); });
    }

    if (t_sensor.joinable()) t_sensor.join();
    if (t_audio.joinable()) t_audio.join();
    if (t_disc.joinable()) t_disc.join();
    if (t_shutdown.joinable()) t_shutdown.join();

    if (shutdown_fd >= 0) close(shutdown_fd);
    if (pipe_write_fd >= 0) close(pipe_write_fd);

    udp.stop();
    web.stop();

    return 0;
}
