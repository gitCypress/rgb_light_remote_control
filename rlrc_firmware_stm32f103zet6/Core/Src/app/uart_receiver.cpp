#include "uart_receiver.hpp"

UART_Receiver& UART_Receiver::getInstance() {
    static UART_Receiver instance;
    return instance;
}

void UART_Receiver::init() {
    // 只需要启动 DMA，不需要 IDLE 中断了！
    // 硬件会自动在缓冲区里循环写入
    HAL_UART_Receive_DMA(&huart3, rx_buffer.data(), RX_BUFFER_SIZE);
}

uint16_t UART_Receiver::available() const {
    // 计算 DMA 写到了哪里 (Head)
    // __HAL_DMA_GET_COUNTER 返回的是"剩余空间"，所以要用总大小减去
    const uint16_t head_ptr = RX_BUFFER_SIZE - __HAL_DMA_GET_COUNTER(huart3.hdmarx);

    if (head_ptr >= tail_ptr) {
        return head_ptr - tail_ptr;
    } else {
        // 发生了回环 (Wrap around)
        return (RX_BUFFER_SIZE - tail_ptr) + head_ptr;
    }
}

uint8_t UART_Receiver::readCobs() {
    if (available() == 0) return 0;

    uint8_t byte = rx_buffer[tail_ptr];

    // 推进读指针，并处理回环
    tail_ptr++;
    if (tail_ptr >= RX_BUFFER_SIZE) {
        tail_ptr = 0;
    }

    return byte;
}

uint8_t UART_Receiver::peek() const {
    if (available() == 0) return 0;
    return rx_buffer[tail_ptr];
}