/**
 * @file    light_sensor.c
 * @brief   光敏传感器模块实现
 *
 * AO → ADC1_CH6 (GPIO34)：读取光照模拟值
 * DO → GPIO13：数字阈值输出，低电平有效（有光=LOW）
 */

#include "light_sensor.h"
#include "esp_log.h"

static const char *TAG = "light_sensor";

/* GPIO 和 ADC 句柄 */
static gpio_num_t        s_digital_gpio = GPIO_NUM_NC;
static adc_oneshot_unit_handle_t s_adc_handle = NULL;
static adc_channel_t     s_adc_channel  = ADC_CHANNEL_6;


/**
 * @brief  初始化光敏传感器
 */
esp_err_t light_sensor_init(gpio_num_t digital_gpio, adc_channel_t adc_channel)
{
    s_digital_gpio = digital_gpio;
    s_adc_channel  = adc_channel;

    /* ── 数字输入：上拉输入，DO 低电平有效 ── */
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << digital_gpio),
        .mode         = GPIO_MODE_INPUT,
        .pull_up_en   = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE,
    };
    esp_err_t ret = gpio_config(&io_conf);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "gpio_config failed: %d", ret);
        return ESP_FAIL;
    }

    /* ── ADC1 oneshot 初始化 ── */
    adc_oneshot_unit_init_cfg_t unit_cfg = {
        .unit_id  = ADC_UNIT_1,
        .ulp_mode = ADC_ULP_MODE_DISABLE,
    };
    ret = adc_oneshot_new_unit(&unit_cfg, &s_adc_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "adc_oneshot_new_unit failed: %d", ret);
        return ESP_FAIL;
    }

    /* ── 通道配置：12bit，11dB 衰减（量程 0~3.9V）── */
    adc_oneshot_chan_cfg_t chan_cfg = {
        .bitwidth = ADC_BITWIDTH_12,
        .atten    = ADC_ATTEN_DB_11,
    };
    ret = adc_oneshot_config_channel(s_adc_handle, adc_channel, &chan_cfg);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "adc_oneshot_config_chan failed: %d", ret);
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "light sensor init ok, digital=GPIO%d, adc_ch=%d", digital_gpio, adc_channel);
    return ESP_OK;
}


/**
 * @brief  读取数字输出状态
 *
 * @return 1 = 有光，0 = 暗
 */
int light_sensor_digital(void)
{
    /* DO 低电平有效：GPIO=0 → 有光 */
    return gpio_get_level(s_digital_gpio) == 0 ? 1 : 0;
}


/**
 * @brief  读取 ADC 原始值
 *
 * @return 0-4095，失败返回 -1
 */
int light_sensor_analog(void)
{
    int raw = 0;
    esp_err_t ret = adc_oneshot_read(s_adc_handle, s_adc_channel, &raw);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "adc read failed: %d", ret);
        return -1;
    }
    return raw;
}


/**
 * @brief  ADC 原始值转光照百分比（0=暗，100=最亮）
 */
int light_sensor_to_percent(int raw)
{
    if (raw < 0) return 0;
    /* 光照越强 → 光敏电阻阻值越小 → 分压越低 → ADC 值越小，取反映射 */
    int percent = 100 - (raw * 100 / 4095);
    if (percent < 0)   percent = 0;
    if (percent > 100) percent = 100;
    return percent;
}
