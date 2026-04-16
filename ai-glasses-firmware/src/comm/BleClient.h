#pragma once

#include <string>

namespace comm {

class BleClient {
public:
    explicit BleClient(const std::string& device = "mock");
    bool connect();
    std::string readSensorData();
    bool isUsingMock() const;

private:
    std::string device_;
    bool use_mock_;
};

} // namespace comm
