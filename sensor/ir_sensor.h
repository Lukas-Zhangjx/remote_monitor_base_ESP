/**
 * @file    ir_sensor.h
 * @brief   红外检测模块公共接口（数字输出型）
 *
 * 适用于带数字输出的红外传感器模块（如 FC-51、TCRT5000 等）。
 * 模块输出电平：
 *   检测到目标（遮挡/反射）→ OUT = LOW  (0)
 *   无目标                  → OUT = HIGH (1)
 */

#ifndef IR_SENSOR_H
#define IR_SENSOR_H

#include "esp_err.h"
#include "driver/gpio.h"

/**
 * @brief  初始化红外传感器，配置 GPIO 为上拉输入
 *
 * @param gpio_num  连接 OUT 引脚的 GPIO 编号
 * @return ESP_OK / ESP_FAIL
 */
esp_err_t ir_sensor_init(gpio_num_t gpio_num);

/**
 * @brief  读取当前检测状态
 *
 * @return 1 = 检测到目标，0 = 无目标
 */
int ir_sensor_detected(void);

#endif /* IR_SENSOR_H */
