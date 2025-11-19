#pragma once
#include "main.h"
#include <array>
#include <cstdint>

extern "C" UART_HandleTypeDef huart3;

class UART_Receiver {
public:
    // 缓冲区加大到 1KB 或 2KB，为摄像头数据和突发日志做准备
    static constexpr uint16_t RX_BUFFER_SIZE = 2048;

    static UART_Receiver& getInstance();

    void init();

    /**
     * @brief 检查缓冲区中有多少字节等待处理
     */
    [[nodiscard]] uint16_t available() const;

    /**
     * @brief 从缓冲区读取一个字节 (并推进读指针)
     * @return 读取的字节，如果缓冲区空则返回 0
     */
    uint8_t read();

    /**
     * @brief 查看下一个字节但不取出 (Peek)
     */
    [[nodiscard]] uint8_t peek() const;

private:
    UART_Receiver() = default;
    
    // DMA 直接写入的环形缓冲区
    std::array<uint8_t, RX_BUFFER_SIZE> rx_buffer{};
    
    // 我们的“读”指针 (软件维护)
    // "写"指针由硬件 DMA (CNDTR) 维护
    uint16_t tail_ptr = 0;
};