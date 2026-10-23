#ifndef __MODBUS_H
  #define __MODBUS_H

  #include "stm32f10x.h"

  /* ===== 可配置项 ===== */
  #define MODBUS_SLAVE_ADDR   0x01    /* 本从机地址 */
  #define MODBUS_BUF_SIZE     256     /* 收发缓冲区大小（Modbus 最大帧约 256） */
  #define MODBUS_REG_COUNT    5       /* 寄存器表大小 */

  /* ===== 对外接口（只有这两个） ===== */
  void Modbus_Init(void);   /* 开机调用：初始化 USART2 + DMA + 空闲中断 */
  void Modbus_Poll(void);   /* 循环/任务里调用：检查有没有收到完整帧 */
  void Modbus_OnIdleIRQ(void);
  void Modbus_SetRegister(uint8_t index, uint16_t value);
  #endif
