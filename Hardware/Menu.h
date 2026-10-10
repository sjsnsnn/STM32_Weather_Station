#ifndef __MENU_H
#define __MENU_H

#include "stm32f10x.h"
#include "Button.h"

/*
 * 菜单页面枚举 — 自动从 0 开始编号
 * PAGE_LIGHT  = 0（光照页面）
 * PAGE_TEMP   = 1（温度页面）
 * PAGE_SYSTEM = 2（系统信息页面）
 * PAGE_COUNT  = 3（页面总数，用于取模翻页）
 */
typedef enum {
    PAGE_LIGHT = 0,
    PAGE_TEMP,
    PAGE_SYSTEM,
    PAGE_COUNT
} MenuPage_t;

/* 处理按键事件，返回 1 = 页面变了，0 = 没变 */
uint8_t Menu_HandleEvent(uint32_t event);

/* 渲染当前页面到 OLED */
void Menu_RenderCurrentPage(uint16_t light_mV, uint16_t ntc_mV,
                            uint32_t uptime_sec);

/* 获取当前页面编号 */
MenuPage_t Menu_GetCurrentPage(void);

#endif
