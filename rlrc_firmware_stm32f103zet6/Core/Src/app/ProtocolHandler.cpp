#include "ProtocolHandler.hpp"
#include "ws2812b.hpp"
#include "uart_receiver.hpp" // 仅为了获取 PACKET_BUFFER_SIZE 常量(如果日志需要)
#include <cstdio>
#include <cstring>

namespace ProtocolHandler {
    // --- 核心分发函数 ---
    ErrorCode dispatch(const std::span<const uint8_t> packet) {
        if (packet.empty()) return ErrorCode::INVALID_BUFFER_LENGTH;

        const auto header = static_cast<PacketType>(packet[0]);

        // payload 是除去 header 剩下的部分
        // 如果 packet 只有一个字节(只有header)，subspan(1) 会返回空视图，这是安全的
        const std::span<const uint8_t> payload = (packet.size() > 1) ? packet.subspan(1) : std::span<const uint8_t>{};

        switch (header) {
            case PacketType::CMD_SET_PIXEL:
                return handleSetPixel(payload);
            case PacketType::CMD_TOGGLE:
                return handleToggle(payload);
            case PacketType::MSG_LOG:
                return handleLog(payload);
            case PacketType::CMD_SET_FRAME:
                return handleSetFrame(payload);
        }

        // 从这里出来说明出现未知指令
        // 不写在 switch 的 default，是为了保持 switch 对 enum class 的封闭性检查
        return ErrorCode::UNKNOWN_COMMAND;
    }

    // --- 具体处理函数 ---
    static ErrorCode handleSetPixel(std::span<const uint8_t> payload) {
        // 需要 5 个字节: X, Y, R, G, B
        if (payload.size() < 5) return ErrorCode::INVALID_BUFFER_LENGTH;

        auto &led = WS2812B::getInstance();
        led.setPixel(payload[0], payload[1], payload[2], payload[3], payload[4]);
        led.render();

        printf("[ESP->BIN] SetPixel (%d,%d)\r\n", payload[0], payload[1]);
        return ErrorCode::OK;
    }

    static ErrorCode handleToggle(std::span<const uint8_t> payload) {
        // 需要 1 个字节
        if (payload.empty()) return ErrorCode::INVALID_BUFFER_LENGTH;

        const bool isOn = (payload[0] == 0x01);
        auto &led = WS2812B::getInstance();
        if (isOn)
            led.setAll(64, 64, 64);
        else
            led.clear();
        led.render();

        printf("[ESP->BIN] Toggle: %s\r\n", isOn ? "ON" : "OFF");
        return ErrorCode::OK;
    }

    static ErrorCode handleLog(const std::span<const uint8_t> payload) {
        if (payload.empty()) return ErrorCode::OK; // 空日志忽略

        // 为了 printf 安全，我们需要一个以 \0 结尾的 buffer
        // 假设最大日志长度不超过 256 (你可以定义一个常量)
        char logBuf[256];
        const size_t len = std::min(payload.size(), static_cast<size_t>(255));

        std::memcpy(logBuf, payload.data(), len);
        logBuf[len] = '\0';

        printf("[ESP->LOG] %s", logBuf);
        // 如果原日志没带换行，补一个
        if (len > 0 && logBuf[len - 1] != '\n') printf("\r\n");

        return ErrorCode::OK;
    }

} // namespace ProtocolHandler
