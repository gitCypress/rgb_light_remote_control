#include "maincxx.hpp"
#include <cstdio>
#include <array> // (如果 uart_buffer 在这里定义)
#include "ws2812b.hpp"
#include "uart_receiver.hpp"

// --- C 语言回调跳板 ---
extern "C" void ws2812b_dma_complete_callback() {
    WS2812B::getInstance().on_dma_transfer_complete();
}
extern "C" void usart3_irq_trampoline() {
    UART_Receiver::getInstance().uart_irq_handler();
}

// --- 真正的大脑：指令解析器 ---
void parseAndExecuteCommand(const uint8_t* buffer, const uint16_t len) {
    // 忽略空包
    if (len < 1) return;

    auto& led_driver = WS2812B::getInstance();

    switch (buffer[0]) {
        // [CMD(0x01)] [X] [Y] [R] [G] [B]
        case 0x01: {
            if (len >= 6) {
                uint8_t x = buffer[1];
                uint8_t y = buffer[2];
                uint8_t r = buffer[3];
                uint8_t g = buffer[4];
                uint8_t b = buffer[5];
                led_driver.setPixel(x, y, r, g, b);
                led_driver.render(); // 收到像素指令，立即渲染
            }
            break;
        }

        // [CMD(0x03)] [STATE(0x00/0x01)]
        case 0x03: {
            if (len >= 2) {
                bool isOn = (buffer[1] == 0x01);
                if (isOn) {
                    led_driver.setAll(64, 64, 64); // "全开" = 弱白色
                } else {
                    led_driver.clear(); // "全关"
                }
                led_driver.render(); // 收到开关指令，立即渲染
            }
            break;
        }

        // 可以在这里添加 case 0x02: (全屏刷新)

        default:
            // 未知指令
            break;
    }
}

// --- 主程序入口 ---
void maincxx() {
    auto& led_driver = WS2812B::getInstance();
    auto& uart_receiver = UART_Receiver::getInstance();

    // 准备一个缓冲区
    std::array<uint8_t, UART_Receiver::PACKET_BUFFER_SIZE> uart_buffer{};

    // --- 启动系统 ---
    uart_receiver.init();
    printf("[INFO] Application started.\r\n");
    printf("[INFO] UART3 Receiver Initialized.\r\n");

    // --- 主循环 (非阻塞) ---
    while (true) {

        // --- 1. (非阻塞) 检查 UART 包 ---
        if (uart_receiver.isPacketReady()) {
            const uint16_t len = uart_receiver.getPacket(uart_buffer.data());

            if (len > 0) {
                // (智能打印逻辑保持不变，用于调试)
                bool is_text = true;
                for(uint16_t i = 0; i < len; ++i) {
                    uint8_t c = uart_buffer[i];
                    if (c == 0x00 || (c < 32 && c != '\n' && c != '\r' && c != '\t')) {
                        is_text = false;
                        break;
                    }
                }

                if (is_text) {
                    if (len < UART_Receiver::PACKET_BUFFER_SIZE) uart_buffer[len] = '\0';
                    else uart_buffer[UART_Receiver::PACKET_BUFFER_SIZE - 1] = '\0';
                    printf("[ESP->TXT] %s", (char*)uart_buffer.data());
                    if (uart_buffer[len-1] != '\n') printf("\r\n");
                } else {
                    printf("[ESP->BIN] Received %d bytes: [", len);
                    for(uint16_t i = 0; i < len; ++i) printf("%02X ", uart_buffer[i]);
                    printf("]\r\n");

                    // *** 关键：解析并执行二进制包 ***
                    parseAndExecuteCommand(uart_buffer.data(), len);
                }
            }
        }

        // --- 2. (非阻塞) 检查 LED 驱动错误 ---
        // (我们的错误检查逻辑保持不变)
        switch (led_driver.getLastError()) {
            case WS2812B::ErrorCode::NONE:
                break;
            case WS2812B::ErrorCode::RENDER_BUSY:
                printf("[ERROR] WS2812B: Render called too fast!\r\n");
                break;
            case WS2812B::ErrorCode::HAL_START_FAILED:
                printf("[ERROR] WS2812B: HAL DMA Start Failed!\r\n");
                break;
            case WS2812B::ErrorCode::INVALID_COORDS:
                printf("[ERROR] WS2812B: Invalid SetPixel Coords!\r\n");
                break;
        }

        // --- 3. (非阻塞) 其他主循环任务 ---
        // (例如，如果需要，可以在这里运行本地动画)
        // (!! HAL_Delay() 已被移除 !!)
    }
}

void testLights() {
    auto &led_driver = WS2812B::getInstance();

    // 设置 (0,0) 为红色
    led_driver.clear(); // 先清屏
    led_driver.setPixel(0, 0, 128, 0, 0); // 128 亮度
    led_driver.render(); // 渲染！
    HAL_Delay(500); // 等待 500ms

    // 设置 (0,1) 为绿色
    led_driver.clear();
    led_driver.setPixel(0, 1, 0, 128, 0);
    led_driver.render();
    HAL_Delay(500);

    // 设置 (1,0) 为蓝色
    led_driver.clear();
    led_driver.setPixel(1, 0, 0, 0, 128);
    led_driver.render();
    HAL_Delay(500);
}
