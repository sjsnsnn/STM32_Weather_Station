# W2.5 第 4 步：PID 闭环验证

- 日期：2026-11-11
- 电机：JGA25-370，6V，9.6:1，带霍尔编码器
- 驱动：TB6612FNG，TIM1_CH1 / PA8，20 kHz PWM
- 电机电源：USB-TTL 5V → TB6612 VM
- 编码器电源：STM32 3.3V
- 编码器：TIM3 编码器模式 TI12，PA6/PA7
- 实测计数：408.5 计数/输出轴一圈（10 圈 4085，20 圈 8172）
- 控制周期：20 ms（`dt = 0.02f`）
- PID：`Kp = 0.1`，`Ki = 0.05`，`Kd = 0`，死区 8 RPM，`d_alpha = 0.8`
- 前馈：`u = percent + PID 修正`，最终限幅 -100~100
- `MOTOR_MAX_RPM = 425.0f`（5V 实测 100% 空载约 425 RPM）
- 测试脚本：`python test/w2.5-step4-pid/test_motor_hold.py --speed X --hold 10`
- 反馈打印：每 1 秒打印一次 `target / measured / u`，测量窗口 1 秒
- 负载：空载（用户确认）

## 参数背景

先用开环扫描得到 5V 下的占空比-转速关系：

| 占空比 | 稳态 RPM |
|---:|---:|
| 20% | 85 |
| 50% | 217 |
| 80% | 338 |
| 100% | 425 |

线性度约 **4.25 RPM / 1% 占空比**，因此闭环采用：

```c
u = percent + PID_Update(target_rpm, measured_rpm, dt);
```

`percent` 作为前馈，PID 只负责修正。

## 闭环结果

| 目标 | 目标 RPM | 实测 RPM | u | 误差 | 结论 |
|---:|---:|---|---:|---|---|
| 20% | 85 | 80~88（启动超调到 95） | 19~21 | ±5 RPM | 稳定 |
| 50% | 212 | 212~227，多数在 220 | 47~50 | 约 +8 RPM | 稳定 |
| 80% | 340 | 330~345，多数在 337 | 80~82 | ±3~5 RPM | 稳定 |
| 100% | 425 | 411→455→440（10s 内未完全收敛） | 96~100 | 10s 时约 +15 RPM | 饱和边界，趋势收敛 |
| -20% | -85 | -73~-88，多数在 -80 | -20~-23 | ±5~12 RPM | 稳定，方向正确 |

说明：

- 20%、50%、80% 正转和 -20% 反转都能稳定在目标附近；
- `u` 基本等于目标百分比，说明前馈承担了主要输出，PID 只做小修正；
- 实测值以约 7~8 RPM 的台阶跳动，属于电机齿轮/负载造成的转速纹波，被 8 RPM 死区挡住；
- 50% 档存在约 +8 RPM 的稳态误差，处于死区边缘，对风扇类负载可接受；
- 100% 档前馈已经顶到上限 100%，PID 只能向下修正：`u` 从 100 降到 96，`measured` 从超调 455 回落到 440；10 秒内还没完全收敛到 425，但趋势在下降。

## 原始数据

### 20%（目标 85 RPM）

```text
target=85, measured=80, u=21
target=85, measured=95, u=19
target=85, measured=80, u=20
target=85, measured=80, u=20
target=85, measured=88, u=20
target=85, measured=80, u=20
target=85, measured=88, u=20
target=85, measured=88, u=20
target=85, measured=80, u=20
target=85, measured=80, u=20
```

### 50%（目标 212 RPM）

```text
target=212, measured=220, u=50
target=212, measured=227, u=48
target=212, measured=227, u=47
target=212, measured=220, u=48
target=212, measured=220, u=48
target=212, measured=212, u=48
target=212, measured=220, u=48
target=212, measured=220, u=47
target=212, measured=220, u=47
target=212, measured=220, u=47
```

### 80%（目标 340 RPM）

```text
target=340, measured=330, u=82
target=340, measured=337, u=81
target=340, measured=337, u=81
target=340, measured=337, u=81
target=340, measured=345, u=80
target=340, measured=337, u=80
target=340, measured=337, u=80
target=340, measured=337, u=80
target=340, measured=337, u=80
target=340, measured=345, u=80
```

### 100%（目标 425 RPM，边界饱和测试）

```text
target=425, measured=411, u=100
target=425, measured=411, u=100
target=425, measured=411, u=100
target=425, measured=425, u=100
target=425, measured=455, u=99
target=425, measured=440, u=99
target=425, measured=447, u=97
target=425, measured=440, u=97
target=425, measured=440, u=96
target=425, measured=440, u=96
```

### -20%（目标 -85 RPM）

```text
target=-85, measured=-80, u=-20
target=-85, measured=-73, u=-21
target=-85, measured=-80, u=-20
target=-85, measured=-80, u=-21
target=-85, measured=-88, u=-21
target=-85, measured=-80, u=-21
target=-85, measured=-88, u=-21
target=-85, measured=-80, u=-21
target=-85, measured=-80, u=-21
target=-85, measured=-73, u=-23
```

## 传感器观察

- 50%、80%、-20% 测试期间：
  - Light 约 1730~1785 mV，未随电机明显下降；
  - NTC 约 1683~1742 mV，基本稳定。
- 早前一次 20% 测试中出现过 Light 从约 2088 mV 掉到约 1500 mV、停转后恢复的现象。
- 用户把光敏电阻移到固定位置后，数值不再随电机振动跳变；
  **根因是传感器受电机振动/位置影响，不是电源/地线干扰。**

## 结论

1. 前馈 + PID 在 20%、50%、80% 正转和 20% 反转下均能稳定运行。
2. 实测误差基本在 8 RPM 死区内，`u` 没有持续大幅摆动。
3. 编码器标定 408.5 计数/圈、`MOTOR_MAX_RPM = 425` 与开环数据一致。
4. 100% 为饱和边界：`u` 先顶 100，超调到 455，随后 PID 把 `u` 降到 96、`measured` 回落到 440；10 秒内未完全收敛，趋势正确，不建议用 100% 做精确调速。
5. 6V 电池供电尚未测试。
6. 早前的 Light 干扰已定位为传感器受电机振动/位置影响；固定传感器后消失。

## 待补

- 100% 已测：10 秒内未完全收敛；可延长到 20 秒或提高 `Ki` 再验证，当前不影响 20~80% 使用；
- 6V 电池供电后重新扫描最大转速，并更新 `MOTOR_MAX_RPM`；
- 若需要更小的稳态误差：把死区从 8 降到 4，同时给 RPM 加一阶低通，避免追着纹波抖；
- 若 Light 干扰再次出现：先检查光敏电阻的固定方式，再查共地、去耦、PA0 滤波；
- 把最终参数和测试结论同步到开发日志与 Wiki。
