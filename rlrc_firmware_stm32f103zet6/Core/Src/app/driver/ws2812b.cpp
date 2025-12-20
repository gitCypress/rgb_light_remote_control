#include "ws2812b.hpp"

#include <cstring>

WS2812B & WS2812B::getInstance() {
    static WS2812B instance;
    return instance;
}

void WS2812B::setPixel(const uint8_t x, const uint8_t y, const uint8_t r, const uint8_t g, const uint8_t b) {
    if (x >= 5 || y >= 5) {  // 边界检查
        last_error.store(ErrorCode::INVALID_COORDS);
        return;
    }

    // 将 2D 坐标 (x,y) 转换为 1D 索引
    // Z 形布线 (从 0,0 到 4,4)
    const uint16_t index = y * 5 + x;

    led_data[index][0] = r;
    led_data[index][1] = g;
    led_data[index][2] = b;
}

void WS2812B::clear() { led_data = {}; }

void WS2812B::setAll(uint8_t r, uint8_t g, uint8_t b) {
    for (uint16_t i = 0; i < LED_COUNT; ++i) {
        led_data[i][0] = r;
        led_data[i][1] = g;
        led_data[i][2] = b;
    }
}

// TODO：在这里负责将一维数据转换成二维有点奇怪，后面考虑怎么优化
void WS2812B::setFrame(std::span<const uint8_t> frameData) {
    // 数据长度是否符合预期 (LED 数量 * 3)
    if (frameData.size() != LED_COUNT * 3) {
        last_error.store(ErrorCode::INVALID_FRAME_SIZE);
        return;
    }

    // 拷贝到新数组
    uint8_t* dest_ptr = &led_data[0][0];
    std::copy(frameData.begin(), frameData.end(), dest_ptr);
}

void WS2812B::render() {
    // 检查上一次 DMA 传输是否已完成
    if (!dma_transfer_complete_flag) {
        last_error.store(ErrorCode::RENDER_BUSY);
        return; // 上次传输还未结束，退出以防数据错乱
    }
    dma_transfer_complete_flag = false; // 标记为 "正在传输"

    uint16_t buffer_index = 0;

    // 将 RGB 转换为 PWM
    for (uint16_t led_i = 0; led_i < LED_COUNT; ++led_i) {
        // WS2812B 的数据顺序是 GRB，不是 RGB
        const uint8_t r = led_data[led_i][0];
        const uint8_t g = led_data[led_i][1];
        const uint8_t b = led_data[led_i][2];

        // 颜色数组，按 GRB 顺序
        uint8_t ws_colors_gbr[] = {g, r, b};

        // 遍历 G, R, B 三种颜色
        for (const uint8_t color: ws_colors_gbr) {
            // 遍历一个颜色的 8 个 bit (从 MSB 到 LSB)
            for (int8_t bit = 7; bit >= 0; --bit) {
                if ((color >> bit) & 1) {
                    // "1" 码
                    pwm_buffer[buffer_index] = PWM_HIGH_VAL;
                } else {
                    // "0" 码
                    pwm_buffer[buffer_index] = PWM_LOW_VAL;
                }
                buffer_index++;
            }
        }
    }

    // 启动 DMA 传输
    const HAL_StatusTypeDef status = HAL_TIM_PWM_Start_DMA(
        &htim1, // TIM 句柄
        TIM_CHANNEL_1, // TIM 通道
        reinterpret_cast<uint32_t *>(pwm_buffer.data()), // 内存数据源
        PWM_BUFFER_SIZE // 传输长度
    );

    if (status != HAL_OK) {
        last_error.store(ErrorCode::HAL_START_FAILED);
        dma_transfer_complete_flag = true;
    }
}

// 公共回调函数
void WS2812B::on_dma_transfer_complete() {
    // 设置标志，允许下一次 render()
    dma_transfer_complete_flag = true;
}

WS2812B::ErrorCode WS2812B::getLastError() {
    return last_error.exchange(ErrorCode::NONE);
    // .exchange() 是一个原子操作
    // 1. 返回 last_error 当前的值
    // 2. 将 last_error 设为 ErrorCode::NONE
}
