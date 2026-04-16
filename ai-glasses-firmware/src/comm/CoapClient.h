#pragma once

#include <string>

namespace comm {

class CoapClient {
public:
    explicit CoapClient(const std::string& endpoint);
    bool connect();
    void send(const std::string& path, const std::string& payload);
    bool isUsingMock() const;

private:
    std::string endpoint_;
    bool use_mock_;
};

} // namespace comm
