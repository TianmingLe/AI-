#pragma once

#include <mutex>
#include <netinet/in.h>
#include <string>

namespace comm {

class UdpBroadcaster {
public:
    explicit UdpBroadcaster(int port);
    ~UdpBroadcaster();

    UdpBroadcaster(const UdpBroadcaster&) = delete;
    UdpBroadcaster& operator=(const UdpBroadcaster&) = delete;

    bool start();
    void stop();
    void broadcast(const std::string& message);
    bool isUsingMock() const;

private:
    mutable std::mutex mutex_;
    int port_;
    int sock_fd_;
    sockaddr_in broadcast_addr_;
    bool use_mock_;
};

} // namespace comm
