#pragma once

#include <cstdint>
#include <string>

namespace perception {

struct ImuData {
    float pitch;
    float yaw;
    float roll;
};

class ImuSensor {
public:
    explicit ImuSensor(const std::string& device = "mock");
    ImuData readData();
    bool isUsingMock() const;

private:
    bool use_mock_;
    uint64_t tick_;
};

} // namespace perception
