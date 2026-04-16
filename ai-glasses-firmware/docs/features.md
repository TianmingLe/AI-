# AI Glasses Firmware 功能说明

本文档描述当前仓库内 `ai-glasses-firmware` 已实现的功能、运行时行为、可配置项与构建测试方式。

## 1. 概览

该工程提供一个“可运行的固件骨架”：
- 多传感器采集（IMU/ALS/GPS/电源/音频）
- 异步事件总线（带线程池与背压丢弃策略）
- 多协议通信占位实现（MQTT/CoAP/HTTP/UDP 发现/NTP/OPC UA/WebServer/BLE）
- 启动阶段配置校验（schema 校验，错误退出，告警继续）
- 生产级构建选项（warnings/hardening/sanitizers）
- 单元测试（不依赖外网，覆盖关键路径）

## 2. 运行时组件与数据流

### 2.1 线程与生命周期

入口：[main.cpp](file:///workspace/ai-glasses-firmware/src/main.cpp)
- `t_shutdown`：监听 SIGINT/SIGTERM（优先 signalfd，失败则 self-pipe），触发 `running=false` 并唤醒等待条件变量，实现优雅退出。
- `t_sensor`：周期采集 BLE/IMU/GPS/Power/ALS，并将数据发布到 EventBus；ALS 同时通过 HTTP 上报。
- `t_audio`：周期读取音频 chunk，发布事件并通过 CoAP 发送。
- `t_disc`：周期通过 UDP 广播设备发现心跳包（包含 device_id 与本机 IP）。

主线程等待退出信号后 join 各线程并 stop/close 资源（UDP/WebServer 等）。

### 2.2 EventBus（异步/线程池/背压）

实现：[EventBus.h](file:///workspace/ai-glasses-firmware/src/core/EventBus.h)、[EventBus.cpp](file:///workspace/ai-glasses-firmware/src/core/EventBus.cpp)
- `publish(topic,payload)`：仅入队并快速返回，不在发布线程执行 handler，避免传感器线程被业务回调阻塞。
- worker 线程池：可配置 `worker_count`，并行消费事件队列执行 handler。
- 异常隔离：单个 handler 抛异常不会影响其他 handler 或 worker。
- 背压丢弃：
  - 全局队列溢出：丢弃最旧事件，并累计 `dropped`。
  - per-topic 积压上限：同一 topic 事件过多时丢弃该 topic 最旧事件。
- 可观测性：暴露 `dropped()` 计数（用于测试与运行时告警）。

### 2.3 传感器与融合

目录：`src/perception/`
- IMU：[ImuSensor.cpp](file:///workspace/ai-glasses-firmware/src/perception/ImuSensor.cpp)（当前为 mock，输出平滑变化的姿态数据）
- ALS：[AlsSensor.cpp](file:///workspace/ai-glasses-firmware/src/perception/AlsSensor.cpp)（mock lux 变化）
- 电源：[PowerSensor.cpp](file:///workspace/ai-glasses-firmware/src/perception/PowerSensor.cpp)（mock 电量缓慢下降）
- GPS：[GpsSensor.cpp](file:///workspace/ai-glasses-firmware/src/perception/GpsSensor.cpp)（mock 生成环形轨迹）
- 音频：[AudioInput.cpp](file:///workspace/ai-glasses-firmware/src/perception/AudioInput.cpp)（mock 输出固定大小采样 chunk）
- 传感器融合：[SensorFusion.cpp](file:///workspace/ai-glasses-firmware/src/perception/SensorFusion.cpp)
  - 使用 1D Kalman（pitch/yaw/roll/lat/lon）做简单平滑
  - 相关参数可配置（见配置章节）

### 2.4 通信与外设占位实现

目录：`src/comm/`
- BLE：[BleClient.cpp](file:///workspace/ai-glasses-firmware/src/comm/BleClient.cpp)（mock 读取传感器 JSON 字符串）
- UDP 发现广播：[UdpBroadcaster.cpp](file:///workspace/ai-glasses-firmware/src/comm/UdpBroadcaster.cpp)
  - 定时 broadcast `{"device": "...", "ip": "...", "status": "online"}`
  - 失败日志包含 errno
- HTTP 客户端：[HttpClient.h](file:///workspace/ai-glasses-firmware/src/comm/HttpClient.h)、[HttpClient.cpp](file:///workspace/ai-glasses-firmware/src/comm/HttpClient.cpp)
  - 动态加载 libcurl；不可用时自动使用 mock
  - 每次请求返回 `HttpResult{body,error,code}`，避免跨线程共享错误状态
  - 可配置 HTTPS 强制与 TLS 校验选项（见配置章节）
- CoAP：[CoapClient.cpp](file:///workspace/ai-glasses-firmware/src/comm/CoapClient.cpp)（mock send）
- MQTT：[MqttClient.cpp](file:///workspace/ai-glasses-firmware/src/comm/MqttClient.cpp)（mock publish）
- OPC UA：[OpcUaClient.cpp](file:///workspace/ai-glasses-firmware/src/comm/OpcUaClient.cpp)（mock write）
- NTP：[NtpClient.cpp](file:///workspace/ai-glasses-firmware/src/comm/NtpClient.cpp)（mock sync）
- WebServer：[WebServer.cpp](file:///workspace/ai-glasses-firmware/src/comm/WebServer.cpp)（mock start/stop）
- 本机 IP 获取：[NetworkUtils.h](file:///workspace/ai-glasses-firmware/src/comm/NetworkUtils.h)

### 2.5 渲染占位

目录：`src/render/`
- AR Renderer：[ArRenderer.cpp](file:///workspace/ai-glasses-firmware/src/render/ArRenderer.cpp)（mock renderOverlay，输出日志）

## 3. 配置（config.env）

### 3.1 配置加载与校验

实现：[ConfigManager](file:///workspace/ai-glasses-firmware/src/core/ConfigManager.cpp)、[ConfigSpec](file:///workspace/ai-glasses-firmware/src/core/ConfigSpec.cpp)
- 启动会读取 `config.env`（路径目前固定），失败会告警并继续使用默认值。
- 启动阶段会执行 schema 校验：
  - Warn：记录告警并继续（例如 log_level 非法）
  - Error：记录错误并退出（例如端口越界、HTTPS 强制但 endpoint 为 http）

### 3.2 示例配置

可参考：[config.example.env](file:///workspace/ai-glasses-firmware/config/config.example.env)

### 3.3 主要配置项（按模块）

**日志**
- `log_level`：debug|info|warn|error（非法值为 Warn，回退 info）

**EventBus**
- `eventbus_max_queue_size`：全局队列上限（1..1000000）
- `eventbus_worker_count`：worker 数（1..64）
- `eventbus_max_per_topic`：每 topic 最大积压（1..eventbus_max_queue_size）

**端口**
- `web_port`：WebServer 端口（1..65535）
- `udp_discovery_port`：UDP 广播端口（1..65535）

**HTTP/TLS**
- `http_require_https`：true/false
- `http_verify_peer`：true/false（require_https=true 时必须为 true）
- `http_verify_host`：true/false（require_https=true 时必须为 true）
- `http_ca_bundle`：CA bundle 路径（空表示使用系统默认）
- `als_http_endpoint`：ALS 上报地址，必须以 http:// 或 https:// 开头；require_https=true 时必须 https://

**网络/识别**
- `device_id`：设备标识（UDP 广播 payload）
- `network_interface`：网卡名（默认 eth0）

**传感器设备标识（当前用于 mock/占位）**
- `ble_device` `imu_device` `power_device` `als_device` `gps_device` `audio_device`

**融合参数（Kalman）**
- `kf_imu_q` `kf_imu_r` `kf_gps_q` `kf_gps_r`：必须 > 0

## 4. 构建与测试

### 4.1 典型构建

```bash
mkdir -p build
cd build
cmake ..
make -j4
ctest -V
```

### 4.2 生产级构建选项

顶层 CMake 提供：
- `AIGF_ENABLE_WARNINGS`（默认 ON）
- `AIGF_ENABLE_WERROR`（默认 OFF）
- `AIGF_ENABLE_ASAN` / `AIGF_ENABLE_UBSAN` / `AIGF_ENABLE_TSAN`（默认 OFF，TSAN 不可与 ASAN/UBSAN 同时开启）

示例：
```bash
cmake -S . -B build -DAIGF_ENABLE_WERROR=ON -DAIGF_ENABLE_ASAN=ON
cmake --build build -j4
ctest --test-dir build -V
```

## 5. 已知限制与后续工作方向

- 多个通信模块当前为 mock/占位实现（MQTT/CoAP/OPC UA/WebServer/NTP/BLE），接口与线程模型已搭好，但未接入真实协议栈。
- `config.env` 路径目前硬编码在 main；可扩展为 CLI 参数或环境变量指定。

