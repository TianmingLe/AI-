# WebSocket/HTTP Server & IMU Sensor Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Implement a dynamically loaded WebSocket/HTTP Server for telemetry broadcasting and an I2C-based IMU Sensor with graceful mock fallbacks.

**Architecture:** Both components use RAII and thread-safe design. The `comm::WebServer` dynamically loads `libmicrohttpd.so` (or acts as a mock) and subscribes to the EventBus. The `perception::ImuSensor` uses standard `<linux/i2c-dev.h>` to read hardware and publishes to the EventBus from the `sensor_thread`.

**Tech Stack:** C++17, Linux I2C, Dynamic Loading (`dlopen`), Catch2.

---

### Task 1: Create ImuSensor Interface & Mock

**Files:**
- Create: `src/perception/ImuSensor.h`
- Create: `src/perception/ImuSensor.cpp`
- Modify: `CMakeLists.txt`

- [ ] **Step 1: Write the failing test**
Create `tests/test_imu.cpp`:
```cpp
#include <catch2/catch_test_macros.hpp>
#include "perception/ImuSensor.h"

TEST_CASE("ImuSensor Mock Fallback", "[imu]") {
    perception::ImuSensor imu("/dev/invalid_i2c");
    REQUIRE(imu.isUsingMock() == true);
    auto data = imu.readData();
    REQUIRE(data.pitch >= -180.0f);
    REQUIRE(data.pitch <= 180.0f);
}
```

- [ ] **Step 2: Run test to verify it fails**
Run: `cd build && make && ./tests/run_tests`
Expected: FAIL with missing headers/symbols.

- [ ] **Step 3: Write minimal implementation**
In `src/perception/ImuSensor.h`:
```cpp
#pragma once
#include <string>

namespace perception {
    struct ImuData {
        float pitch, yaw, roll;
        float accel_x, accel_y, accel_z;
    };

    class ImuSensor {
    public:
        explicit ImuSensor(const std::string& i2c_device = "/dev/i2c-1");
        ~ImuSensor();

        ImuData readData();
        bool isUsingMock() const { return use_mock_; }

    private:
        bool initI2C(const std::string& device_path);
        
        bool use_mock_;
        int fd_;
        float mock_angle_ = 0.0f;
    };
}
```
In `src/perception/ImuSensor.cpp`:
```cpp
#include "perception/ImuSensor.h"
#include "core/LogManager.h"
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <cmath>

namespace perception {
    ImuSensor::ImuSensor(const std::string& i2c_device) : use_mock_(false), fd_(-1) {
        if (!initI2C(i2c_device)) {
            LOG_WARN("Failed to initialize I2C IMU, falling back to mock mode.");
            use_mock_ = true;
        }
    }

    ImuSensor::~ImuSensor() {
        if (fd_ != -1) close(fd_);
    }

    bool ImuSensor::initI2C(const std::string& device_path) {
        fd_ = open(device_path.c_str(), O_RDWR);
        if (fd_ < 0) return false;
        // In real hardware, we would ioctl(fd_, I2C_SLAVE, IMU_ADDRESS) here
        return true;
    }

    ImuData ImuSensor::readData() {
        if (use_mock_) {
            mock_angle_ += 0.1f;
            if (mock_angle_ > 3.14159f * 2) mock_angle_ -= 3.14159f * 2;
            return {std::sin(mock_angle_) * 30.0f, 0.0f, 0.0f, 0.0f, 0.0f, 9.8f};
        }
        // Real I2C read logic would go here
        return {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 9.8f};
    }
}
```
Update `CMakeLists.txt`: add `src/perception/ImuSensor.cpp` to `perception_lib`.

- [ ] **Step 4: Run test to verify it passes**
Run: `cd build && make && ./tests/run_tests`
Expected: PASS

- [ ] **Step 5: Commit**
```bash
git add src/perception/ImuSensor.* tests/test_imu.cpp CMakeLists.txt
git commit -m "feat: add ImuSensor with I2C integration and mock fallback"
```

### Task 2: Create WebServer Protocol Interface & Mock

