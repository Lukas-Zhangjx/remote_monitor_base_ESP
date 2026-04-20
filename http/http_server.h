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
 * @brief  更新障碍物检测状态缓存（门窗传感器）
 *
 * 应在 io_task 中检测到状态变化时调用。
 *
 * @param detected  1 = 检测到（门关），0 = 未检测到（门开）
 */
void http_server_update_obstacle(int detected);

/**
 * @brief  更新红外传感器状态缓存（人体/移动检测）
 *
 * 应在 io_task 中检测到状态变化时调用。
 *
 * @param detected  1 = 检测到移动，0 = 无移动
 */
void http_server_update_ir(int detected);

/**
 * @brief  更新光敏传感器缓存
 *
 * @param percent  光照百分比 0-100（100=最亮）
 * @param raw      ADC 原始值 0-4095（用于标定）
 * @param digital  1 = 有光，0 = 暗
 */
void http_server_update_light(int percent, int raw, int digital);

#endif /* HTTP_SERVER_H */
