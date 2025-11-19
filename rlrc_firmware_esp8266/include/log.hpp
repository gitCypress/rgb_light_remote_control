#pragma once

#include <Arduino.h>
#include <utility>

#include "Packet.hpp"

template<typename T>
concept SerialPrintable = requires(HardwareSerial &serial, T &&message) {
    { serial.print(std::forward<T>(message)) };
};

namespace Log {
    template<SerialPrintable T>
    void print(T &&message) {
        Serial.write(Packet::LOG_HEADER);
        Serial.print(std::forward<T>(message));
    }

    template<SerialPrintable T>
    void println(T &&message) {
        Serial.write(Packet::LOG_HEADER);
        Serial.println(std::forward<T>(message));
    }

    inline void println() {
        Serial.write(Packet::LOG_HEADER);
        Serial.println();
    }

    inline void printf(const char* format, ...) __attribute__((format(printf, 1, 2)));
    inline void printf(const char* format, ...) {
        // 1. 在栈上创建一个合理大小的缓冲区
        char buffer[256]; // 256 字节的日志缓冲区

        // 2. 获取可变参数列表
        va_list args;
        va_start(args, format);

        // 3. 安全地将格式化字符串写入缓冲区
        vsnprintf(buffer, sizeof(buffer), format, args);

        // 4. 清理可变参数
        va_end(args);

        // 5. (关键) 调用我们自己的 Log::print()
        //    它会自动处理 0xFE 包头！
        Log::print(buffer);
    }
}; // namespace Log
