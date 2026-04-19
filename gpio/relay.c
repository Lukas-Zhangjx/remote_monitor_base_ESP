/**
 * @file    relay.c
 * @brief   继电器 / GPIO 输出控制模块实现
 *
 * 推挽输出模式，高电平有效。
 * 当前支持单路输出（GPIO15），扩展多路时可改为数组。
 */

#include "relay.h"
#include "esp_log.h"

static const char *TAG = "relay";

/* 记录初始化时配置的 GPIO 编号 */
static gpio_num_t s_gpio_num = GPIO_NUM_NC;

/* 缓存当前状态：1 = 导通，0 = 断开 */
static int s_state = 0;


/**
 * @brief  初始化继电器 GPIO，配置为推挽输出，默认断开
 *
 * @param gpio_num  连接继电器/LED 的 GPIO 编号
 * @return ESP_OK / ESP_FAIL
 */
esp_err_t relay_init(gpio_num_t gpio_num)
{
    s_gpio_num = gpio_num;

    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << gpio_num),
        .mode         = GPIO_MODE_OUTPUT,
        .pull_up_en   = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE,
    };

    esp_err_t ret = gpio_config(&io_conf);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "gpio_config failed: %d", ret);
        return ESP_FAIL;
    }

    /* 初始化后默认断开 */
    gpio_set_level(s_gpio_num, 0);
    s_state = 0;

    ESP_LOGI(TAG, "relay init ok, gpio=%d", gpio_num);
    return ESP_OK;
}


/**
 * @brief  设置继电器状态
 *
 * @param state  1 = 导通，0 = 断开
 * @return       实际设置后的状态
 */
int relay_set(int state)
{
    s_state = (state != 0) ? 1 : 0;
    gpio_set_level(s_gpio_num, s_state);
    ESP_LOGI(TAG, "relay -> %s", s_state ? "ON" : "OFF");
    return s_state;
}


/**
 * @brief  获取当前继电器状态
 *
 * @return 1 = 导通，0 = 断开
 */
int relay_get_state(void)
{
    return s_state;
}
