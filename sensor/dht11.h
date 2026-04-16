/**
 * @file    dht11.h
 * @brief   DHT11 温湿度传感器驱动公共接口
 *
 * DHT11 使用单总线协议，一次通信传输 40bit 数据：
 *   8bit 湿度整数 + 8bit 湿度小数 + 8bit 温度整数 + 8bit 温度小数 + 8bit 校验
 * DHT11 小数部分恒为 0，精度：温度 ±2°C，湿度 ±5%RH
 *
 * 依赖：esp_timer（微秒级延时）、driver/gpio
 */

#ifndef DHT11_H
#define DHT11_H

#include "esp_err.h"
#include "driver/gpio.h"

/**
 * @brief DHT11 读取结果结构体
 */
typedef struct {
    float temperature; /* 温度（°C） */
    float humidity;    /* 相对湿度（%RH） */
} dht11_data_t;

/**
 * @brief  初始化 DHT11，配置 GPIO
 *
 * @param gpio_num  连接 DHT11 DATA 引脚的 GPIO 编号
 * @return ESP_OK 成功，ESP_FAIL 失败
 */
esp_err_t dht11_init(gpio_num_t gpio_num);

/**
 * @brief  读取一次 DHT11 数据
 *
 * 单次读取耗时约 4ms，不可在中断中调用。
 * 两次读取间隔建议 >= 2 秒（DHT11 采样周期限制）。
 *
 * @param data  输出：温湿度数据
 * @return ESP_OK      读取成功
 * @return ESP_ERR_TIMEOUT  总线超时（接线问题或传感器未响应）
 * @return ESP_ERR_INVALID_CRC  校验和错误（数据损坏）
 */
esp_err_t dht11_read(dht11_data_t *data);

#endif /* DHT11_H */
