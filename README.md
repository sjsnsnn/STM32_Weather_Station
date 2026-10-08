# STM32 Weather Station (基于 FreeRTOS 的桌面气象站)

## 📖 项目简介
本项目基于 STM32F103C8T6 微控制器，搭载 FreeRTOS 实时操作系统，实现了对环境光照强度与温度的实时采集、OLED 屏幕实时显示以及串口数据交互。项目采用模块化编程思想，具备良好的实时性、稳定性和可扩展性。

## 🔌 硬件清单
- MCU: STM32F103C8T6 最小系统板
- 传感器: 光敏传感器模块 (AO 输出)
- 传感器: NTC 热敏电阻模块 (AO 输出)
- 显示模块: 0.96寸 OLED 屏幕 (I2C接口)
- 调试工具: ST-Link V2、USB转TTL模块
- 面包板及杜邦线若干

## 📐 系统架构 (数据流图)
```text
  [传感器层]
  光敏传感器 (PA0) ──┐
                     ├──> [ADC + DMA 自动扫描]
  NTC 传感器 (PA1) ──┘
                            │
                            ▼
                    [Task_Sensor 任务]
                     (周期: 200ms)
                            │
                            ▼ (xQueueOverwrite)
                ┌───────────┴───────────┐
                ▼                       ▼
       [xDisplayQueue]          [xCommQueue]
        (深度:1, 最新值)         (深度:1, 最新值)
                │                       │
                ▼                       ▼
       [Task_Display]           [Task_Comm]
      (I2C OLED 刷新)          (UART 串口打印)
