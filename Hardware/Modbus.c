#include "Modbus.h"
#include "Serial.h"          /* 借 printf 打调试信息（走 USART1） */
#include <stdio.h>

 /* ===== 私有函数声明（只在 Modbus.c 内部用） ===== */
  static void Modbus_USART2_Init(void);
  static void Modbus_DMA_Init(void);
  static void Modbus_IDLE_Init(void);
  static uint16_t Modbus_CRC16(uint8_t *buf, uint16_t len);
  static void     Modbus_SendBytes(uint8_t *buf, uint16_t len);
  static uint16_t Modbus_BuildException(uint8_t *txBuf, uint8_t func, uint8_t errCode);
  
static volatile uint8_t  rxBuf[MODBUS_BUF_SIZE];   /* 接收缓冲区 */
static volatile uint16_t rxLen      = 0;           /* 这一帧收了多少字节 */
static volatile uint8_t  frameReady = 0;           /* 1 = 收到完整帧了 */

static uint16_t regTable[MODBUS_REG_COUNT];        /* 寄存器表 */

static void Modbus_USART2_Init(void)
  {
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
	
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	
	USART_InitTypeDef USART_InitStructure;
	USART_InitStructure.USART_BaudRate = 115200;
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;
	USART_InitStructure.USART_StopBits = USART_StopBits_1;
	USART_InitStructure.USART_Parity = USART_Parity_No;
	USART_InitStructure.USART_Mode = USART_Mode_Tx | USART_Mode_Rx;
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	USART_Init(USART2, &USART_InitStructure);
	
	USART_Cmd(USART2, ENABLE);
  }

