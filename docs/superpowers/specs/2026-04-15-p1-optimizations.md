# AI眼镜固件 - P1 级别优化设计与规范

## 1. 统一错误返回模型 (Result<T>)
当前很多硬件或通信模块在失败时返回“默认值”或“空字符串”（例如 GpsSensor 返回 `{0,0,0,0}`，HttpClient 返回 `""`），这导致调用方无法区分是真实空数据还是底层错误。
- **设计**：在 `core` 目录下引入通用的 `Result<T>` 模板类（或简单的 `std::optional<T>` 加错误码记录）。
- **应用**：将 `HttpClient::get` / `post` 的返回值从 `std::string` 改为 `std::optional<std::string>`，失败时记录日志并返回 `std::nullopt`。将 `GpsSensor::readLocation` 改为返回 `std::optional<GpsData>`。

## 2. SensorFusion 参数配置化
当前的卡尔曼滤波器参数（Q, R, P）是在 `SensorFusion.cpp` 的构造函数中硬编码的，无法适应不同环境下的硬件噪声特性。
- **设计**：在 `SensorFusion` 的构造函数中引入 `core::ConfigManager` 的依赖。
- **实现**：通过 `config.getFloat("kf_imu_q", 0.01)` 等方式读取参数。支持针对 Pitch/Yaw/Roll 以及 Lat/Lon 分别配置噪声模型。

## 3. 设备发现协议动态 IP 获取
`main.cpp` 中的 `discovery_thread` 广播 JSON 时，其 IP 地址目前硬编码为 `"192.168.1.100"`。
- **设计**：在 `comm::UdpBroadcaster` 或独立的工具类中，通过 Linux 系统的 `getifaddrs()` 动态获取当前活动网卡（例如 `wlan0` 或 `eth0`）的真实 IPv4 地址。
- **实现**：如果无法获取，则回退到 `127.0.0.1`，确保广播的 JSON 数据是真实的。

## 4. 实施步骤
1. 修改 `HttpClient` 和 `GpsSensor` 的头文件及实现，引入 `std::optional`。
2. 调整 `main.cpp` 中对应模块的调用方逻辑。
3. 修改 `SensorFusion`，将 `ConfigManager` 作为参数传入，并在初始化时读取滤波参数。
4. 编写 `NetworkUtils.h/cpp` 实现 `getLocalIP()`，并在 `discovery_thread` 中调用。
5. 重新编译并执行所有自动化测试。
