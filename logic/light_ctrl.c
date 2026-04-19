/**
 * @file    light_ctrl.c
 * @brief   灯光自动控制逻辑模块实现
 *
 * 状态机：
 *
 *   ┌─────────────────────────────────────────┐
 *   │  manual_on=0, auto_active=0 → 灯灭      │ ← 初始/手动关/超时后
 *   └──────┬──────────────────────────────────┘
 *          │ 检测到移动                手动开
 *          ▼                              ▼
 *   ┌─────────────────┐        ┌──────────────────┐
 *   │ auto_active=1   │        │ manual_on=1      │
 *   │ 灯亮，30s倒计时 │        │ 灯亮，常亮       │
 *   └──────┬──────────┘        └──────────────────┘
 *          │ 超时（30s无移动）          │ 手动关
 *          ▼                            ▼
 *        灯灭                          灯灭
 */

#include "light_ctrl.h"
#include "relay.h"
#include "esp_timer.h"
#include "esp_log.h"

static const char *TAG = "light_ctrl";

/* 自动关灯超时：10 秒 */
#define AUTO_TIMEOUT_US  (10LL * 1000 * 1000)

/* 手动开关状态：1 = 手动常亮，0 = 未手动开 */
static int s_manual_on = 0;

/* 自动模式是否激活 */
static int s_auto_active = 0;

/* 上次检测到移动的时间戳（微秒） */
static int64_t s_last_motion_us = 0;


/**
 * @brief  初始化灯光控制模块，默认关灯
 */
void light_ctrl_init(void)
{
    s_manual_on    = 0;
    s_auto_active  = 0;
    s_last_motion_us = 0;
    relay_set(0);
    ESP_LOGI(TAG, "light ctrl init ok");
}


/**
 * @brief  通知控制模块：检测到人体移动
 */
void light_ctrl_on_motion(void)
{
    s_last_motion_us = esp_timer_get_time();

    if (!s_auto_active) {
        s_auto_active = 1;
        relay_set(1);
        ESP_LOGI(TAG, "motion detected, light ON (auto 30s)");
    } else {
        /* 已经亮着，只重置计时器 */
        ESP_LOGD(TAG, "motion: timer reset");
    }
}


/**
 * @brief  手动设置灯光状态（来自网页开关）
 */
void light_ctrl_set_manual(int on)
{
    s_manual_on = on;

    if (on) {
        /* 手动开：常亮，清除自动计时（防止 30s 后被自动关掉） */
        s_auto_active = 0;
        relay_set(1);
        ESP_LOGI(TAG, "manual ON");
    } else {
        /* 手动关：清除自动状态，立即关灯 */
        s_auto_active = 0;
        relay_set(0);
        ESP_LOGI(TAG, "manual OFF");
    }
}


/**
 * @brief  周期性检查自动关灯计时
 *
 * 应在 io_task 每轮循环（100ms）调用。
 */
void light_ctrl_tick(void)
{
    /* 手动常亮或自动未激活时不需要检查 */
    if (s_manual_on || !s_auto_active) return;

    int64_t elapsed = esp_timer_get_time() - s_last_motion_us;
    if (elapsed >= AUTO_TIMEOUT_US) {
        s_auto_active = 0;
        relay_set(0);
        ESP_LOGI(TAG, "auto timeout, light OFF");
    }
}


/**
 * @brief  获取当前灯光实际状态
 */
int light_ctrl_get_state(void)
{
    return relay_get_state();
}
