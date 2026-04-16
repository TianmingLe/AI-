#pragma once

#include "../core/ConfigManager.h"
#include "GpsSensor.h"
#include "ImuSensor.h"
#include "KalmanFilter1D.h"

namespace perception {

class SensorFusion {
public:
    explicit SensorFusion(core::ConfigManager& config);

    ImuData fuseImu(const ImuData& raw);
    GpsData fuseGps(const GpsData& raw);

private:
    double imu_q_;
    double imu_r_;
    double gps_q_;
    double gps_r_;

    bool imu_initialized_;
    bool gps_initialized_;

    KalmanFilter1D kf_pitch_;
    KalmanFilter1D kf_yaw_;
    KalmanFilter1D kf_roll_;
    KalmanFilter1D kf_lat_;
    KalmanFilter1D kf_lon_;
};

} // namespace perception
