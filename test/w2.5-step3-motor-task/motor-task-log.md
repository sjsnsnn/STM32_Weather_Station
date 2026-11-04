# W2.5 第 3 步：Modbus 目标转速 + Task_Motor

- 日期：2026-11-04
- 寄存器：0x0005，目标速度 -100~100
- 控制任务：`Task_Motor`，20ms，`vTaskDelayUntil`
- 测试工具：pymodbus 3.16.1，COM9
- 执行器：TB6612 + 130 电机（空载）

## pymodbus 结果

| 写入 | 读回 | 电机 |
|---:|---:|---|
| 50 | 50 | 逆时针 |
| 0xFFCE（-50） | 65486 | 顺时针 |
| 0 | 0 | 停 |

完整输出见 `pymodbus-write.txt`。

## 说明

- 读回 `65486` 是正常的：寄存器是无符号 16 位，-50 = 0xFFCE。
- `reply 7 bytes` = FC03 读响应；`reply 8 bytes` = FC06 写回显。
- 电机方向由 AO1/AO2 接线决定，装风扇时选吹向 NTC 的方向。

## 待修

- `Modbus_GetRegister()` 的边界判断 `index > MODBUS_REG_COUNT` 应为 `index >= MODBUS_REG_COUNT`。
- `Modbus.h` 中 `MODBUS_THRESHOLD_REG` 的注释“唯一可写的寄存器”已过时。

## 结论

功能通过（待修边界判断后提交）。
