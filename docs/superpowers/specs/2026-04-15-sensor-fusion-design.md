# 传感器数据融合算法设计规范（卡尔曼滤波）

## 1. 概述
在 AI 眼镜中，我们集成了多种传感器（如 IMU、GPS、ALS 环境光、电池电量等）。然而，真实的传感器硬件读数不可避免地伴随着**高频测量噪声**和**瞬时突变**（例如 IMU 数据的抖动、GPS 信号漂移）。
为了提升最终输出的遥测数据质量，本设计引入**一维卡尔曼滤波器 (1D Kalman Filter)**，对各个关键数据流（Pitch, Yaw, Roll, Latitude, Longitude）进行独立的数据平滑和预测。

## 2. 算法设计

### 2.1 卡尔曼滤波基础 (KalmanFilter1D)
对于单个维度的状态 $x$，系统建模为：
- **预测步骤**：
  $$ x_k^- = x_{k-1} $$
  $$ P_k^- = P_{k-1} + Q $$
  其中，$Q$ 为系统过程噪声方差，决定了我们对系统自身模型的信任程度。

- **更新步骤**：
  $$ K_k = \frac{P_k^-}{P_k^- + R} $$
  $$ x_k = x_k^- + K_k (z_k - x_k^-) $$
  $$ P_k = (1 - K_k) P_k^- $$
  其中，$R$ 为测量噪声方差，决定了我们对传感器读数 $z_k$ 的信任程度。

### 2.2 融合策略 (SensorFusion 模块)
- `SensorFusion` 模块持有多个维度的独立滤波器实例。
- **IMU 数据平滑**：分别对 Pitch、Yaw、Roll 进行平滑。配置适中的 $Q$ 和较大的 $R$ 来滤除高频抖动。
- **GPS 轨迹平滑**：分别对 Latitude（纬度）、Longitude（经度）进行平滑。考虑到 GPS 数据更新频率较低且误差较大，设置较大的 $R$ 防止瞬态漂移。

## 3. 实现计划
1. **任务 1**：在 `src/perception/` 下创建 `KalmanFilter1D.h`。由于该类非常简单，我们通过 Header-only 或在 `SensorFusion.cpp` 中定义。
2. **任务 2**：创建 `SensorFusion.h` 和 `SensorFusion.cpp`，暴露 `fuseImu(const ImuData&)` 和 `fuseGps(const GpsData&)` 接口。
3. **任务 3**：在 `main.cpp` 的 `sensor_thread` 中实例化 `SensorFusion` 对象，并在发布数据前调用融合方法获取平滑数据。
4. **任务 4**：编写 `test_sensor_fusion.cpp` 测试卡尔曼滤波器的平滑效果。
