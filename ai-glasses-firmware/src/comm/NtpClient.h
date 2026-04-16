#pragma once

#include <optional>
#include <string>

namespace comm {

class NtpClient {
public:
    explicit NtpClient(const std::string& server = "pool.ntp.org");
    std::optional<long long> syncTime();
    bool isUsingMock() const;

private:
    std::string server_;
    bool use_mock_;
};

} // namespace comm
