//
// Created by 27301 on 2025/11/17.
//

#pragma once

#include <array>
#include <atomic>
#include <cstdint>
#include <span>
#include "main.h"

// 外部链接 CubeMX 生成的 USART3 句柄
extern "C" UART_HandleTypeDef huart3;

class UART_Receiver {
public:
    // DMA 循环缓冲区大小
    static constexpr uint16_t RX_BUFFER_SIZE = 256;

    // 最大数据包大小 (小于 RX_BUFFER_SIZE，大于 76 字节的全屏刷新包)
    static constexpr uint16_t PACKET_BUFFER_SIZE = 128;

    /**
     * @brief 获取 UART_Receiver 的单例
     */
    static UART_Receiver &getInstance();

    /**
     * @brief 启动 DMA 和 IDLE 中断
     * 在 maincxx() 中，MX_USART3_Init() 之后调用
     */
    void init();

    /**
     * @brief 由 C 中断处理函数 USART3_IRQHandler 调用
     */
    void uart_irq_handler();

    /**
     * @brief 检查是否有新包，在 maincxx 轮询
     */
    [[nodiscard]] bool isPacketReady() const;

    /**
     * @brief 获取数据包，在 maincxx 轮询
     * @param out_buffer 你的缓冲区，用于接收数据
     * @return 接收到的数据包长度 (字节)，如果为 0 则表示没有包
     */
    uint16_t getPacket(uint8_t *out_buffer);

    /**
     * @brief 高阶函数：如果有包，就调用回调处理
     */
    template<typename Func>
    void handlePacket(Func callback) {
        if (packet_ready.load()) {
            const uint16_t len = packet_length.load(); // 获取长度
            std::span<const uint8_t> view(packet_buffer.data(), len); // 创建视图
            callback(view); // 调用回调
            packet_ready.store(false); // 自动清理标志
        }
    }

    UART_Receiver(const UART_Receiver &) = delete;
    UART_Receiver &operator=(const UART_Receiver &) = delete;

private:
    UART_Receiver() = default;
    ~UART_Receiver() = default;

    // DMA 循环缓冲区
    std::array<uint8_t, RX_BUFFER_SIZE> rx_buffer{};

    // 完整数据包缓冲区
    std::array<uint8_t, PACKET_BUFFER_SIZE> packet_buffer{};

    // 状态标志
    std::atomic_bool packet_ready{false};
    std::atomic_uint16_t packet_length{0};

    // DMA 指针，用于计算新数据
    uint16_t old_pos = 0;
};
