#include "stm32f10x.h"
#include "AD.h"        // 引入我们自己写的头文件
#include "OLED.h"   // 假设你已经有了 OLED 驱动
#include <stdio.h>
#include "Serial.h"
  int main(void)
  {
      OLED_Init();     // 初始化 OLED（如果你有的话）
      AD_Init();          // 初始化 ADC — 开机调一次就够了
	  Serial_Init();
      while (1)
      {
          uint16_t light = AD_GetValue(0);  // 读通道 0（PA0），拿到 0~4095
		  uint16_t brightnes=4095-light;
          // light 值越大 →光越亮
          // light 值越小 →光越暗
          OLED_ShowNum(1, 1, brightnes, 5);

		  printf("ADC=%d  Brightness=%d\r\n",light ,brightnes);
          // 简单延时
          for (uint32_t i = 0; i < 1000000; i++);
      }
  }
