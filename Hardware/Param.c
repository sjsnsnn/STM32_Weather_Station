  #include "Param.h"
  #include "W25Q64.h"
  #include <stddef.h>

  #define PARAM_ADDR                     0x001000   /* 扇区 1，4KB 对齐 */
  #define PARAM_MAGIC                    0xA55A
  #define PARAM_VERSION                  1
  #define PARAM_DEFAULT_LIGHT_THRESHOLD  800

  typedef struct {
      uint16_t magic;            /* 0xA55A，判断是否初始化过 */
      uint16_t version;          /* 结构体版本 */
      uint16_t light_threshold;  /* 光照阈值 */
      uint16_t crc;              /* 前 6 字节的 CRC16 */
  } Param_t;
  
  static Param_t g_param;
  
   /* 和 Modbus 相同的 CRC16：poly=0xA001，初值 0xFFFF */
  static uint16_t Param_CRC16(const uint8_t *buf, uint16_t len)
  {
      uint16_t crc = 0xFFFF;
      uint16_t i;
      uint8_t j;

      for (i = 0; i < len; i++)
      {
          crc ^= buf[i];
          for (j = 0; j < 8; j++)
          {
              if (crc & 0x0001)
                  crc = (crc >> 1) ^ 0xA001;
              else
                  crc >>= 1;
          }
      }
      return crc;
  }

  /* 从 Flash 读并校验；成功返回 1 */
  static uint8_t Param_Read(Param_t *p)
  {
      uint16_t crc;

      W25Q64_ReadData(PARAM_ADDR, (uint8_t *)p, sizeof(Param_t));

      if (p->magic != PARAM_MAGIC)
          return 0;

      if (p->version != PARAM_VERSION)
          return 0;

      crc = Param_CRC16((const uint8_t *)p, offsetof(Param_t, crc));
      if (crc != p->crc)
          return 0;

      return 1;
  }

   static uint8_t Param_Write(const Param_t *p)
  {
      Param_t tmp = *p;
      Param_t check;

      tmp.crc = Param_CRC16((const uint8_t *)&tmp, offsetof(Param_t, crc));

      W25Q64_SectorErase(PARAM_ADDR);
      W25Q64_PageProgram(PARAM_ADDR, (const uint8_t *)&tmp, sizeof(Param_t));

      /* 回读：先校验 magic/version/CRC 是否自洽 */
      if (!Param_Read(&check))
          return 0;

      /* 再确认读回的是本次写入的数据，而不是旧数据 */
      return (check.crc == tmp.crc) ? 1 : 0;
  }

  
  
  uint16_t Param_GetLightThreshold(void)
  {
      return g_param.light_threshold;
  }
  
   uint8_t Param_SetLightThreshold(uint16_t value)
  {
      Param_t tmp;

      if (value == g_param.light_threshold)
          return 1;

      /* 先复制一份，不动物理内存里的 g_param */
      tmp = g_param;
      tmp.light_threshold = value;

      /* 写 Flash 成功，才更新内存 */
      if (!Param_Write(&tmp))
          return 0;

      g_param = tmp;
      return 1;
  }

   uint8_t Param_Init(void)
  {
      Param_t tmp;

      /* 先尝试从 Flash 读 */
      if (Param_Read(&tmp))
      {
          g_param = tmp;
          return 1;
      }

      /* 读失败：用默认值 */
      tmp.magic = PARAM_MAGIC;
      tmp.version = PARAM_VERSION;
      tmp.light_threshold = PARAM_DEFAULT_LIGHT_THRESHOLD;

      /* 即使写失败，RAM 里也要用默认值 */
      g_param = tmp;

      /* 返回“默认值有没有成功写进Flash”*/
      return Param_Write(&tmp);
  }
