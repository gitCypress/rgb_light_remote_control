#include "maincxx.hpp"
#include <array>
#include <cmath>
#include <cstdio>
#include <cstring>

#include "ProtocolHandler.hpp"
#include "esp8266.hpp"
#include "uart_receiver.hpp"
#include "usart.h"
#include "ws2812b.hpp"

extern "C" void ws2812b_dma_complete_callback() { WS2812B::getInstance().on_dma_transfer_complete(); }

void updateDiffusionAnimation(uint32_t timestamp);
void hsv2rgb(uint8_t h, uint8_t s, uint8_t v, uint8_t &r, uint8_t &g, uint8_t &b);


// --- 主程序入口 ---
void maincxx() {
    HAL_Delay(3000);
    printf("[INFO] Application started.\r\n");

    // 启动 ESP8266
    esp8266::enable();
    printf("[INFO] ESP8266 enabled.\r\n");
    HAL_Delay(1000);

    // 启动 UART
    auto &uart_receiver = UART_Receiver::getInstance();
    uart_receiver.init();
    // __HAL_UART_CLEAR_OREFLAG(&huart3); // 清除溢出错误
    // __HAL_UART_CLEAR_NEFLAG(&huart3);  // 清除噪声错误
    // __HAL_UART_CLEAR_FEFLAG(&huart3);  // 清除帧错误
    // __HAL_UART_CLEAR_IDLEFLAG(&huart3);// 清除空闲标志

    // 存储 uart_receiver 获得的包
    constexpr uint16_t scratchBufferSize = 4096;
    std::array<uint8_t, scratchBufferSize> scratchBuffer{};

    while (true) {
        // 从接收器获取一个完整的包
        auto packet = uart_receiver.tryGetPacket(scratchBuffer);

        // 包不为空时，分发处理
        if (!packet.empty()) {
            switch (ProtocolHandler::dispatch(packet)) {
                case ProtocolHandler::ErrorCode::OK:
                    break;
                case ProtocolHandler::ErrorCode::INVALID_BUFFER_LENGTH:
                    printf("Invalid buffer length.");
                    break;
                case ProtocolHandler::ErrorCode::UNKNOWN_COMMAND:
                    printf("Unknown command.");
                    break;
            }
        }

        // 2. 驱动动画逻辑
        if (ProtocolHandler::g_currentMode == 1) { // 假设 1 是扩散动画模式
            updateDiffusionAnimation(HAL_GetTick());
        }
    }
}

// --- 辅助函数：HSV 转 RGB ---
// 用于生成平滑的彩虹色
// h: 0-255, s: 0-255, v: 0-255
void hsv2rgb(uint8_t h, uint8_t s, uint8_t v, uint8_t &r, uint8_t &g, uint8_t &b) {
    unsigned char region, remainder, p, q, t;

    if (s == 0) { r = v; g = v; b = v; return; }

    region = h / 43;
    remainder = (h - (region * 43)) * 6;

    p = (v * (255 - s)) >> 8;
    q = (v * (255 - ((s * remainder) >> 8))) >> 8;
    t = (v * (255 - ((s * (255 - remainder)) >> 8))) >> 8;

    switch (region) {
        case 0: r = v; g = t; b = p; break;
        case 1: r = q; g = v; b = p; break;
        case 2: r = p; g = v; b = t; break;
        case 3: r = p; g = q; b = v; break;
        case 4: r = t; g = p; b = v; break;
        default: r = v; g = p; b = q; break;
    }
}

// --- 动画函数：彩色扩散 ---
void updateDiffusionAnimation(uint32_t timestamp) {
    // 限制帧率：每 50ms 更新一次 (20 FPS)
    static uint32_t last_frame_time = 0;
    if (timestamp - last_frame_time < 50) return;
    last_frame_time = timestamp;

    auto& led = WS2812B::getInstance();

    // 动画参数
    float center_x = 2.0f;
    float center_y = 2.0f;
    // 时间因子：让波纹随时间向外移动 (除以 200 控制速度)
    float time_phase = timestamp / 200.0f;

    for (int y = 0; y < 5; y++) {
        for (int x = 0; x < 5; x++) {
            // 1. 计算当前像素到中心的距离
            float dist = std::sqrt(std::pow(x - center_x, 2) + std::pow(y - center_y, 2));

            // 2. 计算波浪值 (sin波)
            // (dist - time_phase) 实现了向外移动的效果
            float wave = std::sin(dist - time_phase);

            // 3. 根据波浪值计算颜色
            // 我们只在波峰附近点亮 (wave > 0.5)
            if (wave > 0.0f) { // 稍微宽一点的波段
                // 颜色随时间变化 (彩虹旋转)
                uint8_t hue = (uint8_t)((timestamp / 10) % 255);
                // 亮度随波浪高度衰减
                uint8_t val = (uint8_t)(wave * 255);

                uint8_t r, g, b;
                hsv2rgb(hue, 255, val, r, g, b);
                led.setPixel(x, y, r, g, b);
            } else {
                // 波谷位置熄灭
                led.setPixel(x, y, 0, 0, 0);
            }
        }
    }
    led.render();
}
