#include "WebServer.h"

#include "../core/LogManager.h"

namespace comm {

WebServer::WebServer(int port) : port_(port), running_(false) {}

bool WebServer::start() {
    running_ = true;
    LOG_INFO("WebServer: started (mock) on port " + std::to_string(port_));
    return true;
}

void WebServer::stop() {
    if (!running_) return;
    running_ = false;
    LOG_INFO("WebServer: stopped");
}

int WebServer::port() const {
    return port_;
}

} // namespace comm
