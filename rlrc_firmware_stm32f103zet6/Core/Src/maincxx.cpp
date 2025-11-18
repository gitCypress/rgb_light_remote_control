#include "maincxx.hpp"
#include <array>
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

    // 启动 UART
    auto &uart_receiver = UART_Receiver::getInstance();
    uart_receiver.init();
    printf("[INFO] UART3 Receiver Initialized.\r\n");

    while (true) {
        auto phErr = ProtocolHandler::ErrorCode::OK;

        // 检查 UART 包
        uart_receiver.handlePacket([&](auto packet) {
            phErr = ProtocolHandler::dispatch(packet);
        });

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
