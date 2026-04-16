#include "GpsSensor.h"

#include "../core/LogManager.h"

#include <cmath>

namespace perception {

GpsSensor::GpsSensor(const std::string&) : use_mock_(true), mock_lat_(37.7749), mock_lon_(-122.4194), mock_angle_(0.0) {
    LOG_INFO("GpsSensor: using mock");
}

GpsSensor::~GpsSensor() = default;

bool GpsSensor::isUsingMock() const {
    return use_mock_;
}

void GpsSensor::generateMockData() {
    mock_angle_ += 0.05;
    if (mock_angle_ > 6.283185307) mock_angle_ -= 6.283185307;
    double r = 0.001;
    mock_lat_ = 37.7749 + r * std::cos(mock_angle_);
    mock_lon_ = -122.4194 + r * std::sin(mock_angle_);
}

std::optional<GpsData> GpsSensor::readLocation() {
    if (use_mock_) {
        generateMockData();
        return GpsData{mock_lat_, mock_lon_, 15.0, 8};
    }
    return std::nullopt;
}

} // namespace perception
