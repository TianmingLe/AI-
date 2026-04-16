#include "OpcUaClient.h"

#include "../core/LogManager.h"

namespace comm {

OpcUaClient::OpcUaClient(const std::string& endpoint) : endpoint_(endpoint), use_mock_(true) {}

bool OpcUaClient::connect() {
    use_mock_ = true;
    LOG_INFO("OpcUaClient: using mock, endpoint=" + endpoint_);
    return true;
}

void OpcUaClient::writeNode(const std::string& node_id, const std::string& value_json) {
    if (use_mock_) {
        LOG_DEBUG("OpcUaClient [Mock] write " + node_id + " size=" + std::to_string(value_json.size()));
        return;
    }
}

bool OpcUaClient::isUsingMock() const {
    return use_mock_;
}

} // namespace comm
