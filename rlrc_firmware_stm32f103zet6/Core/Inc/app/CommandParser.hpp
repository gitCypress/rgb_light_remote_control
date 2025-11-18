//
// Created by 27301 on 2025/11/18.
//

#pragma once
#include <cstdint>

namespace CommandParser {

    enum class ErrorCode {
        INVALID_BUFFER_LENGTH, // Buffer 长度不对
        UNKNOWN_COMMAND,  // 未知指令
    };

    enum CommandID : std::uint8_t {
        CMD_SET_PIXEL = 0x01, // 设置单个像素
        CMD_SET_FRAME = 0x02, // 设置全屏
        CMD_TOGGLE = 0x03, // 开关控制
    };

    /**
     * @brief 解析并执行二进制指令
     * @param buffer 指向数据包负载的指针。为了方便，我们这里传入包含包头的完整 buffer
     * @param len    数据包总长度
     */
    ErrorCode parse(const uint8_t *buffer, uint16_t len);
} // namespace CommandParser
