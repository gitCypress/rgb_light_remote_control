#pragma once

#include "main.h"

#include <array>
#include <atomic>
#include <cstdint> // uint8_t, uint16_t...

// 外部链接 CubeMX 生成的 TIM1 句柄
extern "C" TIM_HandleTypeDef htim1;

class WS2812B {
public:
    // 错误代码
    enum class ErrorCode : uint8_t {
        NONE = 0,
        RENDER_BUSY,      // render() 调用太快，上一帧未完成
        HAL_START_FAILED, // HAL_TIM_PWM_Start_DMA 失败
        INVALID_COORDS    // setPixel() 坐标越界
    };

    // 灯珠数量
    static constexpr uint16_t LED_COUNT = 5 * 5;

    // WS2812B 码元 (72MHz / 90 ticks = 800kHz)
    static constexpr uint16_t PWM_HIGH_VAL = 64; // "1" 码 (0.8µs)
    static constexpr uint16_t PWM_LOW_VAL = 32; // "0" 码 (0.4µs)

    // WS2812B 协议需 24 bits (G, R, B)
    static constexpr uint16_t BITS_PER_LED = 24;

    // 重置码
    // 需要 >50µs 的低电平，用 50 个 1.25µs 的低电平实现
    static constexpr uint16_t RESET_PULSES = 50;

    // PWM 缓冲区的总大小
    // 灯数 * 24 bit + RESET
    static constexpr uint16_t PWM_BUFFER_SIZE = (LED_COUNT * BITS_PER_LED) + RESET_PULSES;

    static WS2812B& getInstance();

    /**
     * @brief 设置单个像素的颜色 (颜色存储在内部缓冲区)
     * @param x 坐标 (0-4)
     * @param y 坐标 (0-4)
     * @param r 红色 (0-255)
     * @param g 绿色 (0-255)
     * @param b 蓝色 (0-255)
     */
    void setPixel(uint8_t x, uint8_t y, uint8_t r, uint8_t g, uint8_t b);

    /**
     * @brief 将所有像素设为黑色 (关闭)
     */
    void clear();

    void setAll(uint8_t r, uint8_t g, uint8_t b);

    /**
     * @brief 渲染
     * 将 setPixel() 设置的颜色转换并启动 DMA 传输
     */
    void render();

    /**
     * @brief DMA 传输完成时由中断回调调用的公共函数
     */
    void on_dma_transfer_complete();

    /**
     * @brief 获取最后一次发生的错误，并清除错误状态
     * @return ErrorCode 错误代码
     */
    ErrorCode getLastError();

    WS2812B(const WS2812B&) = delete;
    WS2812B& operator=(const WS2812B&) = delete;
private:
    WS2812B() = default;
    ~WS2812B() = default;

    // 缓冲区 1: 存储灯珠的 RGB "目标"颜色
    // [LED_COUNT][3] -> 25 * 3 = 75 字节
    // [led_index][0=R, 1=G, 2=B]
    std::array<std::array<uint8_t, 3>, LED_COUNT> led_data{};

    // 缓冲区 2: 存储发送给 DMA 的 PWM "脉宽"值
    // (25 * 24 + 50) * 2 字节 = 1300 字节 (uint16_t)
    std::array<uint16_t, PWM_BUFFER_SIZE> pwm_buffer{};

    std::atomic_bool dma_transfer_complete_flag = true;   // 初始状态为「已完成」
    std::atomic<ErrorCode> last_error{ErrorCode::NONE};  // 错误状态变量
};