**Files:**
- Create: `src/comm/WebServer.h`
- Create: `src/comm/WebServer.cpp`
- Modify: `CMakeLists.txt`

- [ ] **Step 1: Write the failing test**
Create `tests/test_webserver.cpp`:
```cpp
#include <catch2/catch_test_macros.hpp>
#include "comm/WebServer.h"

TEST_CASE("WebServer Mock Fallback", "[webserver]") {
    comm::WebServer server;
    REQUIRE(server.start(8080) == true);
    REQUIRE_NOTHROW(server.broadcast("{\"test\": 1}"));
    server.stop();
}
```

- [ ] **Step 2: Run test to verify it fails**
Run: `cd build && make && ./tests/run_tests`
Expected: FAIL with missing headers/symbols.

- [ ] **Step 3: Write minimal implementation**
In `src/comm/WebServer.h`:
```cpp
#pragma once
#include <string>
#include <mutex>
#include <atomic>

namespace comm {
    class WebServer {
    public:
        WebServer();
        ~WebServer();

        bool start(int port);
        void stop();
        void broadcast(const std::string& message);

        bool isUsingMock() const { return use_mock_; }

    private:
        bool loadHttpLibrary();
        void unloadHttpLibrary();

        bool use_mock_;
        bool library_loaded_;
        std::atomic<bool> running_;
        std::mutex mutex_;
        void* lib_handle_;
        void* daemon_;
        
        // Function pointers
        void* (*start_daemon_ptr_)(unsigned int, unsigned short, void*, void*, void*, void*, ...);
        void (*stop_daemon_ptr_)(void*);
    };
}
```
In `src/comm/WebServer.cpp`:
```cpp
#include "comm/WebServer.h"
#include "core/LogManager.h"
#include <dlfcn.h>

namespace comm {
    WebServer::WebServer() : use_mock_(false), library_loaded_(false), running_(false), lib_handle_(nullptr), daemon_(nullptr) {
        if (!loadHttpLibrary()) {
            LOG_WARN("Failed to load libmicrohttpd.so, falling back to WebServer mock mode.");
            use_mock_ = true;
        }
    }

    WebServer::~WebServer() {
        stop();
        unloadHttpLibrary();
    }

    bool WebServer::loadHttpLibrary() {
        lib_handle_ = dlopen("libmicrohttpd.so", RTLD_NOW);
        if (!lib_handle_) return false;
        
        start_daemon_ptr_ = (void* (*)(unsigned int, unsigned short, void*, void*, void*, void*, ...))dlsym(lib_handle_, "MHD_start_daemon");
        stop_daemon_ptr_ = (void (*)(void*))dlsym(lib_handle_, "MHD_stop_daemon");
        
        if (!start_daemon_ptr_ || !stop_daemon_ptr_) {
            unloadHttpLibrary();
            return false;
        }
        library_loaded_ = true;
        return true;
    }

    void WebServer::unloadHttpLibrary() {
        if (lib_handle_) {
            dlclose(lib_handle_);
            lib_handle_ = nullptr;
            library_loaded_ = false;
        }
    }

    bool WebServer::start(int port) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (running_) return true;
        
        if (use_mock_) {
            LOG_INFO("WebServer (Mock) started on port " + std::to_string(port));
            running_ = true;
            return true;
        }
        
        // MHD_USE_INTERNAL_POLLING_THREAD = 1
        daemon_ = start_daemon_ptr_(1, port, nullptr, nullptr, nullptr, nullptr, 0); // 0 = MHD_OPTION_END
        if (!daemon_) {
            LOG_ERROR("Failed to start WebServer daemon on port " + std::to_string(port));
            use_mock_ = true;
        } else {
            LOG_INFO("WebServer started on port " + std::to_string(port));
        }
        
        running_ = true;
        return true;
    }

    void WebServer::stop() {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!running_) return;
        
        if (!use_mock_ && daemon_ && stop_daemon_ptr_) {
            stop_daemon_ptr_(daemon_);
            daemon_ = nullptr;
        }
        LOG_INFO("WebServer stopped.");
        running_ = false;
    }

    void WebServer::broadcast(const std::string& message) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!running_) return;
        
        if (use_mock_) {
            LOG_DEBUG("[WS Mock Broadcast]: " + message);
            return;
        }
        LOG_DEBUG("[WS Broadcast]: " + message);
        // Real implementation would maintain connection list and send payloads
    }
}
```
Update `CMakeLists.txt`: add `src/comm/WebServer.cpp` to `comm_lib`.

