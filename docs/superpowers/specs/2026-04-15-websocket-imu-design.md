# WebSocket/HTTP Server & IMU Sensor Integration Design

## 1. Overview
This specification outlines the architecture and integration strategy for adding two new components to the AI Glasses Firmware:
1. **Network Protocol**: A local WebSocket/HTTP Server (`comm::WebServer`).
2. **New Hardware**: An Inertial Measurement Unit (IMU) Sensor (`perception::ImuSensor`).

Both components will follow the established project patterns of dynamic library loading (or system-level mocks) to ensure compilation and basic execution without hard dependencies in restricted sandbox environments.

## 2. Architecture & Responsibilities

### 2.1 `comm::WebServer` (WebSocket/HTTP Server)
- **Purpose**: Serve a local dashboard for diagnostics, configuration updates, or live telemetry streaming (e.g., sending detection events and raw bounding boxes to a connected browser or mobile app).
- **Interface**:
  - `start(int port)`: Initializes and starts the HTTP/WS listening loop in a background thread.
  - `stop()`: Gracefully shuts down the server.
  - `broadcast(const std::string& message)`: Sends JSON telemetry to all connected WebSocket clients.
- **Implementation Strategy**:
  - Will attempt to dynamically load a standard lightweight C++ HTTP/WS library (e.g., `libwebsockets.so` or `libmicrohttpd.so`).
  - If the library is missing, it will gracefully fall back to a `use_mock_` state, simply logging the broadcasts instead of dropping them.
- **Integration with `EventBus`**:
  - It will subscribe to topics like `perception/detections` and `sensor/imu` and pipe these payloads into the `broadcast` method.

### 2.2 `perception::ImuSensor` (IMU Hardware)
- **Purpose**: Read 6-axis/9-axis data (Pitch, Yaw, Roll, Accelerations) to provide Head Tracking capabilities.
- **Interface**:
  - `bool init(const std::string& i2c_device)`: Opens the I2C device (e.g., `/dev/i2c-1`) and verifies the IMU's WHO_AM_I register.
  - `ImuData readData()`: Returns the latest Euler angles and linear accelerations.
- **Implementation Strategy**:
  - Uses standard Linux I2C ioctls (`<linux/i2c-dev.h>`).
  - If `/dev/i2c-x` does not exist or permission is denied, it switches to a mock mode generating smooth sinusoidal dummy data for testing the downstream pipeline.
- **Integration with `sensor_thread`**:
  - The existing `sensor_thread` (which currently reads BLE) will be expanded to also poll `ImuSensor` at ~100Hz (10ms) and publish `sensor/imu` events to the `EventBus`.

## 3. Data Flow

1. **IMU Polling**: `sensor_thread` reads `perception::ImuSensor::readData()` every 10ms.
2. **Event Publishing**: `sensor_thread` packages the IMU data into JSON and publishes to `sensor/imu`.
3. **WebSocket Broadcast**: The `comm::WebServer` receives the event via `EventBus` and broadcasts it to any connected Web/Mobile clients.

## 4. Error Handling & Edge Cases
- **Port Conflicts**: If the WebServer port (e.g., 8080) is in use, the component will log a `LOG_ERROR` and safely degrade to mock mode.
- **I2C Bus Errors**: If the I2C bus hangs or returns `EIO`, the IMU will automatically log the failure, switch to mock mode, and attempt a soft reset/reconnect on the next cycle.
- **Graceful Shutdown**: Both the WebServer thread and the IMU I2C file descriptors will be cleanly closed during the main thread's `SIGINT` shutdown phase.

## 5. Testing Strategy
- The components will be integrated into the main Catch2 test suite (e.g., testing that the IMU gracefully falls back to mock mode on invalid I2C paths).
- Will be validated by compiling via CMake and running the executable in the sandbox to observe mock logs.