void Modbus_Init(void)
  {
      Modbus_USART2_Init();
	  Modbus_DMA_Init();
	  Modbus_IDLE_Init();
	  
	  regTable[0] = 1674;                 /* 光照：假装 1674 mV */
      regTable[1] = 1836;                 /* 温度：假装 1836 mV */
      regTable[2] = 0;                    /* 运行时间 */
      regTable[3] = MODBUS_SLAVE_ADDR;    /* 从机地址 */
      regTable[4] = 800;                  /* 预留阈值 */
	  
     printf("Modbus: USART2 ready\r\n");
	 uint8_t test[]="123456789";
	 printf("CRC test:0x%04X(expect 4B37)\r\n",Modbus_CRC16(test,9));
  }

  void Modbus_Poll(void)
  {
	  if(!frameReady)return;
	  
	  if (rxLen < 4)
      {
          printf("drop: too short (rxLen=%d)\r\n", rxLen);
          frameReady = 0;
          rxLen = 0;
          return;
      }
	  
	  uint16_t crc_calc=Modbus_CRC16((uint8_t*)rxBuf,rxLen-2);
	  uint16_t crc_recv=rxBuf[rxLen-2]|((uint16_t)rxBuf[rxLen-1]<<8);
	  
	  if (crc_calc != crc_recv)
      {
          printf("drop: CRC error (calc=%04X recv=%04X)\r\n", crc_calc, crc_recv);
          frameReady = 0; 
		  rxLen = 0; 
		  return;
      }
	  
	  if(rxBuf[0]!=MODBUS_SLAVE_ADDR&&rxBuf[0]!=0)
	  {
		printf("drop: not my address (%02X)\r\n", rxBuf[0]);
		  frameReady = 0; 
		  rxLen = 0; 
		  return;
	  }
	  
	 /* 广播（地址 0）：执行但不回应 */
      if (rxBuf[0] == 0)
      {
          printf("broadcast received, no reply\r\n");
          frameReady = 0; rxLen = 0; return;
      }
	  
	  uint16_t func=rxBuf[1];
	  uint8_t txBuf[MODBUS_BUF_SIZE];
	  uint16_t txLen=0;
	  
	  if(func==0x03)
	  {
		uint16_t startAddr=(rxBuf[2]<<8|rxBuf[3]);
		uint16_t count=(rxBuf[4]<<8|rxBuf[5]);
		
		if(count==0||startAddr>=MODBUS_REG_COUNT||count>(MODBUS_REG_COUNT-startAddr))
		{
			txLen=Modbus_BuildException(txBuf,func,0x02);
		}
		else
		{
			uint16_t idx=0;
			txBuf[idx++]=MODBUS_SLAVE_ADDR;
			txBuf[idx++]=0x03;
			txBuf[idx++]=(uint8_t)(count*2);
			
			for(uint16_t i=0;i<count;i++)
			{
				uint16_t v=regTable[startAddr+i];
			    txBuf[idx++]=(v>>8)&0xFF;
				txBuf[idx++]=v&0xFF;
			}
			uint16_t crc=Modbus_CRC16(txBuf,idx);
			txBuf[idx++] = crc & 0xFF;           
            txBuf[idx++] = (crc >> 8) & 0xFF;
			
			txLen=idx;
		}
	
	  }
	  
		else
		{
			txLen = Modbus_BuildException(txBuf, func, 0x01);
		}
	  Modbus_SendBytes(txBuf,txLen);
	  printf("reply %d bytes\r\n", txLen);

      frameReady = 0;
      rxLen = 0;
  }

  static void Modbus_DMA_Init(void)
  {
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);
	  DMA_InitTypeDef DMA_InitStructure;
	  
	  DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)&USART2->DR;
      DMA_InitStructure.DMA_MemoryBaseAddr     = (uint32_t)rxBuf;
      DMA_InitStructure.DMA_DIR                = DMA_DIR_PeripheralSRC;
      DMA_InitStructure.DMA_BufferSize         = MODBUS_BUF_SIZE;
      DMA_InitStructure.DMA_PeripheralInc      = DMA_PeripheralInc_Disable;
      DMA_InitStructure.DMA_MemoryInc          = DMA_MemoryInc_Enable;
      DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
      DMA_InitStructure.DMA_MemoryDataSize     = DMA_MemoryDataSize_Byte;
      DMA_InitStructure.DMA_Mode               = DMA_Mode_Normal;
      DMA_InitStructure.DMA_Priority           = DMA_Priority_Medium;
      DMA_InitStructure.DMA_M2M                = DMA_M2M_Disable;
	  
	  DMA_Init(DMA1_Channel6, &DMA_InitStructure);
	  
	  DMA_Cmd(DMA1_Channel6, ENABLE);
	  
	  USART_DMACmd(USART2, USART_DMAReq_Rx, ENABLE);
  }

  static void Modbus_IDLE_Init(void)
  {
      USART_ITConfig(USART2, USART_IT_IDLE, ENABLE);   /* 使能空闲中断 */

      NVIC_InitTypeDef NVIC_InitStructure;
      NVIC_InitStructure.NVIC_IRQChannel                   = USART2_IRQn;
      NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 3;
      NVIC_InitStructure.NVIC_IRQChannelSubPriority        = 0;
      NVIC_InitStructure.NVIC_IRQChannelCmd                = ENABLE;
      NVIC_Init(&NVIC_InitStructure);
  }

  void Modbus_OnIdleIRQ(void)
  {
	 /* ①算出这一帧收了多少字节 */
      rxLen = MODBUS_BUF_SIZE - DMA_GetCurrDataCounter(DMA1_Channel6);

      /* ②打标志：告诉 Modbus_Poll 有活干了 */
      frameReady = 1;

      /* ③重置 DMA，准备收下一帧 */
      DMA_Cmd(DMA1_Channel6, DISABLE);
      DMA_SetCurrDataCounter(DMA1_Channel6, MODBUS_BUF_SIZE);
      DMA_Cmd(DMA1_Channel6, ENABLE);
  }

  static uint16_t Modbus_CRC16(uint8_t *buf, uint16_t len)
  {
	  uint16_t crc=0xFFFF;
	  
	  for(uint16_t i=0;i<len;i++)
	  {
		crc ^=buf[i];
		  for(uint16_t j=0;j<8;j++)
		  {
			if(crc&0x0001)
				crc=(crc>>1)^0xA001;
			else
				crc>>=1;
		}
	  }
	  return crc;
  }

  
  static void Modbus_SendBytes(uint8_t *buf, uint16_t len)
  {
	for(uint16_t i=0;i<len;i++)
	  {
		USART_SendData(USART2,buf[i]);
		  while(USART_GetFlagStatus(USART2,USART_FLAG_TXE)==RESET);
		  
	  }
	  
  }

  static uint16_t Modbus_BuildException(uint8_t *txBuf, uint8_t func, uint8_t errCode)
  {
	uint16_t idx=0;
	  txBuf[idx++]=MODBUS_SLAVE_ADDR;
	  txBuf[idx++]=func|0x80;
	  txBuf[idx++]=errCode;
	  
	  uint16_t crc=Modbus_CRC16(txBuf,idx);
	  txBuf[idx++]=crc&0xFF;
	  txBuf[idx++]=(crc>>8)&0xFF;
	  
	  return idx;
  }