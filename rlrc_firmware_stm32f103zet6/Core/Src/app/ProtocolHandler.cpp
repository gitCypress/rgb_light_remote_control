#include "ProtocolHandler.hpp"
#include "uart_receiver.hpp"
#include "ws2812b.hpp"
#include "Cobs.hpp" // 引入解码算法
#include <cstdio>
#include <cstring> // for memcpy/memset

// 协议常量
constexpr uint8_t CMD_SET_PIXEL = 0x01;
constexpr uint8_t CMD_TOGGLE    = 0x03;
constexpr uint8_t HEADER_LOG    = 0xFE;

ProtocolHandler& ProtocolHandler::getInstance() {
    static ProtocolHandler instance;
    return instance;
}

void ProtocolHandler::update() {
    auto& uart = UART_Receiver::getInstance();

    // 只要 UART 硬件缓冲区里有数据，就一直循环处理
    while (uart.available() > 0) {
        uint8_t byte = uart.read();

        if (byte == 0x00) {
            // ==============================
            // ✨ 魔法时刻：检测到包尾 (0x00)
            // ==============================
            if (_encodedIndex > 0) {
                // 1. 调用 COBS 解码
                // 输入：encodedBuffer 的前 _encodedIndex 个字节
                // 输出：写入 decodedBuffer
                size_t decodedLen = Cobs::decode(
                    {_encodedBuffer.data(), _encodedIndex}, 
                    _decodedBuffer.data()
                );

                // 2. 分发解码后的数据
                if (decodedLen > 0) {
                    dispatchDecoded({_decodedBuffer.data(), decodedLen});
                }

                // 3. 重置索引，准备接收下一包
                _encodedIndex = 0;
            }
        } else {
            // ==============================
            // 还在接收中，存入缓冲区
            // ==============================
            if (_encodedIndex < _encodedBuffer.size()) {
                _encodedBuffer[_encodedIndex++] = byte;
            } else {
                // 缓冲区溢出保护：
                // 如果一包数据超过 256 字节还没遇到 0x00，说明出错了。
                // 我们清空缓冲区，重新开始寻找下一个 0x00。
                _encodedIndex = 0;
                printf("[ERROR] COBS Buffer Overflow! Dropping packet.\r\n");
            }
        }
    }
}

// --- 分发逻辑 (此时数据已经是干净的 [Header][Args...]) ---

void ProtocolHandler::dispatchDecoded(std::span<const uint8_t> packet) {
    uint8_t header = packet[0];

    // payload 是除去 header 剩下的部分
    // 如果 packet 只有一个字节，subspan(1) 会返回空视图，这是安全的
    std::span<const uint8_t> payload = (packet.size() > 1) ? packet.subspan(1) : std::span<const uint8_t>{};

    switch (header) {
        case CMD_SET_PIXEL:
            handleSetPixel(payload);
            break;
        case CMD_TOGGLE:
            handleToggle(payload);
            break;
        case HEADER_LOG:
            handleLog(payload);
            break;
        default:
            // 这里不需要再打印未知包了，除非你是在调试新协议
            // printf("[WARN] Unknown Cmd: 0x%02X\r\n", header);
            break;
    }
}

// --- 具体业务逻辑 ---

void ProtocolHandler::handleSetPixel(std::span<const uint8_t> payload) {
    // 需要 5 个字节: X, Y, R, G, B
    if (payload.size() < 5) return;

    auto& led = WS2812B::getInstance();
    led.setPixel(payload[0], payload[1], payload[2], payload[3], payload[4]);
    led.render();
    
    printf("[ESP->BIN] SetPixel (%d,%d)\r\n", payload[0], payload[1]);
}

void ProtocolHandler::handleToggle(std::span<const uint8_t> payload) {
    // 需要 1 个字节: State
    if (payload.size() < 1) return;

    bool isOff = (payload[0] == 0x00);
    auto& led = WS2812B::getInstance();
    
    if (isOff) led.clear();
    else led.setAll(64, 64, 64);
    
    led.render();
    printf("[ESP->BIN] Toggle: %s\r\n", isOff ? "OFF" : "ON");
}

void ProtocolHandler::handleLog(std::span<const uint8_t> payload) {
    if (payload.empty()) return;

    // 安全打印：我们需要一个以 \0 结尾的 buffer
    // 256 是 _decodedBuffer 的大小，足够了
    char logBuf[256];
    size_t len = payload.size();
    if (len >= 256) len = 255; // 截断保护

    std::memcpy(logBuf, payload.data(), len);
    logBuf[len] = '\0'; // 添加结束符

    printf("[ESP->LOG] %s", logBuf);

    // 如果原日志没带换行，补一个
    if (len > 0 && logBuf[len-1] != '\n') {
        printf("\r\n");
    }
}