//
// Created by 27301 on 2025/11/19.
//

#include "../../../Inc/app/driver/esp8266.hpp"
#include "main.h"

#include "stm32f1xx_hal_gpio.h"

void esp8266::enable() {
    HAL_GPIO_WritePin(ESP_Enable_GPIO_Port, ESP_Enable_Pin, GPIO_PIN_SET);
}

