#pragma once

#include <string>

namespace perception {

class AlsSensor {
public:
    explicit AlsSensor(const std::string& device = "mock");
    float readLux();
    bool isUsingMock() const;

private:
    bool use_mock_;
    float lux_;
};

} // namespace perception
