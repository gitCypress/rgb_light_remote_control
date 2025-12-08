/**
 * COBS，Consistent Overhead Byte Stuffing，固定开销字节填充
 * 原理类似于链表，开头多一个路标来指明「原本」的 00 的位置。
 *
 * 以                                [11 22 00 33 00 44] 为例
 * 第一个 00 在第 3 位：开头加上路标， [03 11 22 00 33 00 44]
 * 第二个 00 与第一个 00 相距 2 位，  [03 11 22 02 33 00 44]
 * 第三个 00 距离隐形的末尾 也是 2 位，[03 11 22 02 33 02 44] ，最终得到的数据段完全没有 00
 * 最终发包时，加上分界符，           [03 11 22 02 33 02 44 00]
 *
 * 对于 uint8 而言，单个值最多只能表示 255 的距离
 * 因此在相对距离为 255 的时候也需要加 00 来分段，code = 0xFF 也就具有特殊含义了
 */

#pragma once
#include <cstdint>
#include <cstddef>
#include <span>

namespace Cobs {
    constexpr auto TAIL = 0x00;
    /**
     * @brief COBS 原地解码
     * @param buffer 存储结果的缓冲区 (输入时不含分界符)
     * @return std::span<uint8_t> 解码后进行截断的有效数据的视图。出错或溢出时，返回空 span 或截断数据。
     */
    inline std::span<uint8_t> decode(std::span<uint8_t> buffer) {
        size_t read_index = 0;  // 读指针
        size_t write_index = 0; // 写指针
        uint8_t code = 0;       // 零值路标

        while (read_index < buffer.size()) {
            code = buffer[read_index++];

            // 拷贝 (Code - 1) 个非零数据
            for (uint8_t block = 0; block < code - 1; block++) {
                if (read_index >= buffer.size()) break; // 输入越界保护

                buffer[write_index++] = buffer[read_index++];
            }

            // 如果 Code < 0xFF，且没读到流的末尾，说明这里原本有个 0
            if (code < 0xFF && read_index < buffer.size()) {
                buffer[write_index++] = TAIL;
            }
        }

        // .first(length) 会返回按照 length 长度截断的视图
        return buffer.first(write_index);
    }
}