#ifndef __TIMER_H
#define __TIMER_H

  #include "stm32f10x.h"
	
  extern volatile uint32_t g_sys_tick; 
  void Timer_Init(void);

  #endif
