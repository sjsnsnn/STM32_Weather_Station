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
	  
	  OLED_ShowString(1,1,"Weather Station");
	  printf("System startup: Light & NTC sensor ready...\r\n");
	  
      while (1)
      {
		   if (g_sys_tick - last_time >= 1000) 
        {
            last_time = g_sys_tick; // 更新上次执行的时间
            
            // --------- 1秒钟执行一次的核心业务逻辑（全部放在这里面） ---------
          uint16_t light_raw = ADC_Values[0];
	      uint16_t ntc_raw = ADC_Values[1];
			
          uint16_t brightness = 4096 - light_raw; 
          uint16_t light_voltage = brightness * 3300 / 4096;
			
		  
          uint16_t ntc_voltage = ntc_raw * 3300 / 4096;
			
          OLED_ShowString(2, 1, "Light:");
          OLED_ShowNum(2, 7, light_voltage, 4); 
          OLED_ShowString(2, 11, "mV");
            
          OLED_ShowString(3, 1, "NTC:  ");
          OLED_ShowNum(3, 7, ntc_voltage, 4);
          OLED_ShowString(3, 11, "mV");
		  
			if(!is_dark&&brightness<DARK_ADC_THRESHOLD)
		  {
			    is_dark=1;
				OLED_ShowString(4,1,"STATUS:DARK");
				printf("Warning: Low light, voltage: %d mV\r\n",light_voltage);
		  }
          else if(is_dark&&brightness>BRIGHT_ADC_THRESHOLD)
		  {
			  is_dark=0;
			OLED_ShowString(4,1,"STATUS:BRIGHT");
			  printf("Normal: Bright, voltage: %d mV\r\n",light_voltage);
		  }
		  
		  printf("光照: %d mV | NTC温度: %d mV\r\n", light_voltage, ntc_voltage);
		  
		}
      }
  }
