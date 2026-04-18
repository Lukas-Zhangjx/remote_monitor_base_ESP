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


/* ================================================================
 * io_task：GPIO 数字输入类模块
 *   职责：读取所有开关量输入，如障碍检测、按键等
 *   周期：100ms
 * ================================================================ */
static void io_task(void *pvParameters)
{
    /* --- 模块初始化 --- */
    obstacle_init(OBSTACLE_GPIO);

    int last_state = -1; /* 上次状态，-1 表示未初始化 */

    ESP_LOGI(TAG, "io_task started");

    /* --- 主循环 --- */
    while (1) {
        int state = obstacle_detected();

        /* 仅在状态变化时更新，避免频繁刷日志和缓存写入 */
        if (state != last_state) {
            ESP_LOGI(TAG, "obstacle: %s", state ? "DETECTED" : "clear");
            http_server_update_obstacle(state);
            last_state = state;
        }

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}


/* ================================================================
 * sensor_task：模拟/总线传感器类模块
 *   职责：读取所有传感器并更新数据缓存，供 network_task 使用
 *   周期：2000ms（受 DHT11 采样限制）
 * ================================================================ */
static void sensor_task(void *pvParameters)
{
    /* --- 模块初始化 --- */
    esp_log_level_set("dht11", ESP_LOG_NONE); /* DHT11 模块待更换，屏蔽日志 */
    dht11_init(DHT11_GPIO);

    ESP_LOGI(TAG, "sensor_task started");

    /* --- 主循环 --- */
    while (1) {
        /* DHT11 温湿度：读取并写入 HTTP 缓存 */
        http_server_update_sensor();

        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}


/* ================================================================
 * network_task：网络服务类模块
 *   职责：启动并维护所有网络服务，如 HTTP Server、MQTT 等
 * ================================================================ */
static void network_task(void *pvParameters)
{
    /* --- 模块初始化 --- */
    if (http_server_start() != ESP_OK) {
        ESP_LOGE(TAG, "http server failed to start");
    }

    /* HTTP Server 内部由 esp_http_server 驱动，此任务无需主循环 */
    vTaskDelete(NULL);
}


/* ================================================================
 * output_task：输出类模块
 *   职责：驱动所有输出设备，如 LED、OLED 等
 *   周期：根据输出设备需求定义
 * ================================================================ */
static void output_task(void *pvParameters)
{
    /* --- 模块初始化 --- */
    led_init(LED_GPIO);

    ESP_LOGI(TAG, "output_task started");

    /* --- 主循环 --- */
    while (1) {
        /* TODO: LED 状态根据业务逻辑驱动（当前保持初始熄灭状态） */

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}


/* ================================================================
 * app_main：系统入口
 *   职责：初始化系统级组件，创建所有框架任务
 * ================================================================ */
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

    /* 创建框架任务 */
    xTaskCreate(io_task,      "io_task",      2048, NULL, 4, NULL);
    xTaskCreate(sensor_task,  "sensor_task",  4096, NULL, 4, NULL);
    xTaskCreate(network_task, "network_task", 4096, NULL, 5, NULL);
    xTaskCreate(output_task,  "output_task",  2048, NULL, 3, NULL);
}
