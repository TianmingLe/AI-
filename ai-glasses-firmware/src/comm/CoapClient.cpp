#include "CoapClient.h"

#include "../core/LogManager.h"

namespace comm {

CoapClient::CoapClient(const std::string& endpoint) : endpoint_(endpoint), use_mock_(true) {}

bool CoapClient::connect() {
    use_mock_ = true;
    LOG_INFO("CoapClient: using mock, endpoint=" + endpoint_);
    return true;
}

void CoapClient::send(const std::string& path, const std::string& payload) {
    if (use_mock_) {
        LOG_DEBUG("CoapClient [Mock] send " + path + " size=" + std::to_string(payload.size()));
        return;
    }
}

bool CoapClient::isUsingMock() const {
    return use_mock_;
}

} // namespace comm
