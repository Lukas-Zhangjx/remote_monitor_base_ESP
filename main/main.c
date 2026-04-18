#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "wifi_sta.h"
#include "http_server.h"
#include "dht11.h"
#include "led.h"
#include "obstacle.h"

static const char *TAG = "main";

/* 引脚定义 */
#define DHT11_GPIO    GPIO_NUM_23
#define LED_GPIO      GPIO_NUM_2
#define OBSTACLE_GPIO GPIO_NUM_22


/**
 * @brief  障碍物检测任务：初始化模块，循环读取并打印状态
 *
 * @param pvParameters  未使用
 */
static void obstacle_task(void *pvParameters)
{
    if (obstacle_init(OBSTACLE_GPIO) != ESP_OK) {
        ESP_LOGE(TAG, "obstacle init failed, task exit");
        vTaskDelete(NULL);
        return;
    }

    while (1) {
        ESP_LOGI(TAG, "obstacle: %s", obstacle_detected() ? "DETECTED" : "clear");
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}


/**
 * @brief  温湿度传感器任务：初始化 DHT11，定时更新 HTTP 缓存
 *         DHT11 采样周期限制 >= 2s
 *
 * @param pvParameters  未使用
 */
static void sensor_task(void *pvParameters)
{
    /* DHT11 传感器暂时屏蔽（模块待更换），关闭其日志避免干扰 */
    esp_log_level_set("dht11", ESP_LOG_NONE);
    dht11_init(DHT11_GPIO);

    while (1) {
        http_server_update_sensor();
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}


/**
 * @brief  主任务：初始化公共模块，创建各功能子任务后退出
 *
 * @param pvParameters  未使用
 */
static void main_task(void *pvParameters)
{
    ESP_LOGI(TAG, "main_task started");

    /* 初始化 LED */
    if (led_init(LED_GPIO) != ESP_OK) {
        ESP_LOGE(TAG, "led init failed");
    }

    /* 启动 HTTP 服务器 */
    if (http_server_start() != ESP_OK) {
        ESP_LOGE(TAG, "http server failed to start");
    }

    /* 创建各功能子任务 */
    xTaskCreate(obstacle_task, "obstacle_task", 2048, NULL, 4, NULL);
    xTaskCreate(sensor_task,   "sensor_task",   4096, NULL, 4, NULL);

    ESP_LOGI(TAG, "all tasks created, main_task exit");
    vTaskDelete(NULL);
}


/**
 * @brief  ESP-IDF 入口：初始化 NVS，连接 WiFi，创建主任务
 */
void app_main(void)
{
    /* 初始化 NVS */
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    /* 连接 WiFi，超时后继续运行 */
    wifi_station_startup();

    /* 创建主任务 */
    xTaskCreate(main_task, "main_task", 4096, NULL, 5, NULL);
}
