/* ===== 头文件 ===== */
  #include "stm32f10x.h"
  #include "AD.h"          // ADC DMA 初始化 + ADC_Values[] 数组
  #include "OLED.h"        // OLED 驱动
  #include "Serial.h"      // 串口驱动
  #include <stdio.h>
  #include "Button.h"    /* BTN_LEFT, BTN_RIGHT, Button_Init */
  #include "Menu.h"      /* Menu_HandleEvent, Menu_RenderCurrentPage */
  #include "Modbus.h"
  #include "Motor.h"
  #include "Encoder.h"
  #include "PID.h"
  #include "W25Q64.h"
  #include "Param.h"

  /* ===== FreeRTOS 头文件 ===== */
  #include "FreeRTOS.h"
  #include "task.h"
  #include "queue.h"
  
  #define MOTOR_CTRL_PERIOD_MS   20
  #define PID_CLOSED_LOOP_ENABLE   1   /* 1 = 闭环 */
  #define MOTOR_OPEN_LOOP_TEST     0   /* 开环扫描结束 */

typedef struct{
	uint16_t light_voltage;   // 光照电压（mV）
    uint16_t ntc_voltage;
}SensorData_t;

QueueHandle_t xDisplayQueue = NULL;
QueueHandle_t xCommQueue    = NULL;
TaskHandle_t g_sensorHandle  = NULL;
TaskHandle_t g_displayHandle = NULL;
TaskHandle_t g_commHandle    = NULL;
TaskHandle_t g_motorHandle   = NULL;


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
	/* static int16_t encAccum = 0; */   /* 临时停用 EC11 菜单：TIM3 给电机编码器 */

	static uint16_t last_saved;
    static uint16_t pending = 0;
    static uint8_t  has_pending = 0;     /* 1 = pending 里有有效值 */
    static TickType_t last_change = 0;
    uint16_t threshold;

    last_saved = Param_GetLightThreshold();
	
    Menu_RenderCurrentPage(0, 0, 0);  /* 开机画默认页面（光照页） */

    while (1)
    {
		Modbus_Poll();
		
		threshold = Modbus_GetRegister(MODBUS_THRESHOLD_REG);

  if (threshold != last_saved)
  {
      if (!has_pending || threshold != pending)
      {
          /* 第一次出现新值，或值又变了：重新计时 */
          pending = threshold;
          has_pending = 1;
          last_change = xTaskGetTickCount();
      }
      else if ((xTaskGetTickCount() - last_change) >= pdMS_TO_TICKS(1000))
      {
          /* 1 秒内没再变化，才写 Flash */
          if (Param_SetLightThreshold(threshold))
          {
              last_saved = threshold;
              has_pending = 0;
              printf("Param saved: %d\r\n", threshold);
          }
          else
          {
              printf("Param save failed: %d\r\n", threshold);
              last_change = xTaskGetTickCount();   /* 1 秒后重试 */
          }
      }
  }
  else
  {
      has_pending = 0;
  }
		
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
		
#if 0   /* ===== EC11 旋钮翻页（临时停用：TIM3 已给电机编码器）===== */
                /* ===== EC11 旋钮翻页 ===== */
    encAccum += Encoder_Get();

  if (encAccum >= 4 || encAccum <= -4)
  {
      uint8_t changed = 0;

      if (encAccum >= 4)
      {
          encAccum -= 4;
          changed = Menu_HandleEvent(BTN_RIGHT);
      }
      else
      {
          encAccum += 4;
          changed = Menu_HandleEvent(BTN_LEFT);   /* 注意：LEFT，不是 RIGHT */
      }

      if (changed)
      {
          OLED_Clear();
          Menu_RenderCurrentPage(data.light_voltage,
                                 data.ntc_voltage,
                                 xTaskGetTickCount() / 1000);
      }
   }
#endif  
    }
}

  
void Task_Comm(void *pvParameters)
  {
      SensorData_t data;
      uint32_t report_cnt = 0;

      while (1)
      {
          if (xQueueReceive(xCommQueue, &data, portMAX_DELAY) == pdTRUE)
          {
              printf("Light: %d mV | NTC: %d mV\r\n",
                     data.light_voltage, data.ntc_voltage);

              /* 每 5 次（约 5 秒）打印一次，避免刷屏 */
              if (++report_cnt >= 5)
              {
                  report_cnt = 0;

                  printf("Stack HWM (words) -> Sensor:%lu, Display:%lu, Comm:%lu, Motor:%lu | FreeHeap:%u B, MinFreeHeap:%uB\r\n",
				         (unsigned long)uxTaskGetStackHighWaterMark(g_sensorHandle),
                         (unsigned long)uxTaskGetStackHighWaterMark(g_displayHandle),
                         (unsigned long)uxTaskGetStackHighWaterMark(g_commHandle),
                         (unsigned long)uxTaskGetStackHighWaterMark(g_motorHandle),
                         (unsigned int)xPortGetFreeHeapSize(),
                         (unsigned int)xPortGetMinimumEverFreeHeapSize());
              }
          }

          vTaskDelay(pdMS_TO_TICKS(1000));
      }
  }



static PID_t g_pid;

