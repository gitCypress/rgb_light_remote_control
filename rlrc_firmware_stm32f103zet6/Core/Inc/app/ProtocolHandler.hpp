//
// Created by 27301 on 2025/11/18.
//

#pragma once
#include <cstdint>
#include <span>

namespace ProtocolHandler {

    enum class ErrorCode {
        OK,
        INVALID_BUFFER_LENGTH, // Buffer 长度不对
        UNKNOWN_COMMAND,  // 未知指令
    };

    enum PacketType : std::uint8_t {
        CMD_SET_PIXEL = 0x01, // 设置单个像素
        CMD_SET_FRAME = 0x02, // 设置全屏
        CMD_TOGGLE = 0x03, // 开关控制
        LOG_MESSAGE = 0xFE,  // ESP8266 日志
    };

    ErrorCode dispatch(std::span<const uint8_t> packet);
} // namespace CommandParser
