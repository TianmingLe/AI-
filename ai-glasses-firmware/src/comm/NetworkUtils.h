#pragma once

#include <arpa/inet.h>
#include <ifaddrs.h>
#include <netinet/in.h>

#include <string>

namespace comm {

class NetworkUtils {
public:
    static std::string getLocalIP(const std::string& preferred_interface) {
        std::string fallback;
        std::string preferred;

        ifaddrs* ifaddr = nullptr;
        if (getifaddrs(&ifaddr) != 0) {
            return "127.0.0.1";
        }

        for (ifaddrs* ifa = ifaddr; ifa != nullptr; ifa = ifa->ifa_next) {
            if (!ifa->ifa_addr) continue;
            if (ifa->ifa_addr->sa_family != AF_INET) continue;

            const auto* addr = reinterpret_cast<const sockaddr_in*>(ifa->ifa_addr);
            std::string ip = inet_ntoa(addr->sin_addr);
            std::string name = ifa->ifa_name ? ifa->ifa_name : "";

            if (name == "lo") continue;
            if (fallback.empty()) fallback = ip;
            if (name == preferred_interface) {
                preferred = ip;
                break;
            }
        }

        freeifaddrs(ifaddr);

        if (!preferred.empty()) return preferred;
        if (!fallback.empty()) return fallback;
        return "127.0.0.1";
    }
};

} // namespace comm
