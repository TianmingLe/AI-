#pragma once

#include <string>

namespace comm {

class WebServer {
public:
    explicit WebServer(int port);
    bool start();
    void stop();
    int port() const;

private:
    int port_;
    bool running_;
};

} // namespace comm
