# W2.7：W25Q64 参数掉电存储验证

- 日期：2026-11-19
- 存储：W25Q64，扇区 1（`0x001000`）
- 参数结构：`magic(0xA55A) + version(1) + light_threshold + crc`
- CRC16：poly `0xA001`，初值 `0xFFFF`
- 保存策略：Modbus 寄存器 4 变化后 **1 秒防抖** 保存；值不变不写
- 掉电恢复：`Param_Init()` 读取 + magic/version/CRC 校验；失败回退默认值 800
- 写入方式：擦除扇区 1 → 页写入 `Param_t` → 回读并复用 `Param_Read` 校验

## 测试 1：800 → 1200

启动：

```text
W25Q64 ID: EF4017
Light threshold = 800
```

写入 1200：

```text
Param saved: 1200
```

Modbus 读回：

```text
寄存器 4 = 1200
```

按复位后：

```text
W25Q64 ID: EF4017
Light threshold = 1200
```

结论：1200 成功写入 Flash，复位后由 `Param_Init()` 恢复。

## 测试 2：边界值 0

写入 0：

```text
Param saved: 0
```

按复位后：

```text
Light threshold = 0
```

结论：0 也能被正确识别为“有效待保存值”，说明 `has_pending` 修复有效；
不再把 0 误当成“没有待保存值”。

## 结论

- 光照阈值 800 → 1200 的写入、保存、复位恢复通过；
- 阈值 0 的边界值保存和恢复通过；
- `has_pending` 标志正确区分“0 值”和“没有待保存值”；
- 未测试：magic/CRC 损坏回退、写入过程掉电、双扇区备份；
- 当前单扇区方案在擦写过程中断电可能丢参数，依靠 CRC 校验回退默认值；
  更严格的方案需要 A/B 双扇区备份。

## 原始输出

```text
W25Q64 ID: EF4017
Modbus: USART2 ready
CRC test:0x4B37(expect 4B37)
Light threshold = 800
System startup: FreeRTOS Weather Station

Param saved: 1200
寄存器 4 = 1200

复位后：
W25Q64 ID: EF4017
Light threshold = 1200
System startup: FreeRTOS Weather Station

Param saved: 0

复位后：
Light threshold = 0
System startup: FreeRTOS Weather Station
```
