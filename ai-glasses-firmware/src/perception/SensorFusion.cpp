#include "SensorFusion.h"

#include "../core/LogManager.h"

namespace perception {

SensorFusion::SensorFusion(core::ConfigManager& config)
    : imu_q_(config.getFloat("kf_imu_q").value_or(0.01)),
      imu_r_(config.getFloat("kf_imu_r").value_or(0.1)),
      gps_q_(config.getFloat("kf_gps_q").value_or(1e-5)),
      gps_r_(config.getFloat("kf_gps_r").value_or(1e-3)),
      imu_initialized_(false),
      gps_initialized_(false),
      kf_pitch_(imu_q_, imu_r_, 1.0, 0.0),
      kf_yaw_(imu_q_, imu_r_, 1.0, 0.0),
      kf_roll_(imu_q_, imu_r_, 1.0, 0.0),
      kf_lat_(gps_q_, gps_r_, 1.0, 0.0),
      kf_lon_(gps_q_, gps_r_, 1.0, 0.0) {
    LOG_INFO("SensorFusion: init");
}

ImuData SensorFusion::fuseImu(const ImuData& raw) {
    if (!imu_initialized_) {
        kf_pitch_.reset(imu_q_, imu_r_, 1.0, raw.pitch);
        kf_yaw_.reset(imu_q_, imu_r_, 1.0, raw.yaw);
        kf_roll_.reset(imu_q_, imu_r_, 1.0, raw.roll);
        imu_initialized_ = true;
        return raw;
    }

    ImuData out{};
    out.pitch = static_cast<float>(kf_pitch_.update(raw.pitch));
    out.yaw = static_cast<float>(kf_yaw_.update(raw.yaw));
    out.roll = static_cast<float>(kf_roll_.update(raw.roll));
    return out;
}

GpsData SensorFusion::fuseGps(const GpsData& raw) {
    if (!gps_initialized_) {
        kf_lat_.reset(gps_q_, gps_r_, 1.0, raw.latitude);
        kf_lon_.reset(gps_q_, gps_r_, 1.0, raw.longitude);
        gps_initialized_ = true;
        return raw;
    }

    GpsData out = raw;
    out.latitude = kf_lat_.update(raw.latitude);
    out.longitude = kf_lon_.update(raw.longitude);
    return out;
}

} // namespace perception
