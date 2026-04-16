/**
 * @file    http_server.c
 * @brief   HTTP 服务器模块实现
 *
 * 使用 ESP-IDF esp_http_server 组件。
 * 静态页面（index.html）通过 CMakeLists EMBED_FILES 嵌入固件，
 * 运行时直接从 flash 读取，无需文件系统。
 *
 * 传感器数据与继电器控制目前使用桩函数占位，
 * 待对应驱动模块完成后替换。
 */

#include <string.h>
#include "esp_err.h"
#include "esp_log.h"
#include "esp_http_server.h"
#include "http_server.h"

static const char *TAG = "http_server";

/* ----------------------------------------------------------------
 * 嵌入的静态资源（由 CMakeLists EMBED_FILES 指令生成）
 * 符号名规则：_binary_<路径中/和.替换为_>_start / _end
 * web/index.html -> _binary_web_index_html_start
 * ---------------------------------------------------------------- */
extern const uint8_t index_html_start[] asm("_binary_web_index_html_start");
extern const uint8_t index_html_end[]   asm("_binary_web_index_html_end");

/* 服务器句柄，用于 stop 时释放 */
static httpd_handle_t s_server = NULL;


/* ================================================================
 *  桩函数区域 — 待传感器/继电器模块完成后替换
 * ================================================================ */

/**
 * @brief  获取温度（桩）
 *
 * TODO: 替换为实际传感器驱动读取逻辑（如 DHT11、AHT20 等）
 *
 * @return 模拟温度值（°C）
 */
static float stub_get_temperature(void)
{
    return 25.0f; /* TODO: 读取真实传感器 */
}

/**
 * @brief  获取湿度（桩）
 *
 * TODO: 替换为实际传感器驱动读取逻辑
 *
 * @return 模拟湿度值（%RH）
 */
static float stub_get_humidity(void)
{
    return 60.0f; /* TODO: 读取真实传感器 */
}

/**
 * @brief  设置继电器状态（桩）
 *
 * TODO: 替换为 GPIO 驱动实现，控制对应继电器引脚电平
 *
 * @param relay_id  继电器编号（当前只有 1）
 * @param state     1 = 吸合（开启），0 = 断开（关闭）
 * @return          实际设置后的状态（与入参一致，驱动实现后应读回确认）
 */
static int stub_set_relay(int relay_id, int state)
{
    /* TODO: gpio_set_level(RELAY1_GPIO, state); */
    ESP_LOGI(TAG, "relay %d -> %s (stub)", relay_id, state ? "ON" : "OFF");
    return state;
}


/* ================================================================
 *  URI 处理器
 * ================================================================ */

/**
 * @brief  GET / — 返回嵌入的 index.html
 *
 * @param req  HTTP 请求句柄
 * @return ESP_OK
 */
static esp_err_t handler_get_index(httpd_req_t *req)
{
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req,
                    (const char *)index_html_start,
                    index_html_end - index_html_start);
    return ESP_OK;
}

/**
 * @brief  GET /api/sensors — 返回传感器数据 JSON
 *
 * 响应格式：
 *   {"temperature": 25.0, "humidity": 60.0}
 *
 * @param req  HTTP 请求句柄
 * @return ESP_OK
 */
static esp_err_t handler_get_sensors(httpd_req_t *req)
{
    char buf[64];
    float temp = stub_get_temperature();
    float humi = stub_get_humidity();

    /* 手动格式化 JSON，避免引入 cJSON 依赖 */
    snprintf(buf, sizeof(buf),
             "{\"temperature\":%.1f,\"humidity\":%.1f}",
             temp, humi);

    httpd_resp_set_type(req, "application/json");
    httpd_resp_sendstr(req, buf);
    return ESP_OK;
}

/**
 * @brief  POST /api/relay — 解析 JSON 并设置继电器状态
 *
 * 请求体格式：
 *   {"id": 1, "state": 1}   （state: 1=开，0=关）
 *
 * 响应格式：
 *   {"id": 1, "state": 1}   （反映实际执行后的状态）
 *
 * @param req  HTTP 请求句柄
 * @return ESP_OK / ESP_FAIL
 */
static esp_err_t handler_post_relay(httpd_req_t *req)
{
    char buf[64] = {0};

    /* 读取请求体，最多 63 字节防止溢出 */
    int len = httpd_req_recv(req, buf, sizeof(buf) - 1);
    if (len <= 0) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Empty body");
        return ESP_FAIL;
    }

    /* 简单解析 JSON 中的 id 与 state 字段
     * TODO: 若字段增多，改用 cJSON 解析更安全 */
    int relay_id = 0, state = 0;
    if (sscanf(buf, "{\"id\":%d,\"state\":%d}", &relay_id, &state) != 2) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid JSON");
        return ESP_FAIL;
    }

    /* 校验继电器编号（当前只支持 1 路） */
    if (relay_id != 1) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid relay id");
        return ESP_FAIL;
    }

    int actual_state = stub_set_relay(relay_id, state);

    /* 返回实际状态 */
    char resp[32];
    snprintf(resp, sizeof(resp), "{\"id\":%d,\"state\":%d}", relay_id, actual_state);
    httpd_resp_set_type(req, "application/json");
    httpd_resp_sendstr(req, resp);
    return ESP_OK;
}


/* ================================================================
 *  URI 表
 * ================================================================ */

static const httpd_uri_t uri_index = {
    .uri      = "/",
    .method   = HTTP_GET,
    .handler  = handler_get_index,
};

static const httpd_uri_t uri_sensors = {
    .uri      = "/api/sensors",
    .method   = HTTP_GET,
    .handler  = handler_get_sensors,
};

static const httpd_uri_t uri_relay = {
    .uri      = "/api/relay",
    .method   = HTTP_POST,
    .handler  = handler_post_relay,
};


/* ================================================================
 *  公共接口实现
 * ================================================================ */

/**
 * @brief  启动 HTTP 服务器并注册所有路由
 *
 * @return ESP_OK 成功，ESP_FAIL 失败
 */
esp_err_t http_server_start(void)
{
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.lru_purge_enable = true; /* 连接数满时自动淘汰最旧连接 */

    ESP_LOGI(TAG, "starting http server on port %d", config.server_port);

    if (httpd_start(&s_server, &config) != ESP_OK) {
        ESP_LOGE(TAG, "failed to start http server");
        return ESP_FAIL;
    }

    httpd_register_uri_handler(s_server, &uri_index);
    httpd_register_uri_handler(s_server, &uri_sensors);
    httpd_register_uri_handler(s_server, &uri_relay);

    ESP_LOGI(TAG, "http server started");
    return ESP_OK;
}

/**
 * @brief  停止 HTTP 服务器并释放资源
 */
void http_server_stop(void)
{
    if (s_server) {
        httpd_stop(s_server);
        s_server = NULL;
        ESP_LOGI(TAG, "http server stopped");
    }
}
