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

    // 存储 uart_receiver 获得的包
    constexpr uint16_t scratchBufferSize = 256;
    std::array<uint8_t, scratchBufferSize> scratchBuffer{};

    while (true) {
        // 从接收器获取一个完整的包
        auto packet = uart_receiver.tryGetPacket(scratchBuffer);

        // 包不为空时，分发处理
        if constexpr (!scratchBuffer.empty()) {
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
    }
}
