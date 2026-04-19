/**
 * @file    light_ctrl.h
 * @brief   灯光自动控制逻辑模块
 *
 * 控制规则：
 *   1. 检测到人体移动 → 自动开灯，30秒无人后自动关闭
 *   2. 每次重新检测到移动 → 重置 30 秒计时器
 *   3. 手动（网页）开灯 → 常亮，不受自动计时影响
 *   4. 手动（网页）关灯 → 立即关闭，清除自动计时
 *
 * 依赖：relay 模块（gpio/relay.h），esp_timer
 */

#ifndef LIGHT_CTRL_H
#define LIGHT_CTRL_H

/**
 * @brief  初始化灯光控制模块，默认关灯
 */
void light_ctrl_init(void);

/**
 * @brief  通知控制模块：检测到人体移动
 *
 * 自动开灯并重置 30 秒倒计时。
 * 应在 io_task 检测到 HC-SR501 触发时调用。
 */
void light_ctrl_on_motion(void);

/**
 * @brief  手动设置灯光状态（来自网页开关）
 *
 * @param on  1 = 手动开，0 = 手动关
 */
void light_ctrl_set_manual(int on);

/**
 * @brief  周期性检查自动关灯计时，应在 io_task 每轮循环调用
 */
void light_ctrl_tick(void);

/**
 * @brief  获取当前灯光实际状态
 *
 * @return 1 = 亮，0 = 灭
 */
int light_ctrl_get_state(void);

#endif /* LIGHT_CTRL_H */
