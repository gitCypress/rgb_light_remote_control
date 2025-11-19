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

    while (true) {
        ProtocolHandler::getInstance().update();
    }
}
