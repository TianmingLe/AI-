# Audio Input (Microphone Array) & CoAP Protocol Integration Design

## 1. Overview
This specification details the architecture for adding two new high-performance, production-ready components to the AI Glasses Firmware:
1. **New Hardware**: A Microphone Array for Audio Input (`perception::AudioInput`).
2. **Network Protocol**: A CoAP (Constrained Application Protocol) Client (`comm::CoapClient`).

Consistent with the rest of the codebase, both components will rely on dynamic library loading (`dlopen`) to ensure extreme portability and will gracefully degrade to Mock mode if the underlying hardware or OS libraries are unavailable. All changes will adhere to the strict thread-safety and zero-copy principles established in previous refactors.

## 2. Architecture & Responsibilities

### 2.1 `perception::AudioInput` (Microphone Array)
- **Purpose**: Capture raw PCM audio data (e.g., 16kHz, 16-bit Mono) from the hardware microphone via ALSA (Advanced Linux Sound Architecture). This paves the way for future Wake-Word detection or Voice Command recognition.
- **Interface**:
  - `bool init(const std::string& alsa_device)`: Opens the PCM capture device (e.g., `"default"` or `"hw:0,0"`).
  - `std::vector<int16_t> readAudioChunk(size_t frame_count)`: Reads a chunk of audio frames.
- **Implementation Strategy**:
  - Dynamically loads `libasound.so.2` using `dlopen`.
  - Uses `snd_pcm_open`, `snd_pcm_set_params`, and `snd_pcm_readi` functions.
  - **Mock Fallback**: If ALSA fails to load or no soundcard is present, it generates a synthetic 440Hz sine wave (simulated beep) as the audio chunk.
- **Integration with Threads**:
  - A new dedicated `audio_thread` will be spawned in `main.cpp` to continuously pull audio chunks (e.g., every 100ms) without blocking the perception or sensor threads.
  - Pushes audio amplitude/RMS or raw data events to `EventBus` under topic `sensor/audio`.

### 2.2 `comm::CoapClient` (CoAP Protocol)
- **Purpose**: A lightweight, UDP-based IoT protocol to report low-bandwidth telemetry (like battery life, basic IMU stats, or audio RMS) to a backend server efficiently.
- **Interface**:
  - `bool connect(const std::string& coap_uri)`: Resolves the server address and prepares the UDP context.
  - `bool sendTelemetry(const std::string& path, const std::string& payload)`: Constructs a CoAP Non-Confirmable (NON) POST request and sends it.
- **Implementation Strategy**:
  - Dynamically loads `libcoap-3.so` or `libcoap-2.so`.
  - Uses functions like `coap_new_context`, `coap_new_pdu`, and `coap_send`.
  - **Mock Fallback**: If the library is missing or the network is unreachable, it logs the CoAP payload to `LOG_DEBUG` and immediately returns success.
- **Integration with `EventBus`**:
  - Subscribes to `sensor/audio` and `sensor/imu` events, then fires off lightweight CoAP packets to `coap://backend-server/telemetry`.

## 3. Data Flow

1. **Audio Capture**: `audio_thread` reads PCM chunks via ALSA (`readAudioChunk`).
2. **Event Publishing**: `audio_thread` calculates simple RMS (Root Mean Square) to detect noise levels, constructs a JSON `{"rms": 0.45}`, and publishes to `sensor/audio`.
3. **CoAP Telemetry**: The `CoapClient` receives the RMS event via `EventBus` and fires a UDP packet to the CoAP server.

## 4. Safety & Memory Management
- **ALSA Pointers**: ALSA context pointers (`snd_pcm_t*`) will be managed via `std::unique_ptr` with custom deleters calling `snd_pcm_close` to prevent descriptor leaks.
- **CoAP Context**: Similarly, `coap_context_t*` and `coap_session_t*` will be RAII-wrapped.
- **Exception Safety**: The `audio_thread` will be wrapped in the same `try-catch` and `std::condition_variable` shutdown logic as `sensor_thread` to ensure Graceful Shutdown on `SIGINT`.

## 5. Testing Strategy
- Create `test_audio.cpp` and `test_coap.cpp` in Catch2.
- Verify that `AudioInput` successfully switches to Mock mode and produces valid PCM vectors (simulated sine waves).
- Verify that `CoapClient` does not crash and safely handles invalid URIs by falling back to mock mode.