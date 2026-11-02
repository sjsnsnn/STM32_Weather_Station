# EC11 菜单集成记录

## 现象

慢转一档（4 计数）时，屏幕无反应；快速转偶尔有反应。

## 根因

`Encoder_Get()` 读后清零。`Task_Display` 每轮读一次；一档的 4 个计数可能被拆成 2 + 2：

- 第 1 轮：`enc = 2`，小于阈值 4 → 忽略 → 清零
- 第 2 轮：`enc = 2`，同样忽略

零头丢失，永远凑不满 4。

## 修复

```c
static int16_t encAccum = 0;

encAccum += Encoder_Get();

if (encAccum >= 4 || encAccum <= -4)
{
    uint8_t changed = 0;

    if (encAccum >= 4)
    {
        encAccum -= 4;
        changed = Menu_HandleEvent(BTN_RIGHT);
    }
    else
    {
        encAccum += 4;
        changed = Menu_HandleEvent(BTN_LEFT);
    }

    if (changed)
        OLED_Clear();

    Menu_RenderCurrentPage(data.light_voltage,
                           data.ntc_voltage,
                           xTaskGetTickCount() / 1000);
}
```

## 边界

- 正反各 2：累加为 0，不触发（正确）
- 一次 8：触发一次，剩 4，下一轮再触发
- 机械抖动多计 1~2：触发后保留余数；必要时加 100nF

## 结果

一档翻一页，按键仍然可用。
