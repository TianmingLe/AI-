# WebSocket/HTTP 服务器与 IMU 传感器集成设计

## 1. 概述
本规范概述了向 AI 智能眼镜固件中添加两个新组件的架构和集成策略：
1. **网络协议**：本地 WebSocket/HTTP 服务器 (`comm::WebServer`)。
2. **新硬件**：惯性测量单元 (IMU) 传感器 (`perception::ImuSensor`)。

这两个组件都将遵循项目中既定的动态库加载模式（或系统级 Mock 机制），以确保在受限的沙盒环境中能够顺利编译并执行基础功能，而无需硬依赖外部库。

## 2. 架构与职责

### 2.1 `comm::WebServer` (WebSocket/HTTP 服务器)
- **目的**：提供一个本地控制台，用于诊断、配置更新或实时遥测数据流（例如，将检测事件和原始边界框发送到连接的浏览器或移动应用）。
- **接口**：
  - `start(int port)`：初始化并在后台线程中启动 HTTP/WS 监听循环。
  - `stop()`：优雅地关闭服务器。
  - `broadcast(const std::string& message)`：向所有连接的 WebSocket 客户端发送 JSON 格式的遥测数据。
- **实现策略**：
  - 尝试动态加载标准的轻量级 C++ HTTP/WS 库（如 `libwebsockets.so` 或 `libmicrohttpd.so`）。
  - 如果缺失该库，它将优雅地降级为 `use_mock_` 状态，仅记录广播日志而不是直接丢弃。
- **与 `EventBus` 的集成**：
  - 它将订阅诸如 `perception/detections` 和 `sensor/imu` 等主题，并将这些负载数据通过 `broadcast` 方法广播出去。

### 2.2 `perception::ImuSensor` (IMU 硬件)
- **目的**：读取 6 轴/9 轴数据（俯仰角 Pitch、偏航角 Yaw、翻滚角 Roll、加速度），以提供头部追踪（Head Tracking）功能。
- **接口**：
  - `bool init(const std::string& i2c_device)`：打开 I2C 设备（如 `/dev/i2c-1`）并验证 IMU 的 WHO_AM_I 寄存器。
  - `ImuData readData()`：返回最新的欧拉角和线性加速度。
- **实现策略**：
  - 使用标准的 Linux I2C ioctl 接口（`<linux/i2c-dev.h>`）。
  - 如果 `/dev/i2c-x` 不存在或权限被拒绝，它将切换到 Mock 模式，生成平滑的正弦波模拟数据，以便测试下游数据管道。
- **与 `sensor_thread` 的集成**：
  - 现有的 `sensor_thread`（目前用于读取 BLE）将被扩展，以约 100Hz（10ms）的频率轮询 `ImuSensor`，并将 `sensor/imu` 事件发布到 `EventBus`。

## 3. 数据流

1. **IMU 轮询**：`sensor_thread` 每 10ms 调用一次 `perception::ImuSensor::readData()`。
2. **事件发布**：`sensor_thread` 将 IMU 数据打包为 JSON，并发布到 `sensor/imu` 主题。
3. **WebSocket 广播**：`comm::WebServer` 通过 `EventBus` 接收事件，并将其广播给任何连接的 Web/移动客户端。

## 4. 错误处理与边缘情况
- **端口冲突**：如果 WebServer 端口（如 8080）被占用，该组件将记录 `LOG_ERROR` 并安全降级到 Mock 模式。
- **I2C 总线错误**：如果 I2C 总线挂起或返回 `EIO`，IMU 将自动记录故障，切换到 Mock 模式，并在下一次循环中尝试软重置/重连。
- **优雅退出**：在主线程的 `SIGINT` 关闭阶段，WebServer 线程和 IMU I2C 文件描述符都将被干净地关闭。

## 5. 测试策略
- 这些组件将被集成到主 Catch2 测试套件中（例如，测试 IMU 在提供无效 I2C 路径时是否能优雅地回退到 Mock 模式）。
- 将通过 CMake 编译并在沙盒中运行可执行文件，观察 Mock 日志来验证其有效性。