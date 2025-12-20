// //
// // Created by 27301 on 2025/11/18.
// //

#pragma once
#include <cstdint>
#include <span>

namespace ProtocolHandler {

    enum class ErrorCode {
        OK,
        INVALID_BUFFER_LENGTH,
        UNKNOWN_COMMAND,
    };

    enum class PacketType : std::uint8_t {
        CMD_SET_PIXEL = 0x01,
        CMD_SET_FRAME = 0x02,
        CMD_TOGGLE    = 0x03,
        MSG_LOG   = 0xFE,
    };

    /**
     * @brief 分发处理已解码的数据包
     * @param packet 已经解码好的、干净的数据包 (不含 COBS 0x00, 不含 Overhead 字节)
     */
    ErrorCode dispatch(std::span<const uint8_t> packet);

    static ErrorCode handleLog(std::span<const uint8_t> payload);
    static ErrorCode handleSetPixel(std::span<const uint8_t> payload);
    static ErrorCode handleSetFrame(std::span<const uint8_t> payload);
    static ErrorCode handleToggle(std::span<const uint8_t> payload);
}

