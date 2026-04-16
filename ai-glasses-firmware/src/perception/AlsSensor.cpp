#include "AlsSensor.h"

#include "../core/LogManager.h"

namespace perception {

AlsSensor::AlsSensor(const std::string&) : use_mock_(true), lux_(300.0f) {
    LOG_INFO("AlsSensor: using mock");
}

float AlsSensor::readLux() {
    lux_ += 1.5f;
    if (lux_ > 800.0f) lux_ = 200.0f;
    return lux_;
}

bool AlsSensor::isUsingMock() const {
    return use_mock_;
}

} // namespace perception
