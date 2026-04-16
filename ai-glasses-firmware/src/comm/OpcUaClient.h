#pragma once

#include <string>

namespace comm {

class OpcUaClient {
public:
    explicit OpcUaClient(const std::string& endpoint);
    bool connect();
    void writeNode(const std::string& node_id, const std::string& value_json);
    bool isUsingMock() const;

private:
    std::string endpoint_;
    bool use_mock_;
};

} // namespace comm
