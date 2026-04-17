/**
 * @file    led.c
 * @brief   LED GPIO 控制模块实现
 *
 * 推挽输出模式：GPIO 高电平 = LED 点亮，低电平 = LED 熄灭
 * 内部维护当前状态，避免每次都读取 GPIO 寄存器
 */

#include "led.h"
#include "esp_log.h"

static const char *TAG = "led";

/* 记录初始化时配置的 GPIO 编号 */
static gpio_num_t s_gpio_num = GPIO_NUM_NC;

/* 缓存当前 LED 状态：1 = 点亮，0 = 熄灭 */
static int s_state = 0;


/**
 * @brief  初始化 LED GPIO，配置为推挽输出，默认熄灭
 *
 * @param gpio_num  连接 LED 的 GPIO 编号
 * @return ESP_OK / ESP_FAIL
 */
esp_err_t led_init(gpio_num_t gpio_num)
{
    s_gpio_num = gpio_num;

    /* 配置为推挽输出，不需要上拉/下拉 */
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

    /* 初始化后默认熄灭 */
    gpio_set_level(s_gpio_num, 0);
    s_state = 0;

    ESP_LOGI(TAG, "led init ok, gpio=%d", gpio_num);
    return ESP_OK;
}

/**
 * @brief  点亮 LED
 */
void led_on(void)
{
    gpio_set_level(s_gpio_num, 1);
    s_state = 1;
    ESP_LOGD(TAG, "led on");
}

/**
 * @brief  熄灭 LED
 */
void led_off(void)
{
    gpio_set_level(s_gpio_num, 0);
    s_state = 0;
    ESP_LOGD(TAG, "led off");
}

/**
 * @brief  翻转 LED 状态
 */
void led_toggle(void)
{
    /* 根据缓存状态翻转，避免读取 GPIO 寄存器 */
    if (s_state) {
        led_off();
    } else {
        led_on();
    }
}

/**
 * @brief  获取当前 LED 状态
 *
 * @return 1 = 点亮，0 = 熄灭
 */
int led_get_state(void)
{
    return s_state;
}
