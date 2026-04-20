/**
 * @file    light_sensor.h
 * @brief   光敏传感器模块公共接口
 *
 * 支持两路输出：
 *   AO（模拟）→ ADC1_CH6 (GPIO34)，读取光照强度原始值 0-4095
 *   DO（数字）→ GPIO13，低电平表示光照超过阈值（亮），高电平表示暗
 *
 * ADC 使用 ESP-IDF v5.x adc_oneshot 驱动。
 */

#ifndef LIGHT_SENSOR_H
#define LIGHT_SENSOR_H

#include "esp_err.h"
#include "driver/gpio.h"
#include "esp_adc/adc_oneshot.h"

/**
 * @brief  初始化光敏传感器
 *
 * @param digital_gpio  DO 引脚编号（数字输出，GPIO13）
 * @param adc_channel   AO 对应的 ADC1 通道（ADC_CHANNEL_6 for GPIO34）
 * @return ESP_OK / ESP_FAIL
 */
esp_err_t light_sensor_init(gpio_num_t digital_gpio, adc_channel_t adc_channel);

/**
 * @brief  读取数字输出状态
 *
 * @return 1 = 有光（DO=LOW，光照超过阈值），0 = 暗（DO=HIGH）
 */
int light_sensor_digital(void);

/**
 * @brief  读取模拟光照强度
 *
 * @return 0-4095 原始 ADC 值，值越小光照越强（光敏电阻阻值随光增大而减小）
 */
int light_sensor_analog(void);

/**
 * @brief  将 ADC 原始值转换为光照百分比
 *
 * @param raw  ADC 原始值 0-4095
 * @return     0-100，值越大表示越亮
 */
int light_sensor_to_percent(int raw);

#endif /* LIGHT_SENSOR_H */
