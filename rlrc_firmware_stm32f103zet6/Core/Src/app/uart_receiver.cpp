#include "uart_receiver.hpp"

#include "cobs.hpp"

UART_Receiver &UART_Receiver::getInstance() {
    static UART_Receiver instance;
    return instance;
}

void UART_Receiver::init() {
    // 为 DMA 指定搬运源是 USART3 串口外设
    // 目的地是 ringBuffer
    // 我们开启了 Circular 模式，RX_BUFFER_SIZE 在这里是一轮搬运的长度
    // 与 ringBuffer 对齐以后就形成了环形缓冲区
    HAL_UART_Receive_DMA(&huart3, ringBuffer.data(), ringBuffer.size());
}

std::span<uint8_t> UART_Receiver::tryGetPacket(std::span<uint8_t> output_buffer) {
    const uint16_t count = aviliable();

    // 没有数据
    if (count == 0) {
        return {};
    };

    // 在 RingBuffer 中寻找包尾
    int32_t tailOffset = -1;
    uint16_t packetLen = 0; // 编码数据的长度 (不含 0x00)
    uint16_t searchPtr = tail;
    for (uint16_t i = 0; i < count; i++) {
        if (ringBuffer[searchPtr] == Cobs::TAIL) {
            tailOffset = i;
            packetLen = i;
            break;
        }
        // 环形自增
        searchPtr++;
        if (searchPtr >= RX_BUFFER_SIZE) searchPtr = 0;
    }

    // 没找到包尾，说明包还没收完，直接返回空
    if (tailOffset == -1) {
        // TODO：如果 count 接近 BUFFER_SIZE 还没找到 0，说明可能溢出或错乱，可以选择在这里 flush 掉一部分数据防止死锁。
        return {};
    }

    // 边界检查，确保用户提供的 buffer 够大
    if (packetLen > output_buffer.size()) { // 如果不够大
        // 丢弃这个包，防止死循环
        // 推进 tail_ptr = packetLen + 1，跳过 0x00
        const uint16_t skipLen = packetLen + 1;
        tail = (tail + skipLen) % RX_BUFFER_SIZE;
        return {};
    }

    // 线性化，从 ringBuffer 拷贝到 output_buffer
    uint16_t copyPtr = tail;
    for (uint16_t i = 0; i < packetLen; i++) {
        output_buffer[i] = ringBuffer[copyPtr];
        copyPtr++;
        if (copyPtr >= RX_BUFFER_SIZE) copyPtr = 0;
    }

    // 推进 RingBuffer 读指针，消费掉 encoded data + 0x00
    tail = (tail + packetLen + 1) % RX_BUFFER_SIZE;

    // 原地 COBS 解码
    // output_buffer 的前 packetLen 个字节是有效输入
    // decode_inplace 会返回解码后的视图
    return Cobs::decode(output_buffer.first(packetLen));
}

uint16_t UART_Receiver::aviliable() const {
    const auto currHead = head();
    return currHead < tail ? currHead - tail + ringBuffer.size() : currHead - tail; // 处理回环
}

uint16_t UART_Receiver::head() const {
    // 计算 DMA 写指针
    // __HAL_DMA_GET_COUNTER 读取 DMA Controller 中的 CNDTR (Channel Number of Data to Transfer) 寄存器
    // huart3.h~dma~rx 是 USART3 的 DMA 句柄
    // CNDTR 是倒过来的，返回的是"剩余空间"，要用总大小减一次
    return ringBuffer.size() - __HAL_DMA_GET_COUNTER(huart3.hdmarx);
}
