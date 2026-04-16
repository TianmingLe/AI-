#include "ImuSensor.h"

#include "../core/LogManager.h"

#include <cmath>

namespace perception {

ImuSensor::ImuSensor(const std::string&) : use_mock_(true), tick_(0) {
    LOG_INFO("ImuSensor: using mock");
}

ImuData ImuSensor::readData() {
    tick_++;
    float t = static_cast<float>(tick_) * 0.02f;
    ImuData d{};
    d.pitch = std::sin(t) * 10.0f;
    d.yaw = std::cos(t) * 5.0f;
    d.roll = std::sin(t * 0.5f) * 7.0f;
    return d;
}

bool ImuSensor::isUsingMock() const {
    return use_mock_;
}

} // namespace perception
