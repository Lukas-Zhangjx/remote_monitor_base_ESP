/**
 * @file    obstacle.h
 * @brief   障碍物检测模块公共接口
 *
 * 模块输出数字电平：
 *   检测到障碍物 → OUT = LOW  (0)
 *   无障碍物     → OUT = HIGH (1)
 */

#ifndef OBSTACLE_H
#define OBSTACLE_H

#include "esp_err.h"
#include "driver/gpio.h"

/**
 * @brief  初始化障碍物传感器，配置 GPIO 为上拉输入
 *
 * @param gpio_num  连接 OUT 引脚的 GPIO 编号
 * @return ESP_OK / ESP_FAIL
 */
esp_err_t obstacle_init(gpio_num_t gpio_num);

/**
 * @brief  读取当前检测状态
 *
 * @return 1 = 检测到障碍物，0 = 无障碍物
 */
int obstacle_detected(void);

#endif /* OBSTACLE_H */
