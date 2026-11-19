#ifndef __PARAM_H
  #define __PARAM_H

  #include "stm32f10x.h"

  uint8_t Param_Init(void);
  uint16_t Param_GetLightThreshold(void);
  uint8_t Param_SetLightThreshold(uint16_t value);

  #endif
