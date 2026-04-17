/**
 * @file    led.h
 * @brief   LED GPIO 控制模块公共接口
 *
 * 使用推挽输出模式驱动 LED，高电平点亮。
 * 接线：GPIO → 220Ω 限流电阻 → LED阳极 → LED阴极 → GND
 */

#ifndef LED_H
#define LED_H

#include "esp_err.h"
#include "driver/gpio.h"

/**
 * @brief  初始化 LED GPIO，配置为推挽输出，默认熄灭
 *
 * @param gpio_num  连接 LED 的 GPIO 编号
 * @return ESP_OK 成功，ESP_FAIL 失败
 */
esp_err_t led_init(gpio_num_t gpio_num);

/**
 * @brief  点亮 LED
 */
void led_on(void);

/**
 * @brief  熄灭 LED
 */
void led_off(void);

/**
 * @brief  翻转 LED 状态
 */
void led_toggle(void);

/**
 * @brief  获取当前 LED 状态
 *
 * @return 1 = 点亮，0 = 熄灭
 */
int led_get_state(void);

#endif /* LED_H */
