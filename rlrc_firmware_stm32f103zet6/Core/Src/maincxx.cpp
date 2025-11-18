#include "maincxx.hpp"
#include <array> // (濡傛灉 uart_buffer 鍦ㄨ繖閲屽畾涔?
#include <cstdio>
#include "ProtocolHandler.hpp"
#include "uart_receiver.hpp"
#include "ws2812b.hpp"

// --- C 语言回调跳板 ---
extern "C" void ws2812b_dma_complete_callback() { WS2812B::getInstance().on_dma_transfer_complete(); }
extern "C" void usart3_irq_trampoline() { UART_Receiver::getInstance().uart_irq_handler(); }

// --- 主程序入口 ---
void maincxx() {
    printf("[INFO] Application started.\r\n");

    // 缓冲区
    // TODO: 简化掉这个缓冲区设计
    std::array<uint8_t, UART_Receiver::PACKET_BUFFER_SIZE> uart_buffer{};

    // 启动 UART
    auto &uart_receiver = UART_Receiver::getInstance();
    uart_receiver.init();
    printf("[INFO] UART3 Receiver Initialized.\r\n");

    while (true) {

        auto phErr = ProtocolHandler::ErrorCode::OK;

        // 检查 UART 包
        if (uart_receiver.isPacketReady()) {
            const uint16_t len = uart_receiver.getPacket(uart_buffer.data());
            phErr = ProtocolHandler::dispatch(uart_buffer.data(), len);
        }

        switch (phErr) {
            case ProtocolHandler::ErrorCode::OK:
                break;
            case ProtocolHandler::ErrorCode::INVALID_BUFFER_LENGTH:
                printf("[ERROR] ProtocolHandler-INVALID_BUFFER_LENGTH.");
                break;
            case ProtocolHandler::ErrorCode::UNKNOWN_COMMAND:
                printf("[ERROR] ProtocolHandler-UNKNOWN_COMMAND.");
                break;
        }
    }
}

// TODO: 删掉这个测试函数
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
