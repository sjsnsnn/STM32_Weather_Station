# STM32 Weather Station — Claude Code 工作规范

## 知识库位置

本项目的知识库位于：`D:/KnowledgeVault/`

核心目录：
- `D:/KnowledgeVault/04-Wiki/` — 编译后的结构化百科文章（面试复习用）
- `D:/KnowledgeVault/01-Projects/STM32-Weather-Station/` — 项目笔记
- `D:/KnowledgeVault/00-Inbox/` — 草稿箱

## 规则1：每次启动时自动加载 Wiki 上下文

每次新会话开始时，先读取以下文件来恢复项目上下文：

```
D:/KnowledgeVault/01-Projects/STM32-Weather-Station/项目总览.md
```

然后再根据当前任务需要，读取 `04-Wiki/` 下相关的百科文章。

例如：
- 修改 ADC 相关代码 → 读 `04-Wiki/ADC-DMA多通道采样.md`
- 修改串口相关代码 → 读 `04-Wiki/串口通信.md`
- 修改 FreeRTOS 任务 → 读 `04-Wiki/FreeRTOS任务调度.md`

## 规则2：完成代码修改后自动生成/更新 Wiki

当完成一个模块的代码编写或重大修改后，自动执行以下操作：

1. 读取刚修改的代码文件
2. 在 `D:/KnowledgeVault/04-Wiki/` 下找到对应的 Wiki 文章（没有就新建）
3. 更新 Wiki 文章，保持以下结构：
   - 摘要（一段话概括）
   - 核心原理（初始化流程、工作原理）
   - 关键代码解释（带中文注释的代码片段）
   - 与其他知识的关联（`[[双向链接]]`）
   - 面试要点表格（问题 + 参考答案）

不需要用户主动要求，代码写完就自动更新 Wiki。

## 规则3：用户是来学习的

用户是大三学生，正在准备找实习/工作。所以：
- 代码要有详细的中文注释，解释"为什么这样写"
- 面试可能问到的问题要重点标注
- 遇到的坑和解决方案要记录到 Wiki

## 项目基本信息

- MCU: STM32F103C8T6 (Cortex-M3, 72MHz)
- RTOS: FreeRTOS
- IDE: Keil MDK
- 源码路径: `library/Hardware/` (外设驱动), `user/` (主程序)
- 已有模块: ADC+DMA, USART1, TIM2, I2C OLED, FreeRTOS 三任务
