#include "UdpBroadcaster.h"

#include "../core/LogManager.h"

#include <arpa/inet.h>
#include <cerrno>
#include <cstring>
#include <sys/socket.h>
#include <unistd.h>

namespace comm {

UdpBroadcaster::UdpBroadcaster(int port)
    : port_(port), sock_fd_(-1), broadcast_addr_{}, use_mock_(false) {}

UdpBroadcaster::~UdpBroadcaster() {
    stop();
}

bool UdpBroadcaster::isUsingMock() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return use_mock_;
}

bool UdpBroadcaster::start() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (sock_fd_ >= 0) {
        close(sock_fd_);
        sock_fd_ = -1;
    }

    sock_fd_ = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock_fd_ < 0) {
        use_mock_ = true;
        LOG_WARN(std::string("UdpBroadcaster: socket() failed, using mock err=") + std::strerror(errno));
        return true;
    }

    int enable = 1;
    if (setsockopt(sock_fd_, SOL_SOCKET, SO_BROADCAST, &enable, sizeof(enable)) < 0) {
        close(sock_fd_);
        sock_fd_ = -1;
        use_mock_ = true;
        LOG_WARN(std::string("UdpBroadcaster: setsockopt(SO_BROADCAST) failed, using mock err=") + std::strerror(errno));
        return true;
    }

    std::memset(&broadcast_addr_, 0, sizeof(broadcast_addr_));
    broadcast_addr_.sin_family = AF_INET;
    broadcast_addr_.sin_port = htons(port_);
    broadcast_addr_.sin_addr.s_addr = htonl(INADDR_BROADCAST);

    use_mock_ = false;
    LOG_INFO("UdpBroadcaster: started on port " + std::to_string(port_));
    return true;
}

void UdpBroadcaster::stop() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (sock_fd_ >= 0) {
        close(sock_fd_);
        sock_fd_ = -1;
    }
}

void UdpBroadcaster::broadcast(const std::string& message) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (use_mock_ || sock_fd_ < 0) {
        LOG_DEBUG("UdpBroadcaster [Mock]: " + message);
        return;
    }

    ssize_t sent = sendto(
        sock_fd_,
        message.c_str(),
        message.size(),
        0,
        reinterpret_cast<sockaddr*>(&broadcast_addr_),
        sizeof(broadcast_addr_)
    );
    if (sent < 0) {
        LOG_WARN(std::string("UdpBroadcaster: sendto failed err=") + std::strerror(errno));
    }
}

} // namespace comm
