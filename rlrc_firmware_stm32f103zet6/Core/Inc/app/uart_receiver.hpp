#pragma once
#include <array>
#include <cstdint>
#include <span>

#include "main.h"

extern "C" UART_HandleTypeDef huart3;

class UART_Receiver {
public:
    static constexpr uint16_t RX_BUFFER_SIZE = 2048;

    static UART_Receiver &getInstance();

    /**
     * 启动 DMA 从 USART3 外设向 ringBuffer 的搬运过程。
     */
    void init();

    /**
     * @brief 尝试获取一个完整的解码包
     * * @param output_buffer [输入/输出] 调用者提供的临时工作区。
     * 函数会将原始数据拷贝到这里，并原地解码。
     * @return std::span<uint8_t> 解码后的有效数据视图。
     * 如果没收到完整包，返回空 span (size=0)。
     */
    std::span<uint8_t> tryGetPacket(std::span<uint8_t> output_buffer);

private:
    UART_Receiver() = default;

    [[nodiscard]] uint16_t aviliable() const;

    /**
     * @brief 写指针。计算自 DMA CNDTR
     */
    [[nodiscard]] uint16_t head() const;

    uint16_t tail = 0; // 读指针

    std::array<uint8_t, RX_BUFFER_SIZE> ringBuffer{}; // 环形缓冲区，DMA 负责生产
};
