#pragma once

#include <string>

namespace comm {

class MqttClient {
public:
    MqttClient(const std::string& broker, const std::string& client_id);
    bool connect();
    void publish(const std::string& topic, const std::string& payload);
    bool isUsingMock() const;

private:
    std::string broker_;
    std::string client_id_;
    bool use_mock_;
};

} // namespace comm
