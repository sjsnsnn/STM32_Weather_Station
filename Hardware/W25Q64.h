 #ifndef __W25Q64_H
  #define __W25Q64_H

  #include "stm32f10x.h"

  void W25Q64_Init(void);
  uint32_t W25Q64_ReadID(void);
  uint8_t W25Q64_ReadStatus1(void);
  void W25Q64_SectorErase(uint32_t addr);
  void W25Q64_PageProgram(uint32_t addr, const uint8_t *buf, uint16_t len);
  void W25Q64_ReadData(uint32_t addr, uint8_t *buf, uint16_t len);

  #endif
