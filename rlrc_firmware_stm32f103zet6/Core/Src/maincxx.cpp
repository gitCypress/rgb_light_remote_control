#include "maincxx.hpp"
#include <cstdio>
#include <array> // (如果 uart_buffer 在这里定义)
#include "ws2812b.hpp"
#include "uart_receiver.hpp"
#include "CommandParser.hpp"

// --- C 语言回调跳板 ---
extern "C" void ws2812b_dma_complete_callback() {
    WS2812B::getInstance().on_dma_transfer_complete();
}
extern "C" void usart3_irq_trampoline() {
    UART_Receiver::getInstance().uart_irq_handler();
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
                // 这里我们使用 CommandParser 定义的常量，代码可读性更高
                // 但为了分流日志(0xFE)，我们还是得先看一眼 header
                switch (uint8_t header = uart_buffer[0]) {
                    // --- 指令类 ---
                    case CommandParser::CMD_SET_PIXEL:
                    case CommandParser::CMD_SET_FRAME:
                    case CommandParser::CMD_TOGGLE:
                    {
                        printf("[ESP->BIN] Received %d bytes: [", len);
                        for(uint16_t i = 0; i < len; ++i) printf("%02X ", uart_buffer[i]);
                        printf("]\r\n");

                        // !!! 移交权力 !!!
                        // 直接把数据甩给解析器，maincxx 不再关心具体是哪个指令
                        CommandParser::parse(uart_buffer.data(), len);
                        break;
                    }

                        // --- 日志类 ---
                    case 0xFE:
                    {
                        if (len > 1) {
                            if (len < UART_Receiver::PACKET_BUFFER_SIZE) uart_buffer[len] = '\0';
                            else uart_buffer[UART_Receiver::PACKET_BUFFER_SIZE - 1] = '\0';

                            printf("[ESP->LOG] %s", (char*)&uart_buffer[1]);
                            if (uart_buffer[len-1] != '\n') printf("\r\n");
                        }
                        break;
                    }

                    default:
                        printf("[ERROR] Unknown Packet: 0x%02X\r\n", header);
                        break;
                }
            }
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
