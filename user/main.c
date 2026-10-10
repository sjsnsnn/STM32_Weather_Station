/* ===== 头文件 ===== */
  #include "stm32f10x.h"
  #include "AD.h"          // ADC DMA 初始化 + ADC_Values[] 数组
  #include "OLED.h"        // OLED 驱动
  #include "Serial.h"      // 串口驱动
  #include <stdio.h>
  #include "Button.h"    /* BTN_LEFT, BTN_RIGHT, Button_Init */
  #include "Menu.h"      /* Menu_HandleEvent, Menu_RenderCurrentPage */

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
    SensorData_t data = {0, 0};       /* 初始化为 0，防止首次显示垃圾值 */
    uint32_t btnNotifyValue = 0;      /* 存放按键通知值 */

    Menu_RenderCurrentPage(0, 0, 0);  /* 开机画默认页面（光照页） */

    while (1)
    {
        /*
         * 改动 A：portMAX_DELAY → pdMS_TO_TICKS(100)
         * 最多等 100ms，等不到也继续往下走检查按键
         */
        if (xQueueReceive(xDisplayQueue, &data, pdMS_TO_TICKS(100)) == pdTRUE)
        {
            Menu_RenderCurrentPage(data.light_voltage, data.ntc_voltage,
                                   xTaskGetTickCount() / 1000);
        }

        /*
         * 改动 B：检查按键通知（不等待，有就有，没有就跳过）
         *
         * xTaskNotifyWait 参数：
         *   0x03            → 退出时清除 bit0 和 bit1
         *   0               → 进入时不清除
         *   &btnNotifyValue → 通知值存到这个变量
         *   0               → 超时 = 0，不等待
         */
        if (xTaskNotifyWait(0, 0x03, &btnNotifyValue, 0) == pdTRUE)
        {
            if (btnNotifyValue & BTN_LEFT)
            {
                if (Menu_HandleEvent(BTN_LEFT))
                    OLED_Clear();    /* 页面变了，先清屏 */
            }
            if (btnNotifyValue & BTN_RIGHT)
            {
                if (Menu_HandleEvent(BTN_RIGHT))
                    OLED_Clear();
            }

            /* 重绘当前页面（用最新的传感器数据） */
            Menu_RenderCurrentPage(data.light_voltage, data.ntc_voltage,
                                   xTaskGetTickCount() / 1000);
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

	  /* 改动 C：声明变量，用于获取 Display 任务句柄 */
	  TaskHandle_t displayHandle;

	  xTaskCreate(Task_Sensor,  "Sensor",  128, NULL, 3, NULL);
      xTaskCreate(Task_Display, "Display", 256, NULL, 1, &displayHandle);
      xTaskCreate(Task_Comm,    "Comm",    256, NULL, 2, NULL);

	  /* 改动 D：初始化按键，传入 Display 任务句柄 */
	  Button_Init(displayHandle);

	  vTaskStartScheduler();
	  
	  printf("FATAL: Scheduler failed to start!\r\n");
      while (1);  // 死循环，方便你接调试器看停在哪
  }
