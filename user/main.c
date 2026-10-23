/* ===== 头文件 ===== */
  #include "stm32f10x.h"
  #include "AD.h"          // ADC DMA 初始化 + ADC_Values[] 数组
  #include "OLED.h"        // OLED 驱动
  #include "Serial.h"      // 串口驱动
  #include <stdio.h>
  #include "Button.h"    /* BTN_LEFT, BTN_RIGHT, Button_Init */
  #include "Menu.h"      /* Menu_HandleEvent, Menu_RenderCurrentPage */
  #include "Modbus.h"

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
		  
		  
      
		  /* ⚠️ 注意：这里对原始值做了取反（4096 - raw）。
		     所以算出的 *_voltage 不是引脚上的真实电压，而是 3300mV 减它。
		     串口打印和 OLED 上显示的 "mV" 都受这个取反影响。 */
		  uint16_t brightness  = 4096 - light_raw;
          data.light_voltage   = brightness * 3300 / 4096;
          uint16_t ntcness  = 4096 - ntc_raw;
		  data.ntc_voltage     = ntcness * 3300 / 4096;
		  
		  Modbus_SetRegister(0,data.light_voltage);
		  Modbus_SetRegister(1,data.ntc_voltage);
		  Modbus_SetRegister(2,xTaskGetTickCount() / 1000);
		  
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
		Modbus_Poll();
        /*
         * 等显示数据，最多等 100ms。
         * 这里不能用 portMAX_DELAY 死等 —— 否则按键通知要等到
         * 下一帧传感器数据到达才被处理，按键响应会明显迟钝。
         */
        if (xQueueReceive(xDisplayQueue, &data, pdMS_TO_TICKS(100)) == pdTRUE)
        {
            Menu_RenderCurrentPage(data.light_voltage, data.ntc_voltage,
                                   xTaskGetTickCount() / 1000);
        }

        /*
         * 非阻塞检查按键通知（有就处理，没有就跳过）
         *
         * xTaskNotifyWait 四个参数按顺序：
         *   0               → 进入时不清除任何位
         *   0x03            → 退出时清除 bit0 / bit1（消费掉，防止重复处理）
         *   &btnNotifyValue → 通知值输出到这个变量
         *   0               → 超时 0，不等待
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
	  OLED_Init();     // 初始化 OLED（SSD1306，GPIO 软件模拟 I2C）
      AD_Init(); 	  // 初始化 ADC — 开机调一次就够了
	  Modbus_Init();
	  
	  OLED_ShowString(1, 1, "RTOSWeather");
      printf("System startup: FreeRTOS Weather Station\r\n");
	  
	  xDisplayQueue = xQueueCreate(1, sizeof(SensorData_t));
      xCommQueue    = xQueueCreate(1, sizeof(SensorData_t));
	  
	  if (xDisplayQueue == NULL || xCommQueue == NULL)
    {
        printf("FATAL: Queue creation failed!\r\n");
        while (1);
    }

	  /* 用于接住 Display 任务的句柄，后面要交给 Button_Init */
	  TaskHandle_t displayHandle;

	  xTaskCreate(Task_Sensor,  "Sensor",  128, NULL, 3, NULL);
      xTaskCreate(Task_Display, "Display", 256, NULL, 1, &displayHandle);
      xTaskCreate(Task_Comm,    "Comm",    256, NULL, 2, NULL);

	  /* 按键中断里要给 Display 任务发通知，所以把句柄传进去 */
	  Button_Init(displayHandle);

	  vTaskStartScheduler();
	  
	  printf("FATAL: Scheduler failed to start!\r\n");
      while (1);  // 只在调度器启动失败时才会走到这里；停在此处便于调试定位
  }
