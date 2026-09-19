#include "stm32f10x.h"

volatile uint32_t g_sys_tick=0;

  void Timer_Init(void)
  {
      RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);

      TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStruct;

      TIM_TimeBaseInitStruct.TIM_Prescaler = 7200-1;
      TIM_TimeBaseInitStruct.TIM_Period = 10-1;
      TIM_TimeBaseInitStruct.TIM_CounterMode = TIM_CounterMode_Up;
      TIM_TimeBaseInitStruct.TIM_ClockDivision = TIM_CKD_DIV1;
      TIM_TimeBaseInitStruct.TIM_RepetitionCounter = 0;
      TIM_TimeBaseInit(TIM2, &TIM_TimeBaseInitStruct);

      TIM_ITConfig(TIM2, TIM_IT_Update, ENABLE);

      NVIC_InitTypeDef NVIC_InitStruct;
      NVIC_InitStruct.NVIC_IRQChannel = TIM2_IRQn;
      NVIC_InitStruct.NVIC_IRQChannelPreemptionPriority = 2;
      NVIC_InitStruct.NVIC_IRQChannelSubPriority = 0;
      NVIC_InitStruct.NVIC_IRQChannelCmd = ENABLE;
      NVIC_Init(&NVIC_InitStruct);

      TIM_Cmd(TIM2, ENABLE);
  }

  void TIM2_IRQHandler(void)
{
    // 检查是不是更新中断触发的
    if (TIM_GetITStatus(TIM2, TIM_IT_Update) == SET)
    {
        g_sys_tick++;  // 每进一次中断，计数加1。这就是你自己的“心跳”
        
        TIM_ClearITPendingBit(TIM2, TIM_IT_Update); // 必须清除标志位！否则会死机
    }
}
