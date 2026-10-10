#include "Menu.h"
#include "OLED.h"

/*
 * 当前页面状态 — static 表示只在本文件可见
 * 别的文件想查当前页面，只能调 Menu_GetCurrentPage()
 */
static MenuPage_t currentPage = PAGE_LIGHT;

/* 获取当前页面编号 */
MenuPage_t Menu_GetCurrentPage(void)
{
    return currentPage;
}

/*
 * 处理按键事件（翻页）
 *
 * 参数：event — 按键事件（BTN_LEFT 或 BTN_RIGHT）
 * 返回：1 = 页面变了（调用者应该清屏重绘）
 *       0 = 页面没变
 *
 * 翻页原理（取模运算）：
 *   下一页：(currentPage + 1) % PAGE_COUNT
 *     0 → 1 → 2 → 0（循环）
 *   上一页：(currentPage + PAGE_COUNT - 1) % PAGE_COUNT
 *     0 → 2 → 1 → 0（反向循环）
 *   为什么不直接用 (currentPage - 1) % N？
 *   因为 C 语言对负数取余的结果不确定，用 +N-1 保证操作数为正
 */
uint8_t Menu_HandleEvent(uint32_t event)
{
    MenuPage_t oldPage = currentPage;   /* 先保存旧页面，用于对比 */

    switch (event)
    {
        case BTN_RIGHT:
            /* 下一页：+1 取模 */
            currentPage = (MenuPage_t)((currentPage + 1) % PAGE_COUNT);
            break;

        case BTN_LEFT:
            /* 上一页：+N-1 取模（等价于 -1 但避免负数） */
            currentPage = (MenuPage_t)((currentPage + PAGE_COUNT - 1) % PAGE_COUNT);
            break;

        default:
            break;
    }

    /* 页面变了返回 1，没变返回 0 */
    return (currentPage != oldPage) ? 1 : 0;
}

/*
 * 渲染当前页面到 OLED
 *
 * 参数：
 *   light_mV   — 光照传感器电压（mV）
 *   ntc_mV     — NTC 温度传感器电压（mV）
 *   uptime_sec — 系统运行时间（秒）
 *
 * 根据 currentPage 决定显示哪个页面的内容
 */
void Menu_RenderCurrentPage(uint16_t light_mV, uint16_t ntc_mV,
                            uint32_t uptime_sec)
{
    switch (currentPage)
    {
        case PAGE_LIGHT:
            /* 光照页面：显示标题 + 光照电压 */
            OLED_ShowString(1, 1, "== Light ==");
            OLED_ShowString(3, 1, "L:");
            OLED_ShowNum(3, 3, light_mV, 4);
            OLED_ShowString(3, 8, "mV");
            break;

        case PAGE_TEMP:
            /* 温度页面：显示标题 + NTC 电压 */
            OLED_ShowString(1, 1, "== Temp ==");
            OLED_ShowString(3, 1, "T:");
            OLED_ShowNum(3, 3, ntc_mV, 4);
            OLED_ShowString(3, 8, "mV");
            break;

        case PAGE_SYSTEM:
            /* 系统页面：显示标题 + 运行时间 */
            OLED_ShowString(1, 1, "== System ==");
            OLED_ShowString(3, 1, "Up:");
            OLED_ShowNum(3, 5, uptime_sec, 5);
            OLED_ShowString(3, 11, "s");
            break;

        default:
            break;
    }
}