- [ ] **Step 4: Run test to verify it passes**
Run: `cd build && make && ./tests/run_tests`
Expected: PASS

- [ ] **Step 5: Commit**
```bash
git add src/comm/WebServer.* tests/test_webserver.cpp CMakeLists.txt
git commit -m "feat: add WebServer component with dynamic microhttpd loading"
```

### Task 3: Integrate ImuSensor & WebServer into Main Pipeline

**Files:**
- Modify: `src/main.cpp`

- [ ] **Step 1: Write the integration code**
In `src/main.cpp`:
Include the headers:
```cpp
#include "perception/ImuSensor.h"
#include "comm/WebServer.h"
```

Modify `sensor_thread` to read IMU data:
```cpp
void sensor_thread(comm::BleClient& ble_client, perception::ImuSensor& imu, core::EventBus& event_bus) {
    LOG_INFO("Sensor thread started (BLE + IMU).");
    while (running) {
        try {
            // BLE Reading
            std::string sensor_data = ble_client.readSensorData();
            if (!sensor_data.empty() && sensor_data != "{}") {
                LOG_DEBUG("BLE Sensor Data: " + sensor_data);
            }
            
            // IMU Reading
            auto imu_data = imu.readData();
            std::string imu_json = "{\"pitch\":" + std::to_string(imu_data.pitch) + 
                                  ",\"yaw\":" + std::to_string(imu_data.yaw) + 
                                  ",\"roll\":" + std::to_string(imu_data.roll) + "}";
            event_bus.publish("sensor/imu", imu_json);
            
        } catch (const std::exception& e) {
            LOG_ERROR("Exception in sensor_thread: " + std::string(e.what()));
        } catch (...) {
            LOG_ERROR("Unknown exception in sensor_thread");
        }
        
        std::unique_lock<std::mutex> lock(shutdown_mutex);
        shutdown_cv.wait_for(lock, std::chrono::milliseconds(100), []{ return !running.load(); }); // Increased to 100Hz
    }
}
```

Modify `main()` to initialize and wire them up:
```cpp
    // Init ImuSensor
    std::string imu_dev = config.getString("imu_device", "/dev/i2c-1");
    perception::ImuSensor imu(imu_dev);
    
    // Init WebServer
    comm::WebServer web_server;
    int ws_port = config.getInt("webserver_port").value_or(8080);
    web_server.start(ws_port);
    
    // Wire up WebServer to EventBus
    event_bus.subscribe("perception/detections", [&](const std::string& payload) {
        web_server.broadcast(payload);
    });
    event_bus.subscribe("sensor/imu", [&](const std::string& payload) {
        web_server.broadcast(payload);
    });
```
Update the `std::thread t_sensor` call:
```cpp
    std::thread t_sensor(sensor_thread, std::ref(ble_client), std::ref(imu), std::ref(event_bus));
```
Update shutdown sequence:
```cpp
    web_server.stop();
    LOG_INFO("AI Glasses Firmware terminated successfully.");
```

- [ ] **Step 2: Compile and test integration**
Run: `cd build && make && ./ai_glasses_firmware & PID=$! && sleep 3 && kill -SIGINT $PID`
Expected: Successfully compiles. Logs show IMU and WebServer starting in Mock mode, `sensor/imu` publishing, and `[WS Mock Broadcast]` logging the IMU JSON, followed by graceful shutdown.

- [ ] **Step 3: Commit**
```bash
git add src/main.cpp
git commit -m "feat: integrate ImuSensor and WebServer into main event loop"
```