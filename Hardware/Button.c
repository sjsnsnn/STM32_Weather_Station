#include "Button.h"

  static TaskHandle_t xDisplayTaskHandle = NULL;

  TaskHandle_t Button_GetDisplayTaskHandle(void)
  {
      return xDisplayTaskHandle;
  }

  void Button_Init(TaskHandle_t displayTaskHandle)
  {
      xDisplayTaskHandle = displayTaskHandle;

      /* 第 1 步：开时钟 */
      RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB | RCC_APB2Periph_AFIO, ENABLE);

      /* 第 2 步：GPIO 配置 */
      GPIO_InitTypeDef GPIO_InitStructure;
      GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_IPU;
      GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_0 | GPIO_Pin_1;
      GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
      GPIO_Init(GPIOB, &GPIO_InitStructure);

      /* 第 3 步：引脚映射 */
      GPIO_EXTILineConfig(GPIO_PortSourceGPIOB, GPIO_PinSource0);
      GPIO_EXTILineConfig(GPIO_PortSourceGPIOB, GPIO_PinSource1);

      /* 第 4 步：EXTI 配置 */
      EXTI_InitTypeDef EXTI_InitStructure;
      EXTI_InitStructure.EXTI_Line    = EXTI_Line0 | EXTI_Line1;
      EXTI_InitStructure.EXTI_Mode    = EXTI_Mode_Interrupt;
      EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling;
      EXTI_InitStructure.EXTI_LineCmd = ENABLE;
      EXTI_Init(&EXTI_InitStructure);

      /* 第 5 步：NVIC 使能中断 */
      NVIC_InitTypeDef NVIC_InitStructure;
      NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
      NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
      NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 10;

      NVIC_InitStructure.NVIC_IRQChannel = EXTI0_IRQn;
      NVIC_Init(&NVIC_InitStructure);

      NVIC_InitStructure.NVIC_IRQChannel = EXTI1_IRQn;
      NVIC_Init(&NVIC_InitStructure);
  }
