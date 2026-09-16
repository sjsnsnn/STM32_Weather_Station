#include "stm32f10x.h"
#include "AD.h"        // 引入我们自己写的头文件
#include "OLED.h"   // 假设你已经有了 OLED 驱动
#include <stdio.h>
#include "Serial.h"

#define LIGHT_THRESHOLD 1000
  int main(void)
  {
      Serial_Init();
	  OLED_Init();     // 初始化 OLED（如果你有的话）
      AD_Init();          // 初始化 ADC — 开机调一次就够了
	  
	  OLED_ShowString(1,1,"Light System");
	  printf("System startup: Light sensor ready...\r\n");
	  
      while (1)
      {
          uint16_t light_val = AD_GetValue(0);		  // 读通道 0（PA0），拿到 0~4095
		  uint16_t brightness=4096-light_val;
		  uint16_t voltage=brightness*3300/4096;
          // light 值越大 →光越亮
          // light 值越小 →光越暗
          OLED_ShowString(2,1,"Val:");
		  OLED_ShowNum(2, 5,voltage,4);
		  OLED_ShowString(2,10,"mV");
		  
		  if(brightness<LIGHT_THRESHOLD)
		  {
			OLED_ShowString(3,1,"STATUS:DARK");
			  printf("Warning: Low light, voltage: %d mV\r\n",voltage);
		  }
          else
		  {
			OLED_ShowString(3,1,"STATUS:BRIGHT");
			  printf("Normal: Bright, voltage: %d mV\r\n",voltage);
		  }
		  
          // 简单延时
          for (uint32_t i = 0; i < 1000000; i++);
      }
  }
