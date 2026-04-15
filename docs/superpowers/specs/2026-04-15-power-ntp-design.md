# 电源管理 (PMIC) 与 NTP 网络时间同步协议集成设计

## 1. 概述
本规范详细说明了向 AI 智能眼镜固件中添加两个至关重要的新组件的架构设计：
1. **新硬件**：电源管理芯片 (PMIC) 传感器 (`perception::PowerSensor`)，用于实时监控电池电压、电流和剩余电量 (SoC)。
2. **网络协议**：网络时间协议 (NTP) 客户端 (`comm::NtpClient`)，用于将设备的系统时间与互联网时间服务器同步。

这两个组件都将采用原生 Linux API（I2C 和 UDP Sockets）编写，并遵循严格的动态 Mock 降级策略。如果沙盒环境缺少硬件权限（例如无法修改系统时间或读取 `/dev/i2c-x`），系统将优雅地回退到 Mock 模式并输出模拟数据，确保编译和基本运行不受影响。

## 2. 架构与职责

### 2.1 `perception::PowerSensor` (电源管理芯片)
- **目的**：通过 I2C 接口读取智能眼镜的电池状态（如 BQ27441 等常见电量计芯片）。这是可穿戴设备进行低电量报警和性能节流（Thermal Throttling）的基础。
- **接口**：
  - `bool init(const std::string& i2c_device)`：打开 I2C 总线并准备与 PMIC 通信。
  - `PowerData readBatteryStatus()`：返回当前的电压 (mV)、电流 (mA) 和剩余电量百分比 (%)。
- **实现策略**：
  - 使用 Linux 原生的 `<linux/i2c-dev.h>` 和 `ioctl` 接口读取寄存器。
  - **Mock 降级**：如果找不到硬件，Mock 模式会模拟一个电池随时间缓慢放电的过程（从 100% 匀速下降），并在电量低于 15% 时触发低电量状态。
- **线程集成**：
  - 将在现有的 `sensor_thread` 中增加对 `PowerSensor` 的轮询（频率较低，例如每秒 1 次）。
  - 将电池状态数据推送到 `EventBus` 的 `sensor/battery` 主题。

### 2.2 `comm::NtpClient` (NTP 时间同步协议)
- **目的**：使用标准 NTP 协议 (UDP 端口 123) 与时间服务器 (如 `pool.ntp.org`) 通信，获取准确的 UTC 时间，并校准本地系统时钟。这对于安全证书验证和日志落盘的时间戳至关重要。
- **接口**：
  - `bool syncTime(const std::string& ntp_server)`：向指定的 NTP 服务器发送请求，解析响应中的时间戳，并尝试应用到系统。
- **实现策略**：
  - 不依赖第三方库，直接使用标准的 POSIX UDP Sockets (`<sys/socket.h>`, `<netinet/in.h>`) 构建并解析 48 字节的 NTP 数据包。
  - 尝试使用 `settimeofday` 或 `clock_settime` 调整系统时间。
  - **Mock 降级**：如果在沙盒或非 root 环境下没有权限修改系统时间（`EPERM`），或者网络不可达，它只会打印计算出的时间偏差（Offset）日志，而不会崩溃。

## 3. 数据流与生命周期

1. **时间同步 (启动时)**：
   - `main.cpp` 在启动各种工作线程之前，会调用 `comm::NtpClient::syncTime()`。
   - 如果同步成功（或 Mock 成功），系统记录当前对齐的时间。
2. **电池轮询 (运行时)**：
   - `sensor_thread` 每隔一定时间（如 1 秒）调用 `perception::PowerSensor::readBatteryStatus()`。
   - 将结果（如 `{"voltage": 3800, "soc": 85}`）发布到 `sensor/battery`。
3. **事件分发**：
   - `comm::WebServer` 和 `comm::MqttClient` 订阅 `sensor/battery`，将电池状态实时推送到局域网 Dashboard 和云端。
   - 如果电量过低（例如低于 10%），`sensor_thread` 会向 `EventBus` 发布紧急关机请求。

## 4. 错误处理与异常安全
- **网络超时**：NTP 的 UDP Socket 必须设置接收超时（`SO_RCVTIMEO`，例如 2 秒），以防止网络阻塞导致整个固件的启动流程被无限期挂起。
- **I2C 总线复位**：如果读取电量计连续多次失败，`PowerSensor` 应记录错误并自动回退到 Mock 模式，而不是让传感器线程崩溃。

## 5. 测试策略
- 在 Catch2 中新增 `test_power.cpp` 和 `test_ntp.cpp`。
- 测试 `PowerSensor` 的 Mock 模式是否能正确模拟电池随时间的消耗曲线。
- 测试 `NtpClient` 在提供无效的 NTP 服务器域名时能否快速超时并返回失败。