#include "MqttClient.h"

#include "../core/LogManager.h"

namespace comm {

MqttClient::MqttClient(const std::string& broker, const std::string& client_id)
    : broker_(broker), client_id_(client_id), use_mock_(true) {}

bool MqttClient::connect() {
    use_mock_ = true;
    LOG_INFO("MqttClient: using mock, broker=" + broker_ + " clientId=" + client_id_);
    return true;
}

void MqttClient::publish(const std::string& topic, const std::string& payload) {
    if (use_mock_) {
        LOG_DEBUG("MqttClient [Mock] publish " + topic + " size=" + std::to_string(payload.size()));
        return;
    }
}

bool MqttClient::isUsingMock() const {
    return use_mock_;
}

} // namespace comm
