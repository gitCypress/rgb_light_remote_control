#include "maincxx.hpp"
#include <array>
#include <cstdio>
#include "ProtocolHandler.hpp"
#include "esp8266.hpp"
#include "uart_receiver.hpp"
#include "ws2812b.hpp"

extern "C" void ws2812b_dma_complete_callback() { WS2812B::getInstance().on_dma_transfer_complete(); }

// --- 主程序入口 ---
void maincxx() {
    printf("[INFO] Application started.\r\n");

    // 启动 UART
    auto &uart_receiver = UART_Receiver::getInstance();
    uart_receiver.init();
    printf("[INFO] UART3 Receiver Initialized.\r\n");

    // 启动 ESP8266
    HAL_Delay(10000);
    esp8266::enable();
    printf("[INFO] ESP8266 enabled.\r\n");

    while (true) {
        ProtocolHandler::getInstance().update();
    }
}
