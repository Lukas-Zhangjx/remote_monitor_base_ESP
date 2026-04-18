/**
 * @file    obstacle.c
 * @brief   障碍物检测模块实现
 *
 * 模块 OUT 引脚为开漏输出，低电平有效：
 *   检测到障碍物 → OUT 拉低 → GPIO 读到 0
 *   无障碍物     → OUT 释放 → 上拉拉高 → GPIO 读到 1
 */

#include "obstacle.h"
#include "esp_log.h"

static const char *TAG = "obstacle";

/* 记录初始化时配置的 GPIO 编号 */
static gpio_num_t s_gpio_num = GPIO_NUM_NC;


/**
 * @brief  初始化障碍物传感器，配置 GPIO 为上拉输入
 *
 * @param gpio_num  连接 OUT 引脚的 GPIO 编号
 * @return ESP_OK / ESP_FAIL
 */
esp_err_t obstacle_init(gpio_num_t gpio_num)
{
    s_gpio_num = gpio_num;

    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << gpio_num),
        .mode         = GPIO_MODE_INPUT,
        .pull_up_en   = GPIO_PULLUP_ENABLE,   /* 上拉，无障碍时保持高电平 */
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE,
    };

    esp_err_t ret = gpio_config(&io_conf);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "gpio_config failed: %d", ret);
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "obstacle sensor init ok, gpio=%d", gpio_num);
    return ESP_OK;
}


/**
 * @brief  读取当前检测状态
 *
 * @return 1 = 检测到障碍物（OUT=LOW），0 = 无障碍物（OUT=HIGH）
 */
int obstacle_detected(void)
{
    /* OUT 低电平有效，取反后 1 表示"检测到" */
    return gpio_get_level(s_gpio_num) == 0 ? 1 : 0;
}
