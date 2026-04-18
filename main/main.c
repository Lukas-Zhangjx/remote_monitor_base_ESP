#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "wifi_sta.h"
#include "http_server.h"
#include "dht11.h"
#include "led.h"
#include "obstacle.h"

static const char *TAG = "main";

/* DHT11 DATA 引脚，接 GPIO23 */
#define DHT11_GPIO  GPIO_NUM_23

/* LED 引脚，接 GPIO2 */
#define LED_GPIO    GPIO_NUM_2

/* 障碍物传感器 OUT 引脚，接 GPIO22 */
#define OBSTACLE_GPIO  GPIO_NUM_22

/**
 * @brief  主任务：WiFi 连接完成后启动 HTTP 服务器，进入主循环
 *
 * @param pvParameters  FreeRTOS 任务参数（未使用）
 */
static void main_task(void *pvParameters)
{
    ESP_LOGI(TAG, "main_task started");

    /* 初始化 LED，配置 GPIO2 为推挽输出，默认熄灭 */
    if (led_init(LED_GPIO) != ESP_OK) {
        ESP_LOGE(TAG, "led init failed");
    }


    /* 初始化障碍物传感器 */
    if (obstacle_init(OBSTACLE_GPIO) != ESP_OK) {
        ESP_LOGE(TAG, "obstacle sensor init failed");
    }

    /* 初始化 DHT11，dht11_init 内部会等待 1s 让传感器稳定 */
    if (dht11_init(DHT11_GPIO) != ESP_OK) {
        ESP_LOGE(TAG, "dht11 init failed");
        /* 初始化失败不退出，HTTP 服务器仍可正常运行，传感器显示 0 */
    }

    /* 启动 HTTP 服务器，对外提供页面与 API */
    if (http_server_start() != ESP_OK) {
        ESP_LOGE(TAG, "http server failed to start, task exit");
        vTaskDelete(NULL);
        return;
    }

    while (1) {
        /* 每 2 秒读取一次 DHT11 并更新缓存（DHT11 采样周期限制 >= 2s） */
        http_server_update_sensor();

        /* 打印障碍物检测状态，验证模块工作是否正常 */
        ESP_LOGI(TAG, "obstacle: %s", obstacle_detected() ? "DETECTED" : "clear");

        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}

/**
 * @brief  ESP-IDF 入口：初始化 NVS，连接 WiFi，创建主任务
 */
void app_main(void)
{
    /* 初始化 NVS，WiFi 凭据存储依赖此组件 */
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    /* 阻塞直到 WiFi 连接成功或超过最大重试次数 */
    wifi_station_startup();

    /* WiFi 就绪后创建主任务 */
    xTaskCreate(main_task, "main_task", 4096, NULL, 5, NULL);
}
