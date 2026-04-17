/**
 * @file    dht11.c
 * @brief   DHT11 温湿度传感器驱动实现
 *
 * 单总线时序（参考 DHT11 datasheet）：
 *
 *  主机起始信号：
 *    DATA 拉低 >= 18ms → 拉高 20~40μs
 *
 *  DHT11 响应：
 *    拉低 80μs → 拉高 80μs → 开始发送 40bit 数据
 *
 *  bit 编码：
 *    "0"：50μs 低电平 + 26~28μs 高电平
 *    "1"：50μs 低电平 + 70μs 高电平
 *    以高电平持续时间区分 0/1，阈值约 40μs
 *
 *  数据格式（40bit）：
 *    [39:32] 湿度整数  [31:24] 湿度小数（DHT11 恒为 0）
 *    [23:16] 温度整数  [15: 8] 温度小数（DHT11 恒为 0）
 *    [ 7: 0] 校验和 = 前四字节之和低8位
 */

#include <string.h>
#include "dht11.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/portmacro.h"
#include "rom/ets_sys.h"

static const char *TAG = "dht11";

/* 记录初始化时配置的 GPIO 编号 */
static gpio_num_t s_gpio_num = GPIO_NUM_NC;

/* 等待总线电平变化的超时时间（μs）
 * DHT11 响应信号最长 80μs，留足余量设为 200μs */
#define DHT11_TIMEOUT_US  200


/**
 * @brief  等待 DATA 线变为指定电平，返回等待时间（μs），超时返回 -1
 *
 * @param level  期望电平（0 或 1）
 * @return 等待耗时（μs），超时返回 -1
 */
static int wait_for_level(int level)
{
    int elapsed = 0;
    while (gpio_get_level(s_gpio_num) != level) {
        if (elapsed >= DHT11_TIMEOUT_US) {
            return -1; /* 超时 */
        }
        ets_delay_us(1);
        elapsed++;
    }
    return elapsed;
}


/**
 * @brief  初始化 DHT11，配置 DATA 引脚为开漏输出模式
 *
 * @param gpio_num  DATA 引脚编号
 * @return ESP_OK / ESP_FAIL
 */
esp_err_t dht11_init(gpio_num_t gpio_num)
{
    s_gpio_num = gpio_num;

    /* 配置为开漏模式：可输出低电平，释放后由上拉电阻拉高 */
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << gpio_num),
        .mode         = GPIO_MODE_INPUT_OUTPUT_OD, /* 开漏，可读可写 */
        .pull_up_en   = GPIO_PULLUP_ENABLE,        /* 使能内部上拉 */
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE,
    };

    esp_err_t ret = gpio_config(&io_conf);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "gpio_config failed: %d", ret);
        return ESP_FAIL;
    }

    /* 初始化后拉高总线，等待 DHT11 稳定（上电后需要 1s） */
    gpio_set_level(s_gpio_num, 1);
    vTaskDelay(pdMS_TO_TICKS(1000));

    ESP_LOGI(TAG, "dht11 init ok, gpio=%d", gpio_num);
    return ESP_OK;
}


/**
 * @brief  读取一次 DHT11 的温湿度数据
 *
 * @param data  输出结果
 * @return ESP_OK / ESP_ERR_TIMEOUT / ESP_ERR_INVALID_CRC
 */
esp_err_t dht11_read(dht11_data_t *data)
{
    uint8_t raw[5] = {0}; /* 40bit = 5字节原始数据 */

    /* ---- 1. 主机发起起始信号 ---- */
    /* 拉低 >= 18ms，通知 DHT11 开始通信 */
    gpio_set_level(s_gpio_num, 0);
    vTaskDelay(pdMS_TO_TICKS(20)); /* 20ms，大于最低要求 18ms */

    /* ---- 2. 关闭中断后进行时序关键段 ---- */
    portDISABLE_INTERRUPTS();

    /* 释放总线：切换为纯输入模式，彻底撤掉输出驱动，由上拉电阻拉高
     * 开漏模式 set(1) 在某些情况下释放不干净，纯输入最可靠 */
    gpio_set_direction(s_gpio_num, GPIO_MODE_INPUT);
    ets_delay_us(30);

    /* ---- 3. 等待 DHT11 响应 ---- */
    /* 响应：先拉低 80μs */
    if (wait_for_level(0) < 0) {
        portENABLE_INTERRUPTS();
        gpio_set_direction(s_gpio_num, GPIO_MODE_INPUT_OUTPUT_OD);
        gpio_set_level(s_gpio_num, 1);
        ESP_LOGE(TAG, "timeout waiting for DHT11 response low");
        return ESP_ERR_TIMEOUT;
    }
    /* 再拉高 80μs */
    if (wait_for_level(1) < 0) {
        portENABLE_INTERRUPTS();
        gpio_set_direction(s_gpio_num, GPIO_MODE_INPUT_OUTPUT_OD);
        gpio_set_level(s_gpio_num, 1);
        ESP_LOGE(TAG, "timeout waiting for DHT11 response high");
        return ESP_ERR_TIMEOUT;
    }

    /* ---- 3. 读取 40bit 数据 ---- */
    for (int i = 0; i < 40; i++) {
        /* 每个 bit 以 50μs 低电平开始 */
        if (wait_for_level(0) < 0) {
            portENABLE_INTERRUPTS();
            gpio_set_direction(s_gpio_num, GPIO_MODE_INPUT_OUTPUT_OD);
            gpio_set_level(s_gpio_num, 1);
            ESP_LOGE(TAG, "timeout at bit %d low", i);
            return ESP_ERR_TIMEOUT;
        }
        /* 高电平持续时间决定 bit 值：
         *   < 40μs → bit 0
         *   >= 40μs → bit 1 */
        if (wait_for_level(1) < 0) {
            portENABLE_INTERRUPTS();
            gpio_set_direction(s_gpio_num, GPIO_MODE_INPUT_OUTPUT_OD);
            gpio_set_level(s_gpio_num, 1);
            ESP_LOGE(TAG, "timeout at bit %d high start", i);
            return ESP_ERR_TIMEOUT;
        }

        ets_delay_us(40); /* 等待 40μs 后采样 */

        /* 此时高电平还在持续 → bit1，已经结束 → bit0 */
        raw[i / 8] <<= 1;
        if (gpio_get_level(s_gpio_num) == 1) {
            raw[i / 8] |= 1;
        }
    }

    portENABLE_INTERRUPTS();

    /* 恢复开漏输出，总线拉高，等待下次通信 */
    gpio_set_direction(s_gpio_num, GPIO_MODE_INPUT_OUTPUT_OD);
    gpio_set_level(s_gpio_num, 1);

    /* ---- 4. 校验和验证 ---- */
    uint8_t checksum = raw[0] + raw[1] + raw[2] + raw[3];
    if (checksum != raw[4]) {
        ESP_LOGE(TAG, "checksum error: calc=0x%02X recv=0x%02X", checksum, raw[4]);
        return ESP_ERR_INVALID_CRC;
    }

    /* ---- 5. 解析数据 ---- */
    /* DHT11 小数部分（raw[1], raw[3]）恒为 0，直接取整数部分 */
    data->humidity    = (float)raw[0];
    data->temperature = (float)raw[2];

    ESP_LOGI(TAG, "read ok: temp=%.1f humi=%.1f", data->temperature, data->humidity);
    return ESP_OK;
}
