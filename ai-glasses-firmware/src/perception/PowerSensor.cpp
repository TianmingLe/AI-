#include "PowerSensor.h"

#include "../core/LogManager.h"

namespace perception {

PowerSensor::PowerSensor(const std::string&) : use_mock_(true), level_(100.0f) {
    LOG_INFO("PowerSensor: using mock");
}

PowerData PowerSensor::readPower() {
    if (level_ > 0.1f) level_ -= 0.01f;
    PowerData d{};
    d.battery_percent = level_;
    d.voltage = 3.7f + (level_ / 100.0f) * 0.5f;
    return d;
}

bool PowerSensor::isUsingMock() const {
    return use_mock_;
}

} // namespace perception
