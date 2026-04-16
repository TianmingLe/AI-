#include "BleClient.h"

#include "../core/LogManager.h"

namespace comm {

BleClient::BleClient(const std::string& device) : device_(device), use_mock_(true) {}

bool BleClient::connect() {
    use_mock_ = true;
    LOG_INFO("BleClient: using mock");
    return true;
}

std::string BleClient::readSensorData() {
    if (use_mock_) return "{}";
    return "{}";
}

bool BleClient::isUsingMock() const {
    return use_mock_;
}

} // namespace comm
