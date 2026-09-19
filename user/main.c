#include "stm32f10x.h"
#include "AD.h"        // 引入我们自己写的头文件
#include "OLED.h"   // 假设你已经有了 OLED 驱动
#include <stdio.h>
#include "Serial.h"
#include "Timer.h"

#define DARK_ADC_THRESHOLD   1000  // ADC原始值下限（低于它变暗）
#define BRIGHT_ADC_THRESHOLD 1200 // ADC原始值上限（高于它变亮）

uint8_t is_dark=0;
uint32_t last_time = 0;

  int main(void)
  {
	  NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
      Serial_Init();
	  OLED_Init();     // 初始化 OLED（如果你有的话）
      AD_Init(); 	  // 初始化 ADC — 开机调一次就够了
	  Timer_Init();
	  
	  uint16_t voltage;
	  uint16_t brightness;
	  uint16_t light_val = AD_GetValue(0);
	  
	  OLED_ShowString(1,1,"Light System");
	  OLED_ShowString(3, 1, "STATUS: BRIGHT"); 
	  printf("System startup: Light sensor ready...\r\n");
	  
      while (1)
      {
		   if (g_sys_tick - last_time >= 1000) 
        {
            last_time = g_sys_tick; // 更新上次执行的时间
            
            // --------- 1秒钟执行一次的核心业务逻辑（全部放在这里面） ---------
          light_val = AD_GetValue(0);
		  if(light_val==0xFFFF)
		  {
			OLED_ShowString(2, 1, "ADC Error!");
			printf("Error: ADC Timeout!\r\n");
		  }
		  else{
			brightness=4096-light_val;
		    voltage=brightness*3300/4096;
		  }
			  
          // light 值越大 →光越亮
          // light 值越小 →光越暗
          OLED_ShowString(2,1,"Val:");
		  OLED_ShowNum(2, 5,voltage,4);
		  OLED_ShowString(2,10,"mV");
		  
		  if(!is_dark&&brightness<DARK_ADC_THRESHOLD)
		  {
			    is_dark=1;
				OLED_ShowString(3,1,"STATUS:DARK");
				printf("Warning: Low light, voltage: %d mV\r\n",voltage);
		  }
          else if(is_dark&&brightness>BRIGHT_ADC_THRESHOLD)
		  {
			  is_dark=0;
			OLED_ShowString(3,1,"STATUS:BRIGHT");
			  printf("Normal: Bright, voltage: %d mV\r\n",voltage);
		  }
		}
      }
  }
