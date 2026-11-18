# W2.6：SPI + W25Q64 验证

- 日期：2026-11-18
- MCU：STM32F103C8T6
- Flash：W25Q64，6 针模块，3.3V
- SPI：SPI2
  - SCK：PB13
  - MISO：PB14
  - MOSI：PB15
  - CS：PB12（普通 GPIO，软件片选）
- 时钟：APB1 36MHz / 8 = **4.5MHz**
- 模式：CPOL=0、CPHA=0（Mode 0），MSB first
- 代码：`Hardware/SPI.c/h`、`Hardware/W25Q64.c/h`
- 测试入口：`user/main.c` 的 `W25Q64_Test()`

## 测试内容

1. 读 JEDEC ID（0x9F）；
2. 擦除 4KB 扇区 0（0x20 + 24 位地址 + WIP 忙等待）；
3. 擦除后读 16 字节，预期全 `0xFF`；
4. 页写入 16 字节（0x02），内容为 `W25Q64 TEST 1234`；
5. 回读 16 字节，逐字节比较。

## 串口输出

```text
W25Q64 ID: EF4017
W25Q64 test: erase sector 0
after erase: FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF
page program 16 bytes
read back:   57 32 35 51 36 34 20 54 45 53 54 20 31 32 33 34
W25Q64 test: PASS
```

对应 ASCII：

```
57 32 35 51 36 34 20 54 45 53 54 20 31 32 33 34
 W  2  5  Q  6  4     T  E  S  T     1  2  3  4
```

## 结论

- `EF4017` = Winbond W25Q64，说明 SPI2 接线、CS、SPI 模式、供电都正确；
- 擦除后全 `FF`，说明 Write Enable + Sector Erase + WIP 忙等待正常；
- 回读与写入一致，说明 Page Program + Read Data 正常；
- 当前只验证 **单页内 16 字节**写入，未验证跨页/多页写入；
- 当前每次启动都会擦写扇区 0，**掉电保持未单独验证**；
- `W25Q64_WaitBusy()` 的超时返回值暂未在调用处处理，仅用于练习。

## 待补

- `W25Q64_Write()`：按页拆分，自动处理擦除/写入；
- 掉电保持测试：写入后复位，启动时先读再擦；
- 用逻辑分析仪抓一次 SCK/MOSI/MISO/CS 波形并留档；
- 跨页写入、多页写入、长期可靠性不在本阶段范围。
