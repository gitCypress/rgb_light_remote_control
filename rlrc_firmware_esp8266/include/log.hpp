#pragma once

#include <Arduino.h>
#include "Packet.hpp"
#include <PacketSerial.h>
#include <utility>

template<typename T>
concept SerialPrintable =
    requires(HardwareSerial& serial, T&& message) {
        { serial.print(std::forward<T>(message)) };
    };

namespace Log {

    // --- [私有] 核心辅助函数 ---
    // 负责：加包头 -> COBS编码 -> 发送
    inline void sendEncodedPacket(const uint8_t* payload, size_t length) {
        // 1. 准备缓冲区: [Header] + [Payload]
        // 256 字节通常够日志用了，static 避免频繁栈分配 (ESP8266 是单线程的，安全)
        static uint8_t packetBuf[256];

        // 截断保护
        if (length > 254) length = 254;

        packetBuf[0] = Packet::LOG_HEADER;
        memcpy(&packetBuf[1], payload, length);

        // 2. 使用 PacketSerial 进行 COBS 编码并发送
        // PacketSerial 对象很轻量，这就相当于一个包装器
        PacketSerial packetSerial;
        packetSerial.setStream(&Serial);

        // send() 会自动：COBS编码 -> 发送 -> 发送0x00包尾
        packetSerial.send(packetBuf, length + 1);
    }

    // --- 重载 1: 统一处理模板类型 (print) ---
    void print(SerialPrintable auto&& message) {
        // 将任意类型转换为 String，以便获取底层的字节数组
        String s = String(std::forward<decltype(message)>(message));
        sendEncodedPacket(reinterpret_cast<const uint8_t *>(s.c_str()), s.length());
    }

    // --- 重载 2: 统一处理模板类型 (println) ---
    void println(SerialPrintable auto&& message) {
        auto s = String(std::forward<decltype(message)>(message));
        s += "\n\r"; // 手动追加换行符
        sendEncodedPacket(reinterpret_cast<const uint8_t *>(s.c_str()), s.length());
    }

    // --- 重载 3: 空行 ---
    inline void println() {
        sendEncodedPacket(reinterpret_cast<const uint8_t *>("\n\r"), 1);
    }

    // --- 重载 4: printf ---
    inline void printf(const char* format, ...) __attribute__((format(printf, 1, 2)));

    inline void printf(const char* format, ...) {
        char buffer[256];
        va_list args;
        va_start(args, format);
        const int len = vsnprintf(buffer, sizeof(buffer), format, args);
        va_end(args);

        if (len > 0) {
            sendEncodedPacket(reinterpret_cast<const uint8_t *>(buffer), len);
        }
    }

} // namespace Log