#ifndef __AD_H
#define __AD_H

#include "stm32f10x.h"


/* 通道数量定义 —— 以后想加传感器，改这一个数字就行 */
#define ADC_CHANNEL_COUNT   2

/*
 * 全局数组声明（extern）
 *
 * 大白话：
 * 这个数组就是 DMA 的"收货地址"。
 * ADC 每转换完一个通道的数据，DMA 就自动把值塞进这个数组。
 * main.c 里直接读 ADC_Values[0] 和 ADC_Values[1] 就行，
 * 不用再调任何函数、不用再手动触发转换。
 *
 * 注意事项：
 * - 数组里的值是 12 位 ADC 结果，范围 0~4095
 * - 值是"实时更新"的，ADC 在后台不停地转换，DMA 不停地写
 * - 如果你读到的值偶尔抖动，那是正常的，业务层做滤波或取均值即可
 *
 * 为什么加 volatile？
 * - volatile 告诉编译器："这个变量会被硬件（DMA）在背后偷偷改！"
 * - 不加的话，编译器可能优化掉对数组的读取（以为值没变），
 *   导致你读到的一直是旧值——这是一个经典的嵌入式坑。
 */
extern volatile uint16_t ADC_Values[ADC_CHANNEL_COUNT];

/* 初始化函数 —— 配置 GPIO + DMA + ADC，开机调一次就够 */
void AD_Init(void);

#endif
