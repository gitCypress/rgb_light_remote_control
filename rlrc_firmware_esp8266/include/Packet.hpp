//
// Created by 27301 on 2025/11/18.
//

#pragma once
#include <HardwareSerial.h>
#include <cstdint>

namespace Packet {
    static constexpr std::uint8_t LOG_HEADER = 0xFE; // 日志包头
} // namespace Packet
