/**
 * @file    relay.h
 * @brief   继电器 / GPIO 输出控制模块公共接口
 *
 * 推挽输出模式，高电平有效：
 *   state = 1 → GPIO 高电平 → 负载导通
 *   state = 0 → GPIO 低电平 → 负载断开
 */

#ifndef RELAY_H
#define RELAY_H

#include "esp_err.h"
#include "driver/gpio.h"

/**
 * @brief  初始化继电器 GPIO，配置为推挽输出，默认断开
 *
 * @param gpio_num  连接继电器/LED 的 GPIO 编号
 * @return ESP_OK / ESP_FAIL
 */
esp_err_t relay_init(gpio_num_t gpio_num);

/**
 * @brief  设置继电器状态
 *
 * @param state  1 = 导通（开），0 = 断开（关）
 * @return       实际设置后的状态
 */
int relay_set(int state);

/**
 * @brief  获取当前继电器状态
 *
 * @return 1 = 导通，0 = 断开
 */
int relay_get_state(void);

#endif /* RELAY_H */
