#ifndef __BUTTON_H
#define __BUTTON_H

  #include "stm32f10x.h"
  #include "FreeRTOS.h"
  #include "task.h"

  #define BTN_LEFT   ((uint32_t)0x01)
  #define BTN_RIGHT  ((uint32_t)0x02)

  void Button_Init(TaskHandle_t displayTaskHandle);
  TaskHandle_t Button_GetDisplayTaskHandle(void);

  #define Button_IsLeftPressed()   ((GPIOB->IDR & GPIO_Pin_0) == 0)
  #define Button_IsRightPressed()  ((GPIOB->IDR & GPIO_Pin_1) == 0)

#endif
