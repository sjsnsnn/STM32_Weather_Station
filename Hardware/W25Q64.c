 #include "W25Q64.h"
  #include "SPI.h"

  #define W25Q64_CS_LOW()   GPIO_ResetBits(GPIOB, GPIO_Pin_12)
  #define W25Q64_CS_HIGH()  GPIO_SetBits(GPIOB, GPIO_Pin_12)

  void W25Q64_Init(void)
  {
      RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);

      GPIO_InitTypeDef GPIO_InitStructure;
      GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_Out_PP;
      GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_12;
      GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
      GPIO_Init(GPIOB, &GPIO_InitStructure);

      W25Q64_CS_HIGH();
      SPI2_Init();
  }
  
   uint32_t W25Q64_ReadID(void)
  {
      uint32_t id = 0;

      W25Q64_CS_LOW();
      SPI2_SwapByte(0x9F);                              /* JEDEC ID 命令 */
      id |= (uint32_t)SPI2_SwapByte(0xFF) << 16;
      id |= (uint32_t)SPI2_SwapByte(0xFF) << 8;
      id |= (uint32_t)SPI2_SwapByte(0xFF);
      W25Q64_CS_HIGH();

      return id;
  }

   uint8_t W25Q64_ReadStatus1(void)
  {
      uint8_t status;

      W25Q64_CS_LOW();
      SPI2_SwapByte(0x05);              /* Read Status Register 1 */
      status = SPI2_SwapByte(0xFF);
      W25Q64_CS_HIGH();

      return status;
  }
  
  static void W25Q64_WriteEnable(void)
  {
      W25Q64_CS_LOW();
      SPI2_SwapByte(0x06);              /* Write Enable */
      W25Q64_CS_HIGH();
  }
  
  static uint8_t W25Q64_WaitBusy(void)
  {
      uint32_t timeout = 1000000;

      while ((W25Q64_ReadStatus1() & 0x01) != 0)   /* WIP = bit0 */
      {
          if (--timeout == 0)
              return 1;                                /* 超时 */
      }
      return 0;
  }
  
  void W25Q64_SectorErase(uint32_t addr)
  {
      W25Q64_WriteEnable();

      W25Q64_CS_LOW();
      SPI2_SwapByte(0x20);              /* Sector Erase, 4KB */
      SPI2_SwapByte((uint8_t)(addr >> 16));
      SPI2_SwapByte((uint8_t)(addr >> 8));
      SPI2_SwapByte((uint8_t)addr);
      W25Q64_CS_HIGH();

      W25Q64_WaitBusy();
  }
  
  void W25Q64_PageProgram(uint32_t addr, const uint8_t *buf, uint16_t len)
  {
      uint16_t i;

      if (buf == 0 || len == 0 || len > 256)
          return;

      W25Q64_WriteEnable();

      W25Q64_CS_LOW();
      SPI2_SwapByte(0x02);              /* Page Program */
      SPI2_SwapByte((uint8_t)(addr >> 16));
      SPI2_SwapByte((uint8_t)(addr >> 8));
      SPI2_SwapByte((uint8_t)addr);

      for (i = 0; i < len; i++)
          SPI2_SwapByte(buf[i]);

      W25Q64_CS_HIGH();

      W25Q64_WaitBusy();
  }

   void W25Q64_ReadData(uint32_t addr, uint8_t *buf, uint16_t len)
  {
      uint16_t i;

      if (buf == 0 || len == 0)
          return;

      W25Q64_CS_LOW();
      SPI2_SwapByte(0x03);              /* Read Data */
      SPI2_SwapByte((uint8_t)(addr >> 16));
      SPI2_SwapByte((uint8_t)(addr >> 8));
      SPI2_SwapByte((uint8_t)addr);

      for (i = 0; i < len; i++)
          buf[i] = SPI2_SwapByte(0xFF);

      W25Q64_CS_HIGH();
  }
