#include "SPI.h"

 void SPI2_Init(void)
 {
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_SPI2, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
	 
	GPIO_InitTypeDef GPIO_InitStructure;
	 
	  /* PB13 SCK */
      GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_AF_PP;
      GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_13;
      GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
      GPIO_Init(GPIOB, &GPIO_InitStructure);

      /* PB14 MISO */
      GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_IN_FLOATING;
      GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_14;
      GPIO_Init(GPIOB, &GPIO_InitStructure);

      /* PB15 MOSI */
      GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_AF_PP;
      GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_15;
      GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
      GPIO_Init(GPIOB, &GPIO_InitStructure);
	  
	   SPI_InitTypeDef SPI_InitStructure;
      SPI_InitStructure.SPI_Direction         = SPI_Direction_2Lines_FullDuplex;
      SPI_InitStructure.SPI_Mode              = SPI_Mode_Master;
      SPI_InitStructure.SPI_DataSize          = SPI_DataSize_8b;
      SPI_InitStructure.SPI_CPOL              = SPI_CPOL_Low;
      SPI_InitStructure.SPI_CPHA              = SPI_CPHA_1Edge;
      SPI_InitStructure.SPI_NSS               = SPI_NSS_Soft;
      SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_8;  /* 36MHz / 8 = 4.5MHz */
      SPI_InitStructure.SPI_FirstBit          = SPI_FirstBit_MSB;
      SPI_InitStructure.SPI_CRCPolynomial     = 7;
      SPI_Init(SPI2, &SPI_InitStructure);

      SPI_Cmd(SPI2, ENABLE);
 }
 
 uint8_t SPI2_SwapByte(uint8_t tx)
 {
	while (SPI_I2S_GetFlagStatus(SPI2, SPI_I2S_FLAG_TXE) == RESET);
      SPI_I2S_SendData(SPI2, tx);

      while (SPI_I2S_GetFlagStatus(SPI2, SPI_I2S_FLAG_RXNE) == RESET);
      return (uint8_t)SPI_I2S_ReceiveData(SPI2);
 }
