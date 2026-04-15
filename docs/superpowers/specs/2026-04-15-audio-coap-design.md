# 音频输入（麦克风阵列）与 CoAP 协议集成设计

## 1. 概述
本规范详细说明了向 AI 智能眼镜固件中添加两个全新的高性能、生产级组件的架构设计：
1. **新硬件**：用于音频输入的麦克风阵列 (`perception::AudioInput`)。
2. **网络协议**：CoAP (受限应用协议) 客户端 (`comm::CoapClient`)。

与代码库的其余部分保持一致，这两个组件都将依赖于动态库加载（`dlopen`）以确保极高的可移植性，并在底层硬件或操作系统库不可用时优雅地降级到 Mock 模式。所有更改都将遵守在之前的重构中建立的严格的线程安全和零拷贝原则。

## 2. 架构与职责

### 2.1 `perception::AudioInput` (麦克风阵列)
- **目的**：通过 ALSA (高级 Linux 声音架构) 从硬件麦克风捕获原始 PCM 音频数据（例如，16kHz，16 位单声道）。这为未来的语音唤醒（Wake-Word）检测或语音命令识别铺平了道路。
- **接口**：
  - `bool init(const std::string& alsa_device)`：打开 PCM 捕获设备（例如，`"default"` 或 `"hw:0,0"`）。
  - `std::vector<int16_t> readAudioChunk(size_t frame_count)`：读取一块音频帧数据。
- **实现策略**：
  - 使用 `dlopen` 动态加载 `libasound.so.2`。
  - 使用 `snd_pcm_open`、`snd_pcm_set_params` 和 `snd_pcm_readi` 函数。
  - **Mock 降级**：如果 ALSA 加载失败或不存在声卡，它将生成一个合成的 440Hz 正弦波（模拟的“滴”声）作为音频块返回。
- **线程集成**：
  - 将在 `main.cpp` 中生成一个新的专用 `audio_thread` 线程，以持续提取音频块（例如，每 100ms 一次），而不会阻塞感知或传感器线程。
  - 将音频振幅/RMS（均方根）或原始数据事件推送到 `EventBus` 的 `sensor/audio` 主题下。

### 2.2 `comm::CoapClient` (CoAP 协议)
- **目的**：一种基于 UDP 的轻量级物联网协议，用于高效地向后端服务器报告低带宽的遥测数据（如电池寿命、基本 IMU 统计信息或音频 RMS）。
- **接口**：
  - `bool connect(const std::string& coap_uri)`：解析服务器地址并准备 UDP 上下文。
  - `bool sendTelemetry(const std::string& path, const std::string& payload)`：构建一个 CoAP 不可确认 (NON) POST 请求并发送。
- **实现策略**：
  - 动态加载 `libcoap-3.so` 或 `libcoap-2.so`。
  - 使用诸如 `coap_new_context`、`coap_new_pdu` 和 `coap_send` 之类的函数。
  - **Mock 降级**：如果缺少库或网络不可达，它会将 CoAP 负载数据记录到 `LOG_DEBUG` 并立即返回成功状态。
- **与 `EventBus` 的集成**：
  - 订阅 `sensor/audio` 和 `sensor/imu` 事件，然后向 `coap://backend-server/telemetry` 发射轻量级的 CoAP 数据包。

## 3. 数据流

1. **音频捕获**：`audio_thread` 通过 ALSA 读取 PCM 块（`readAudioChunk`）。
2. **事件发布**：`audio_thread` 计算简单的 RMS (均方根) 以检测噪音水平，构建 JSON `{"rms": 0.45}`，并发布到 `sensor/audio`。
3. **CoAP 遥测**：`CoapClient` 通过 `EventBus` 接收 RMS 事件，并向 CoAP 服务器发射 UDP 数据包。

## 4. 安全与内存管理
- **ALSA 指针**：ALSA 上下文指针（`snd_pcm_t*`）将通过具有自定义删除器的 `std::unique_ptr` 进行管理，该删除器会调用 `snd_pcm_close` 以防止文件描述符泄漏。
- **CoAP 上下文**：类似地，`coap_context_t*` 和 `coap_session_t*` 也将被封装为 RAII。
- **异常安全**：`audio_thread` 将被包裹在与 `sensor_thread` 相同的 `try-catch` 和 `std::condition_variable` 关闭逻辑中，以确保在收到 `SIGINT` 信号时实现优雅退出 (Graceful Shutdown)。

## 5. 测试策略
- 在 Catch2 中创建 `test_audio.cpp` 和 `test_coap.cpp`。
- 验证 `AudioInput` 能否成功切换到 Mock 模式并生成有效的 PCM 向量（模拟正弦波）。
- 验证 `CoapClient` 在提供无效 URI 时不会崩溃，并通过回退到 Mock 模式来安全处理。