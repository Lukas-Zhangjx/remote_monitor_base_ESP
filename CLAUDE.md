# 项目开发规范

## 分支规范

- 禁止直接在 `main` / `master` 分支上提交代码
- 所有开发必须先创建功能分支：
  - 新功能：`feature/描述`，例如 `feature/uart-init`
  - Bug 修复：`bugfix/描述`，例如 `bugfix/can-timeout`
- 完成开发并提交后，询问用户是否需要 merge 回主分支

## 编码规范

### 文件组织
- 每个独立功能模块必须有独立的 `.c` 和 `.h` 文件
- 禁止将不同功能的代码混写在同一文件中
- 新增大功能时，必须新建对应文件，不得追加到现有功能文件里

### 函数命名
- 格式：`模块名_功能名`，全小写，单词间用下划线
- 示例：
  - `uart_init()` — UART 模块初始化
  - `can_send_frame()` — CAN 模块发送帧
  - `gpio_set_output()` — GPIO 模块设置输出
- 禁止使用无意义的通用名称，如 `init()`、`process()`、`handle()`

### 代码解耦
- 模块间通过函数接口通信，禁止直接访问其他模块的内部变量
- 每个模块的内部变量声明为 `static`，不对外暴露
- 头文件只暴露必要的公共接口，内部实现细节不放入 `.h`

### 注释规范
- 每个函数前必须写注释，说明功能、参数、返回值
- 关键逻辑行内写行注释，说明为什么这样做
- 模块文件顶部写模块说明注释（功能概述、依赖关系）
- 桩函数必须注释标明 `TODO` 及预期实现内容

### 嵌入式超时策略
- **禁止使用 `portMAX_DELAY` 或任何形式的永久阻塞等待**
- 所有阻塞调用必须设置有限超时，并处理超时返回值：
  - `xEventGroupWaitBits()` → 用 `pdMS_TO_TICKS(N)` 替代 `portMAX_DELAY`
  - `xQueueReceive()` / `xSemaphoreTake()` → 同上
  - `wait_for_level()` 等自旋等待 → 必须有计数器上限
- 超时后必须有明确处理：打印错误日志 + 走降级路径（跳过、重试或报错返回），不得卡死
- 示例：
  ```c
  /* 错误：永久阻塞 */
  xEventGroupWaitBits(group, BIT0, pdFALSE, pdFALSE, portMAX_DELAY);

  /* 正确：有限超时 + 超时处理 */
  EventBits_t bits = xEventGroupWaitBits(group, BIT0, pdFALSE, pdFALSE, pdMS_TO_TICKS(5000));
  if (!(bits & BIT0)) {
      ESP_LOGE(TAG, "wait timeout, fallback");
  }
  ```

### 其他
- 头文件必须有 include guard：
  ```c
  #ifndef MODULE_NAME_H
  #define MODULE_NAME_H
  // ...
  #endif /* MODULE_NAME_H */
  ```

## Commit 规范

- 每个 commit 只做一件事
- commit message 格式：`类型: 简短描述`
  - `feat: add uart dma support`
  - `fix: resolve can bus timeout issue`
  - `refactor: split sensor module into separate files`
  - `docs: update readme with build instructions`
- message 使用英文，动词开头，小写