void Task_Motor(void *pvParameters)
{
	 #if PID_CLOSED_LOOP_ENABLE
      const float dt = MOTOR_CTRL_PERIOD_MS / 1000.0f;   /* 20ms -> 0.02f */
  #endif
	
	 PID_Init(&g_pid, 0.1f, 0.05f, 0.0f, -100.0f, 100.0f);
     g_pid.deadband = 8.0f;    /* 8 RPM 死区 */
     g_pid.d_alpha   = 0.8f;
	
	TickType_t lastWake=xTaskGetTickCount();
	
	for(;;)
	{
		int16_t percent=(int16_t)Modbus_GetRegister(MODBUS_TARGET_SPEED_REG);
		
		if (percent>100)  percent=100;
		else if (percent<-100) percent=-100;
		
		if(percent==0)
		{
			Motor_SetSpeed(0);
			PID_Reset(&g_pid);
		}
#if PID_CLOSED_LOOP_ENABLE
          else
          {
			  static uint16_t print_div=0;
			  
              float target_rpm   = percent * MOTOR_MAX_RPM / 100.0f;
              float measured_rpm = Encoder_GetRPM(dt);
              float u_pid            = PID_Update(&g_pid, target_rpm, measured_rpm, dt);

			  float u=percent+u_pid;
			  
			  if (u > 100.0f)       u = 100.0f;
              else if (u < -100.0f) u = -100.0f;
			  
              Motor_SetSpeed((int8_t)u);
			  
			  if(++print_div>=50)
			  {
				print_div=0;
				  printf("target=%d, measured=%d, u=%d\r\n",(int)target_rpm,(int)measured_rpm,(int)u);
			  }
          }
  #elif MOTOR_OPEN_LOOP_TEST
          else
          {
               static uint16_t print_div=0;
			  
              Motor_SetSpeed((int8_t)percent);
			  
			  if(++print_div>=50)
			  {
				print_div=0;
				  printf("target=%d%%,rpm=%d\r\n",percent,(int)Encoder_GetRPM(1.0f));
			  }
          }
  #else
          else
          {
              Motor_SetSpeed(0);
              PID_Reset(&g_pid);
          }
  #endif
		
		
		vTaskDelayUntil(&lastWake,pdMS_TO_TICKS(MOTOR_CTRL_PERIOD_MS));
	}
}

static void W25Q64_Test(void)
  {
     uint8_t wr[16] = {'W','2','5','Q','6','4',' ','T','E','S','T',' ','1','2','3','4'};
      uint8_t rd[16];
      uint8_t i;
      uint8_t ok = 1;

      printf("W25Q64 test: erase sector 0\r\n");
      W25Q64_SectorErase(0x000000);

      /* 第 1 次读：擦除后，应该全是 FF */
      W25Q64_ReadData(0x000000, rd, 16);
      printf("after erase: ");
      for (i = 0; i < 16; i++)
          printf("%02X ", rd[i]);
      printf("\r\n");

      printf("page program 16 bytes\r\n");
      W25Q64_PageProgram(0x000000, wr, 16);

      /* 第 2 次读：写入后，应该等于 wr */
      W25Q64_ReadData(0x000000, rd, 16);
      printf("read back:   ");
      for (i = 0; i < 16; i++)
          printf("%02X ", rd[i]);
      printf("\r\n");

      for (i = 0; i < 16; i++)
      {
          if (rd[i] != wr[i])
          {
              ok = 0;
              break;
          }
      }

      printf("W25Q64 test: %s\r\n", ok ? "PASS" : "FAIL");
  }

int main(void)
  {
	  NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4);
      
	  Serial_Init();
	  
	  W25Q64_Init();
      printf("W25Q64 ID: %06X\r\n", (unsigned int)W25Q64_ReadID());
      /* W25Q64_Test(); */   
	  
	  OLED_Init();     // 初始化 OLED（SSD1306，GPIO 软件模拟 I2C）
      AD_Init(); 	  // 初始化 ADC — 开机调一次就够了
	  Modbus_Init();
	   
	   if (!Param_Init())
  {
      printf("Param init failed (defaults used)\r\n");
  }

  Modbus_SetRegister(MODBUS_THRESHOLD_REG, Param_GetLightThreshold());
  printf("Light threshold = %d\r\n", Param_GetLightThreshold());
	  
	  OLED_ShowString(1, 1, "RTOSWeather");
      printf("System startup: FreeRTOS Weather Station\r\n");
	  
	  xDisplayQueue = xQueueCreate(1, sizeof(SensorData_t));
      xCommQueue    = xQueueCreate(1, sizeof(SensorData_t));
	  
	  if (xDisplayQueue == NULL || xCommQueue == NULL)
    {
        printf("FATAL: Queue creation failed!\r\n");
        while (1);
    }

	  

	 if (xTaskCreate(Task_Sensor,  "Sensor",  64,  NULL, 3, &g_sensorHandle) != pdPASS)
  {
      printf("Task create failed: Sensor\r\n");
      while (1);
  }

  if (xTaskCreate(Task_Display, "Display", 192, NULL, 1, &g_displayHandle) != pdPASS)
  {
      printf("Task create failed: Display\r\n");
      while (1);
  }

  if (xTaskCreate(Task_Comm,    "Comm",    112, NULL, 2, &g_commHandle) != pdPASS)
  {
      printf("Task create failed: Comm\r\n");
      while (1);
  }

  if (xTaskCreate(Task_Motor,   "Motor",   96,  NULL, 2, &g_motorHandle) != pdPASS)
  {
      printf("Task create failed: Motor\r\n");
      printf("Check configTOTAL_HEAP_SIZE.\r\n");
      while (1);
  }
	
          Button_Init(g_displayHandle);
	  
	  Motor_Init();/* 配置 PA4/PA5 方向脚 + TIM1_CH1/PA8 PWM */
	  Encoder_Init();
 /* Motor_SetSpeed(50); */  /* 开环测试结束；等新电机到了再接入任务 */   
	
	  vTaskStartScheduler();
	  
	  printf("FATAL: Scheduler failed to start!\r\n");
      while (1);  // 只在调度器启动失败时才会走到这里；停在此处便于调试定位
  }
