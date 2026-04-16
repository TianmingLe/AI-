#include "NtpClient.h"

#include "../core/LogManager.h"

#include <chrono>

namespace comm {

NtpClient::NtpClient(const std::string& server) : server_(server), use_mock_(true) {}

std::optional<long long> NtpClient::syncTime() {
    use_mock_ = true;
    auto now = std::chrono::system_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
    LOG_INFO("NtpClient [Mock]: sync server=" + server_);
    return ms;
}

bool NtpClient::isUsingMock() const {
    return use_mock_;
}

} // namespace comm
