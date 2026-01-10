//
// Created by 27301 on 2025/11/18.
//

#pragma once
#include <cstdint>

namespace config {
    // TCP server
    constexpr auto tcpPort = 8080;

    // udp device discovery service
    constexpr auto udpPort = 8888;
    const String discoveryMsg = "WHO_IS_RLRC?";

    // Serial
    constexpr auto serialBaudRate = 115200;

    // WifiManager
    constexpr auto wmPortalTimeout = 180;
} // namespace config
