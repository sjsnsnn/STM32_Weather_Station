# W2.8：FreeRTOS 栈水位测量与 RAM 优化

- 日期：2026-11-23
- MCU：STM32F103C8T6（20KB SRAM）
- FreeRTOS heap：`configTOTAL_HEAP_SIZE = 15KB`
- 测量 API：`uxTaskGetStackHighWaterMark()`，单位 **word（4 字节）**
- 最坏路径：
  - 电机 50% 运行 60 秒
  - Modbus 读写
  - 光照阈值写 Flash（`Param saved: 1200`）
  - 按键翻页

## 优化前

| 任务 | 配置 | 最小 HWM | 峰值使用 |
|---|---:|---:|---:|
| Sensor | 128 words | 104 | 24 words = 96 B |
| Display | 256 words | 130 | 126 words = 504 B |
| Comm | 256 words | 190 | 66 words = 264 B |
| Motor | 256 words | 196 | 60 words = 240 B |

- 总栈：`128 + 256 + 256 + 256 = 896 words = 3584 B`
- `FreeHeap = 10496 B`，`MinFreeHeap = 10496 B`

## 优化后

| 任务 | 配置 | 最小 HWM | 峰值使用 |
|---|---:|---:|---:|
| Sensor | 64 words | 40 | 24 words = 96 B |
| Display | 192 words | 66 | 126 words = 504 B |
| Comm | 112 words | 46 | 66 words = 264 B |
| Motor | 96 words | 44 | 52 words = 208 B |

- 总栈：`64 + 192 + 112 + 96 = 464 words = 1856 B`
- `FreeHeap = 12224 B`，`MinFreeHeap = 12224 B`

## 结论

- 总栈从 **896 words 降到 464 words**，节省 **432 words ≈ 1728 B**；
- FreeHeap 从 **10496 B 增加到 12224 B**，与栈节省量一致；
- 优化后最小剩余栈：
  - Sensor 40 words
  - Display 66 words
  - Comm 46 words
  - Motor 44 words
- 全部 ≥ 16 words，且保留了 ≥40 words 的安全余量；
- 没有继续压缩：再压会牺牲后续加功能/调试打印的空间；
- `MinFreeHeap` 保持 12224 B，说明没有运行期动态分配/泄漏；
- Display 的最坏路径是 Modbus 读写（HWM 130 → 66），按键路径只降到 208；
- Sensor 最省，Comm 主要是 `printf`，Motor 主要是 PID + 浮点 + 调试打印。

## 原始串口输出（节选）

优化前：

```text
Stack HWM (words) -> Sensor:104, Display:212, Comm:190, Motor:226 | FreeHeap:10496 B, MinFreeHeap:10496B
Stack HWM (words) -> Sensor:104, Display:130, Comm:190, Motor:204 | FreeHeap:10496 B, MinFreeHeap:10496B
Stack HWM (words) -> Sensor:104, Display:130, Comm:190, Motor:196 | FreeHeap:10496 B, MinFreeHeap:10496B
```

优化后：

```text
Stack HWM (words) -> Sensor:40, Display:148, Comm:50, Motor:66 | FreeHeap:12224 B, MinFreeHeap:12224B
Stack HWM (words) -> Sensor:40, Display:66, Comm:46, Motor:44 | FreeHeap:12224 B, MinFreeHeap:12224B
```

## 未验证

- 长时间运行（数小时）后的栈水位；
- 新增任务/队列/动态分配后的堆压力；
- 中断嵌套更深时的 MSP 栈；
- 不同编译器优化等级下的栈用量变化。
