#pragma once
#include <cstddef>
#include <cstdint>
#include <span>

namespace Cobs {
    /**
     * @brief COBS 解码
     * @param input 输入数据 (不包含末尾的 0x00)
     * @param output 解码后的数据
     * @return 解码后的长度
     */
    inline size_t decode(const std::span<const uint8_t> input, uint8_t* output) {
        size_t read_index = 0;
        size_t write_index = 0;
        uint8_t code = 0;
        uint8_t block = 0;

        while (read_index < input.size()) {
            code = input[read_index++];
            for (block = 0; block < code - 1; block++) {
                if (read_index >= input.size()) break;
                output[write_index++] = input[read_index++];
            }
            if (code < 0xFF && read_index < input.size()) {
                output[write_index++] = 0;
            }
        }
        return write_index;
    }
}