//
// Created by 27301 on 2025/11/18.
//

#pragma once
#include <cstdint>
#include <span>

class ProtocolHandler {
public:
    static ProtocolHandler& getInstance();

    ProtocolHandler(const ProtocolHandler&) = delete;
    ProtocolHandler& operator=(const ProtocolHandler&) = delete;

    /**
     * @brief 核心驱动函数
     * 从 UART RingBuffer 读取数据 -> 遇到 0x00 -> COBS 解码 -> 执行
     */
    void update();

private:
    ProtocolHandler() = default;

    void dispatchDecoded(std::span<const uint8_t> packet);

    void handleSetPixel(std::span<const uint8_t> payload);
    void handleToggle(std::span<const uint8_t> payload);
    void handleLog(std::span<const uint8_t> payload);

    // 1. 接收缓冲区 (存放带有 COBS 编码的原始数据)
    // 256 字节足够容纳日志和指令
    std::array<uint8_t, 256> _encodedBuffer{};
    size_t _encodedIndex = 0;

    // 2. 解码缓冲区 (存放还原后的数据)
    std::array<uint8_t, 256> _decodedBuffer{};
};

