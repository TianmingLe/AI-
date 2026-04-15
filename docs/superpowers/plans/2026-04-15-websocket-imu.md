# WebSocket/HTTP 服务器与 IMU 传感器实施计划

> **对于代理工具（Agent）：** 需要子技能支持：使用 superpowers:subagent-driven-development（推荐）或 superpowers:executing-plans 逐个任务地执行此计划。步骤使用复选框（`- [ ]`）语法进行跟踪。

**目标**：实现一个可动态加载的 WebSocket/HTTP 服务器用于遥测数据广播，以及一个基于 I2C 的 IMU 传感器，两者都具备优雅的 Mock 降级功能。

**架构**：两个组件均使用 RAII 和线程安全设计。`comm::WebServer` 动态加载 `libmicrohttpd.so`（或作为 Mock 运行）并订阅 EventBus。`perception::ImuSensor` 使用标准的 `<linux/i2c-dev.h>` 读取硬件数据，并在 `sensor_thread` 线程中向 EventBus 发布数据。

**技术栈**：C++17, Linux I2C, 动态加载（`dlopen`）, Catch2。

---

### 任务 1：创建 ImuSensor 接口与 Mock

**相关文件**：
- 创建：`src/perception/ImuSensor.h`
- 创建：`src/perception/ImuSensor.cpp`
- 修改：`CMakeLists.txt`

- [ ] **步骤 1：编写预期的失败测试**
创建 `tests/test_imu.cpp`：
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

- [ ] **步骤 2：运行测试以验证其失败**
运行：`cd build && make && ./tests/run_tests`
预期结果：因缺少头文件/符号而失败 (FAIL)。

- [ ] **步骤 3：编写最小实现代码**
在 `src/perception/ImuSensor.h` 中：
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
在 `src/perception/ImuSensor.cpp` 中：
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
        // 在真实硬件中，我们会在此时调用 ioctl(fd_, I2C_SLAVE, IMU_ADDRESS)
        return true;
    }

    ImuData ImuSensor::readData() {
        if (use_mock_) {
            mock_angle_ += 0.1f;
            if (mock_angle_ > 3.14159f * 2) mock_angle_ -= 3.14159f * 2;
            return {std::sin(mock_angle_) * 30.0f, 0.0f, 0.0f, 0.0f, 0.0f, 9.8f};
        }
        // 真实的 I2C 读取逻辑将在此处实现
        return {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 9.8f};
    }
}
```
更新 `CMakeLists.txt`：将 `src/perception/ImuSensor.cpp` 添加到 `perception_lib` 中。

- [ ] **步骤 4：运行测试以验证其通过**
运行：`cd build && make && ./tests/run_tests`
预期结果：通过 (PASS)

- [ ] **步骤 5：提交代码**
```bash
git add src/perception/ImuSensor.* tests/test_imu.cpp CMakeLists.txt
git commit -m "feat: add ImuSensor with I2C integration and mock fallback"
```

### 任务 2：创建 WebServer 协议接口与 Mock

**相关文件**：
- 创建：`src/comm/WebServer.h`
- 创建：`src/comm/WebServer.cpp`
- 修改：`CMakeLists.txt`

- [ ] **步骤 1：编写预期的失败测试**
创建 `tests/test_webserver.cpp`：
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

- [ ] **步骤 2：运行测试以验证其失败**
运行：`cd build && make && ./tests/run_tests`
预期结果：因缺少头文件/符号而失败 (FAIL)。

- [ ] **步骤 3：编写最小实现代码**
在 `src/comm/WebServer.h` 中：
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
        
        // 函数指针
        void* (*start_daemon_ptr_)(unsigned int, unsigned short, void*, void*, void*, void*, ...);
        void (*stop_daemon_ptr_)(void*);
    };
}
```
在 `src/comm/WebServer.cpp` 中：
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
        // 真实的实现将在此维护连接列表并发送负载数据
    }
}
```
更新 `CMakeLists.txt`：将 `src/comm/WebServer.cpp` 添加到 `comm_lib` 中。

- [ ] **步骤 4：运行测试以验证其通过**
运行：`cd build && make && ./tests/run_tests`
预期结果：通过 (PASS)

- [ ] **步骤 5：提交代码**
```bash
git add src/comm/WebServer.* tests/test_webserver.cpp CMakeLists.txt
git commit -m "feat: add WebServer component with dynamic microhttpd loading"
```

### 任务 3：将 ImuSensor 与 WebServer 集成到主数据管道中

**相关文件**：
- 修改：`src/main.cpp`

- [ ] **步骤 1：编写集成代码**
在 `src/main.cpp` 中：
引入头文件：
```cpp
#include "perception/ImuSensor.h"
#include "comm/WebServer.h"
```

修改 `sensor_thread` 以读取 IMU 数据：
```cpp
void sensor_thread(comm::BleClient& ble_client, perception::ImuSensor& imu, core::EventBus& event_bus) {
    LOG_INFO("Sensor thread started (BLE + IMU).");
    while (running) {
        try {
            // BLE 读取
            std::string sensor_data = ble_client.readSensorData();
            if (!sensor_data.empty() && sensor_data != "{}") {
                LOG_DEBUG("BLE Sensor Data: " + sensor_data);
            }
            
            // IMU 读取
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
        shutdown_cv.wait_for(lock, std::chrono::milliseconds(100), []{ return !running.load(); }); // 提升至 100Hz
    }
}
```

修改 `main()` 进行初始化和连线配置：
```cpp
    // 初始化 ImuSensor
    std::string imu_dev = config.getString("imu_device", "/dev/i2c-1");
    perception::ImuSensor imu(imu_dev);
    
    // 初始化 WebServer
    comm::WebServer web_server;
    int ws_port = config.getInt("webserver_port").value_or(8080);
    web_server.start(ws_port);
    
    // 将 WebServer 挂载到 EventBus
    event_bus.subscribe("perception/detections", [&](const std::string& payload) {
        web_server.broadcast(payload);
    });
    event_bus.subscribe("sensor/imu", [&](const std::string& payload) {
        web_server.broadcast(payload);
    });
```
更新 `std::thread t_sensor` 的调用：
```cpp
    std::thread t_sensor(sensor_thread, std::ref(ble_client), std::ref(imu), std::ref(event_bus));
```
更新关闭序列：
```cpp
    web_server.stop();
    LOG_INFO("AI Glasses Firmware terminated successfully.");
```

- [ ] **步骤 2：编译并测试集成**
运行：`cd build && make && ./ai_glasses_firmware & PID=$! && sleep 3 && kill -SIGINT $PID`
预期结果：编译成功。日志显示 IMU 和 WebServer 在 Mock 模式下启动，`sensor/imu` 成功发布，且 `[WS Mock Broadcast]` 打印出了 IMU 的 JSON 数据，随后完成优雅退出。

- [ ] **步骤 3：提交代码**
```bash
git add src/main.cpp
git commit -m "feat: integrate ImuSensor and WebServer into main event loop"
```