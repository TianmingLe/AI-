#pragma once

#include <string>

namespace perception {

struct PowerData {
    float battery_percent;
    float voltage;
};

class PowerSensor {
public:
    explicit PowerSensor(const std::string& device = "mock");
    PowerData readPower();
    bool isUsingMock() const;

private:
    bool use_mock_;
    float level_;
};

} // namespace perception
