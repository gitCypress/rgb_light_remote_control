//
// Created by 27301 on 2025/11/18.
//

#include "CommandParser.hpp"

#include <cstdio>

#include "ws2812b.hpp"

namespace CommandParser {
    ErrorCode parse(const uint8_t *buffer, const uint16_t len) {
        if (len < 1)
            return ErrorCode::INVALID_BUFFER_LENGTH;

        auto &led_driver = WS2812B::getInstance();

        switch (buffer[0]) {
            case CMD_SET_PIXEL: { // [CMD(0x01)] [X] [Y] [R] [G] [B]
                if (len >= 6) {
                    const uint8_t x = buffer[1];
                    const uint8_t y = buffer[2];
                    const uint8_t r = buffer[3];
                    const uint8_t g = buffer[4];
                    const uint8_t b = buffer[5];

                    led_driver.setPixel(x, y, r, g, b);
                    led_driver.render();
                } else
                    return ErrorCode::INVALID_BUFFER_LENGTH;
                break;
            }

            case CMD_TOGGLE: { // [CMD(0x03)] [STATE]
                if (len >= 2) {
                    const bool isOn = (buffer[1] == 0x01);
                    if (isOn) {
                        // 全开 (弱白光，防止电流过大)
                        led_driver.setAll(64, 64, 64);
                    } else {
                        led_driver.clear();
                    }
                    led_driver.render();
                } else
                    return ErrorCode::INVALID_BUFFER_LENGTH;
                break;
            }

            case CMD_SET_FRAME: // CMD_SET_FRAME (0x02)
                printf("unimplementate: CommandParser::parse::switch case - CMD_SET_FRAME");
                return ErrorCode::UNKNOWN_COMMAND;

            default:
                return ErrorCode::UNKNOWN_COMMAND;
        }
    }
} // namespace CommandParser
