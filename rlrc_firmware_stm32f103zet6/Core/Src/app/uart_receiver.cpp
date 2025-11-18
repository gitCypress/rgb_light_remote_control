#include "uart_receiver.hpp"

#include <cstring>

UART_Receiver & UART_Receiver::getInstance() {
    static UART_Receiver instance;
    return instance;
}

// --- 启动接收器 ---
void UART_Receiver::init() {
    // 开启 IDLE 中断
    __HAL_UART_ENABLE_IT(&huart3, UART_IT_IDLE);

    // 启动 DMA 循环接收
    HAL_UART_Receive_DMA(&huart3, rx_buffer.data(), RX_BUFFER_SIZE);
}

// 中断回调
void UART_Receiver::uart_irq_handler() {
    // 检查是否是 IDLE 中断触发
    if (__HAL_UART_GET_FLAG(&huart3, UART_FLAG_IDLE) != RESET) {

        // 清除 IDLE 标志
        __HAL_UART_CLEAR_IDLEFLAG(&huart3);

        // 计算 DMA "Head" 指针当前位置
        // CNDTR 寄存器是“向下数”的，所以用总大小减去它
        uint16_t current_pos = RX_BUFFER_SIZE - __HAL_DMA_GET_COUNTER(huart3.hdmarx);

        // 如果位置没变，或有包在等待处理，则忽略
        if (current_pos == old_pos || packet_ready.load()) {
            return;
        }

        // 计算新收到的数据包长度
        uint16_t length;
        if (current_pos > old_pos) {
            // 数据是连续的
            // [ ... old_pos ... current_pos ... ]
            length = current_pos - old_pos;
        } else {
            // 数据在缓冲区末尾“回环”
            // [ ... current_pos ... old_pos ... ]
            length = (RX_BUFFER_SIZE - old_pos) + current_pos;
        }

        // 检查包是否有效且在大小限制内
        if (length > 0 && length <= PACKET_BUFFER_SIZE) {

            // 将数据从 DMA 缓冲区拷贝到包缓冲区
            if (current_pos > old_pos) {
                // A. 连续：一次拷贝
                memcpy(packet_buffer.data(), &rx_buffer[old_pos], length);
            } else {
                // B. 不连续：两次拷贝
                const uint16_t part1_len = RX_BUFFER_SIZE - old_pos;
                memcpy(packet_buffer.data(), &rx_buffer[old_pos], part1_len);
                memcpy(packet_buffer.data() + part1_len, &rx_buffer[0], current_pos);
            }

            // 设置标志，通知主循环
            packet_length.store(length);
            packet_ready.store(true);
        }

        // 更新下一次的起始位置
        old_pos = current_pos;
    }
}

bool UART_Receiver::isPacketReady() const {
    return packet_ready.load();
}

uint16_t UART_Receiver::getPacket(uint8_t* out_buffer) {
    if (!packet_ready.load()) {
        return 0;
    }

    // 读取长度
    const uint16_t length = packet_length.load();

    // 拷贝数据到外部缓冲区
    memcpy(out_buffer, packet_buffer.data(), length);

    // 清除标志，准备接收下一个包
    packet_ready.store(false);

    return length;
}