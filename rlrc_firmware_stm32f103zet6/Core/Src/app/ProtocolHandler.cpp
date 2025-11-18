//
// Created by 27301 on 2025/11/18.
//

#include "ProtocolHandler.hpp"

#include <cstdio>

#include "uart_receiver.hpp"
#include "ws2812b.hpp"

namespace ProtocolHandler {

    // 前向声明
    static ErrorCode handleLog(const uint8_t *buffer, uint16_t len);
    static ErrorCode handleSetPixel(const uint8_t *buffer, uint16_t len);
    static ErrorCode handleSetFrame(const uint8_t *buffer, uint16_t len);
    static ErrorCode handleToggle(const uint8_t *buffer, uint16_t len);

    // 公共入口
    ErrorCode dispatch(const uint8_t *buffer, uint16_t len) {
        if (len < 1)
            return ErrorCode::INVALID_BUFFER_LENGTH;

        switch (buffer[0]) {
            // 1. 路由到指令处理
            case CMD_SET_PIXEL: {
                return handleSetPixel(buffer, len);
            }

            case CMD_SET_FRAME: {
                return handleSetFrame(buffer, len);
            }

            case CMD_TOGGLE:
                return handleToggle(buffer, len);

            case LOG_MESSAGE:
                return handleLog(buffer, len);

            default:
                // printf("[ERROR] Unknown Header: 0x%02X\r\n", header);
                return ErrorCode::UNKNOWN_COMMAND;
        }
    }

    static ErrorCode handleSetPixel(const uint8_t *buffer, const uint16_t len) {
        printf("[ESP->BIN] Cmd: 0x%02X, Len: %d\r\n", buffer[0], len);

        auto &led_driver = WS2812B::getInstance();

        if (len >= 6) {
            led_driver.setPixel(buffer[1], buffer[2], buffer[3], buffer[4], buffer[5]);
            led_driver.render();
            return ErrorCode::OK;
        }

        return ErrorCode::INVALID_BUFFER_LENGTH;
    }

    // TODO: handleSetFrame 尚待实现
    static ErrorCode handleSetFrame(const uint8_t *buffer, const uint16_t len) {
        printf("[ESP->BIN] Cmd: 0x%02X, Len: %d\r\n", buffer[0], len);
        return ErrorCode::UNKNOWN_COMMAND;
    }

    static ErrorCode handleToggle(const uint8_t *buffer, const uint16_t len) {
        printf("[ESP->BIN] Cmd: 0x%02X, Len: %d\r\n", buffer[0], len);
        auto &led_driver = WS2812B::getInstance();

        if (len >= 2) {
            switch (buffer[1]) {
                case 0x00 : {
                    led_driver.clear();
                    break;

                }

                case 0x01: {
                    led_driver.setAll(64, 64, 64);
                    break;
                }

                default:
                    // TODO: Toogle 只处理 0x00 和 0x01 不太合理
                    break;
            }
            led_driver.render();
            return ErrorCode::OK;
        }

        return ErrorCode::INVALID_BUFFER_LENGTH;
    }

    static ErrorCode handleLog(const uint8_t *buffer, uint16_t len) {
        if (len <= 1)
            return ErrorCode::INVALID_BUFFER_LENGTH;

        char logBuffer[UART_Receiver::PACKET_BUFFER_SIZE];

        // 拷贝内容 (跳过 0xFE 包头)
        const uint16_t msgLen = len - 1;
        for (int i = 0; i < msgLen; i++) {
            logBuffer[i] = static_cast<char>(buffer[i + 1]);
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
