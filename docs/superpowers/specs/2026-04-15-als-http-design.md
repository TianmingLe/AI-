# 环境光传感器 (ALS) 与 HTTP/REST 客户端集成设计

## 1. 概述
本规范详细说明了向 AI 智能眼镜固件中添加另外两个生产级组件的架构设计：
1. **新硬件**：环境光传感器 (Ambient Light Sensor, ALS) (`perception::AlsSensor`)，用于感知佩戴者周围的光照强度（Lux），以实现 AR 显示亮度的自动调节。
2. **网络协议**：HTTP/REST 客户端 (`comm::HttpClient`)，使眼镜能够主动向云端（如大模型 API 或 OTA 更新服务器）发起 RESTful 请求。

与整个固件的代码规范保持一致，这两个组件将采用动态库加载（`dlopen`）或原生接口，并提供极其健壮的 Mock 降级策略。如果沙盒环境缺少 I2C 设备或 `libcurl`，系统将平滑回退到 Mock 模式并输出模拟日志，确保极高的可移植性。

## 2. 架构与职责

### 2.1 `perception::AlsSensor` (环境光传感器)
- **目的**：通过 I2C 接口读取常见的光传感器（如 VEML7700、TSL2561）的 Lux（勒克斯）值。根据环境光线，未来 AR 渲染引擎可据此调节对比度和透明度。
- **接口**：
  - `bool init(const std::string& i2c_device)`：打开 I2C 总线并准备与 ALS 通信。
  - `float readLux()`：返回当前的光照强度（勒克斯）。
- **实现策略**：
  - 使用 Linux 原生的 `<linux/i2c-dev.h>` 接口进行 I2C 通信。
  - **Mock 降级**：如果在沙盒中找不到 `/dev/i2c-x`，Mock 模式会模拟一个光照变化的过程（例如，从室内 300 Lux 平滑过渡到室外 5000 Lux）。
- **线程集成**：
  - 在 `sensor_thread` 中增加对 `AlsSensor` 的轮询（例如，每秒 2 次，500ms 间隔）。
  - 将光照数据推送到 `EventBus` 的 `sensor/ambient_light` 主题。

### 2.2 `comm::HttpClient` (HTTP/REST 客户端)
- **目的**：用于通过标准的 HTTP/HTTPS 协议与云端 API 交互，如上传遥测快照、请求云端识别接口或下载配置文件。
- **接口**：
  - `std::string get(const std::string& url)`：发送 HTTP GET 请求并返回响应体。
  - `std::string post(const std::string& url, const std::string& json_payload)`：发送带有 JSON 负载的 HTTP POST 请求。
- **实现策略**：
  - 尝试动态加载 `libcurl.so.4`（或 `libcurl.so`）。
  - 使用 `curl_easy_init`, `curl_easy_setopt`, `curl_easy_perform` 等函数实现网络请求。
  - **Mock 降级**：如果缺少 `libcurl`，或者处于离线沙盒环境，它将拦截请求，将 URL 和 Payload 打印到 `LOG_DEBUG`，并返回一段预设的 JSON 成功响应（例如 `{"status":"mock_success"}`）。

## 3. 数据流与生命周期

1. **环境光轮询**：
   - `sensor_thread` 每隔 500ms 调用一次 `perception::AlsSensor::readLux()`。
   - 将光照值打包为 JSON（如 `{"lux": 850.5}`）并发布到 `sensor/ambient_light`。
2. **RESTful 交互**：
   - `comm::HttpClient` 将被其他需要主动发起网络请求的模块调用（目前作为一个独立的工具类存在）。
   - 为了演示集成，`sensor_thread` 可以在检测到光照剧烈变化时（例如突变超过 2000 Lux），通过 `HttpClient` 向一个云端分析接口发送 POST 请求，上传当前所有的环境状态。

## 4. 内存管理与异常安全
- **cURL 句柄管理**：动态加载 `libcurl` 后，所有的 `CURL*` 句柄将使用 `std::unique_ptr` 与自定义的 `curl_easy_cleanup` 删除器进行包裹，防止内存和文件描述符泄漏。
- **线程安全**：`HttpClient` 的请求方法内部是无状态的（Stateless），每次调用都会创建新的 cURL easy 句柄，从而天然支持多线程并发调用。
- **动态库卸载**：在 `HttpClient` 的析构函数中，将安全地调用 `dlclose` 释放 `libcurl.so`。

## 5. 测试策略
- 在 Catch2 中新增 `test_als.cpp` 和 `test_http.cpp`。
- 测试 `AlsSensor` 在无效设备路径下能否正确切换到 Mock 模式并输出有效的 Lux 值。
- 测试 `HttpClient` 的 Mock 模式能否正确拦截 GET/POST 调用并返回预设的 Mock 响应。