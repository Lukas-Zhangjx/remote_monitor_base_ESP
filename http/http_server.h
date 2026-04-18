/**
 * @file    http_server.h
 * @brief   HTTP 服务器模块公共接口
 *
 * 基于 ESP-IDF esp_http_server 组件实现。
 * 提供以下功能：
 *   - 提供 index.html 静态页面（GET /）
 *   - 传感器数据 JSON 接口（GET /api/sensors）
 *   - 继电器控制 JSON 接口（POST /api/relay）
 *
 * 依赖：
 *   - esp_http_server
 *   - dht11 驱动（sensor/dht11.h）
 *   - relay  模块（桩，待实现）
 */

#ifndef HTTP_SERVER_H
#define HTTP_SERVER_H

#include "esp_err.h"

/**
 * @brief  启动 HTTP 服务器，注册所有 URI 处理器
 *
 * 应在 WiFi 连接成功（获得 IP）后调用。
 *
 * @return ESP_OK      启动成功
 * @return ESP_FAIL    启动失败
 */
esp_err_t http_server_start(void);

/**
 * @brief  停止 HTTP 服务器并释放资源
 */
void http_server_stop(void);

/**
 * @brief  读取 DHT11 并更新传感器缓存
 *
 * 应在 sensor_task 中每 2 秒以上调用一次（DHT11 采样周期限制）。
 * HTTP handler 直接读缓存，不阻塞请求处理。
 */
void http_server_update_sensor(void);

/**
 * @brief  更新障碍物检测状态缓存
 *
 * 应在 io_task 中检测到状态变化时调用。
 *
 * @param detected  1 = 检测到障碍物，0 = 无障碍物
 */
void http_server_update_obstacle(int detected);

#endif /* HTTP_SERVER_H */
