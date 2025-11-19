//
// Created by 27301 on 2025/11/18.
//

#pragma once
#include <cstdint>

namespace config {
    // TCP server
    constexpr auto tcpPort = 8080;

    // Serial
    constexpr auto serialBaudRate = 115200;

    // WifiManager
    constexpr auto wmPortalTimeout = 180;
} // namespace config
