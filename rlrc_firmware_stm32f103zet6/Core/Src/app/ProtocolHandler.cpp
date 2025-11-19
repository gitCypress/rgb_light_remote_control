//
// Created by 27301 on 2025/11/18.
//

#include "ProtocolHandler.hpp"

#include <cstdio>

#include "uart_receiver.hpp"
#include "ws2812b.hpp"

namespace ProtocolHandler {

    // 前向声明
    static ErrorCode handleLog(std::span<const uint8_t> packet);
    static ErrorCode handleSetPixel(std::span<const uint8_t> packet);
    static ErrorCode handleSetFrame(std::span<const uint8_t> packet);
    static ErrorCode handleToggle(std::span<const uint8_t> packet);

    // 公共入口
    ErrorCode dispatch(const std::span<const uint8_t> packet) {
        if (packet.empty())
            return ErrorCode::INVALID_BUFFER_LENGTH;

        switch (packet[0]) {
            // 1. 路由到指令处理
            case CMD_SET_PIXEL: {
                return handleSetPixel(packet);
            }

            case CMD_SET_FRAME: {
                return handleSetFrame(packet);
            }

            case CMD_TOGGLE:
                return handleToggle(packet);

            case LOG_MESSAGE:
                return handleLog(packet);

            default:
                // printf("[ERROR] Unknown Header: 0x%02X\r\n", header);
                return ErrorCode::UNKNOWN_COMMAND;
        }
    }

    static ErrorCode handleSetPixel(const std::span<const uint8_t> packet) {
        printf("[ESP->BIN] Cmd: 0x%02X, Len: %d\r\n", packet[0], packet.size());

        auto &led_driver = WS2812B::getInstance();

        if (packet.size() >= 6) {
            led_driver.setPixel(packet[1], packet[2], packet[3], packet[4], packet[5]);
            led_driver.render();
            return ErrorCode::OK;
        }

        return ErrorCode::INVALID_BUFFER_LENGTH;
    }

    // TODO: handleSetFrame 尚待实现
    static ErrorCode handleSetFrame(std::span<const uint8_t> packet) {
        printf("[ESP->BIN] Cmd: 0x%02X, Len: %d\r\n", packet[0], packet.size());
        return ErrorCode::UNKNOWN_COMMAND;
    }

    static ErrorCode handleToggle(std::span<const uint8_t> packet) {
        printf("[ESP->BIN] Cmd: 0x%02X, Len: %d\r\n", packet[0], packet.size());
        auto &led_driver = WS2812B::getInstance();

        if (packet.size() >= 2) {
            switch (packet[1]) {
                case 0x00 : {
                    led_driver.clear();
                    break;

                }
                default:
                    led_driver.setAll(64, 64, 64);
                    break;
            }
            led_driver.render();
            return ErrorCode::OK;
        }

        return ErrorCode::INVALID_BUFFER_LENGTH;
    }

    static ErrorCode handleLog(std::span<const uint8_t> packet) {
        if (packet.size() <= 1)
            return ErrorCode::INVALID_BUFFER_LENGTH;

        char logBuffer[UART_Receiver::PACKET_BUFFER_SIZE];

        // 拷贝内容 (跳过 0xFE 包头)
        const uint16_t msgLen = packet.size() - 1;
        for (int i = 0; i < msgLen; i++) {
            logBuffer[i] = static_cast<char>(packet[i + 1]);
        }
        logBuffer[msgLen] = '\0'; // 确保截断

        printf("[ESP->LOG] %s", logBuffer);

        // 补换行
        if (msgLen > 0 && logBuffer[msgLen - 1] != '\n') {
            printf("\r\n");
        }

        return ErrorCode::OK;
    }
} // namespace ProtocolHandler
