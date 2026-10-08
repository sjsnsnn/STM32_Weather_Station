/* ===== 头文件 ===== */
  #include "stm32f10x.h"
  #include "AD.h"          // ADC DMA 初始化 + ADC_Values[] 数组
  #include "OLED.h"        // OLED 驱动
  #include "Serial.h"      // 串口驱动
  #include <stdio.h>

  /* ===== FreeRTOS 头文件 ===== */
  #include "FreeRTOS.h"
  #include "task.h"
  #include "queue.h"

typedef struct{
	uint16_t light_voltage;   // 光照电压（mV）
    uint16_t ntc_voltage;
}SensorData_t;

QueueHandle_t xDisplayQueue = NULL;
QueueHandle_t xCommQueue    = NULL;

void Task_Sensor(void *pvParameters)
{
      SensorData_t data;
      while (1)
      {
		  uint16_t light_raw = ADC_Values[0];
          uint16_t ntc_raw   = ADC_Values[1];
      
		  uint16_t brightness  = 4096 - light_raw;
          data.light_voltage   = brightness * 3300 / 4096;
          uint16_t ntcness  = 4096 - ntc_raw;
		  data.ntc_voltage     = ntcness * 3300 / 4096;
		  
		  xQueueOverwrite(xDisplayQueue, &data);
          xQueueOverwrite(xCommQueue,&data);
		  
		  vTaskDelay(pdMS_TO_TICKS(200));
	  }
}
  void Task_Display(void *pvParameters)
{
    SensorData_t data;
    while (1)
    {
        // 从显示队列取数据，死等
        if (xQueueReceive(xDisplayQueue, &data, portMAX_DELAY) == pdTRUE)
        {
            OLED_ShowString(2, 1, "L:");
            OLED_ShowNum(2, 3, data.light_voltage, 4);
            OLED_ShowString(2, 8, "mV");
            
            OLED_ShowString(3, 1, "T:");
            OLED_ShowNum(3, 3, data.ntc_voltage, 4);
            OLED_ShowString(3, 8, "mV");
        }
        
    }
}
  
void Task_Comm(void *pvParameters)
{
    SensorData_t data;
    while (1)
    {
        if (xQueueReceive(xCommQueue, &data, portMAX_DELAY) == pdTRUE)
        {
            printf("Light: %d mV | NTC: %d mV\r\n", data.light_voltage, data.ntc_voltage);
        }
        vTaskDelay(pdMS_TO_TICKS(1000)); // 每 1 秒发一次
    }
}



int main(void)
  {
	  NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4);
      
	  Serial_Init();
	  OLED_Init();     // 初始化 OLED（如果你有的话）
      AD_Init(); 	  // 初始化 ADC — 开机调一次就够了
	  
	  OLED_ShowString(1, 1, "RTOSWeather");
      printf("System startup: FreeRTOS Weather Station\r\n");
	  
	  xDisplayQueue = xQueueCreate(1, sizeof(SensorData_t));
      xCommQueue    = xQueueCreate(1, sizeof(SensorData_t));
	  
	  if (xDisplayQueue == NULL || xCommQueue == NULL)
    {
        printf("FATAL: Queue creation failed!\r\n");
        while (1); 
    }
	  
	  xTaskCreate(Task_Sensor,  "Sensor",  128, NULL, 3, NULL);
      xTaskCreate(Task_Display, "Display", 256, NULL, 1, NULL);
      xTaskCreate(Task_Comm,    "Comm",    256, NULL, 2, NULL);
	  
	  vTaskStartScheduler();
	  
	  printf("FATAL: Scheduler failed to start!\r\n");
      while (1);  // 死循环，方便你接调试器看停在哪
  }
	  
