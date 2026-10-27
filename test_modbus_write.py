# -*- coding: utf-8 -*-
"""
功能码 06 测试：写入阈值寄存器，再读回验证

用法：
  1. 关掉串口助手里的 COM9
  2. 保持 STM32 运行
  3. 执行：python test_modbus_write.py

期望：
  写入 800  → 读回 [800]
  写入 3000 → 读回 [3000]
"""

from pymodbus.client import ModbusSerialClient

PORT = 'COM9'
BAUDRATE = 115200
SLAVE_ID = 1
REG = 4          # 阈值寄存器（唯一可写的）

client = ModbusSerialClient(
    port=PORT,
    baudrate=BAUDRATE,
    timeout=1,
)

if not client.connect():
    print("串口打开失败：%s 是否被串口助手占用？" % PORT)
    raise SystemExit(1)

try:
    for value in (800, 3000):
        print("=" * 45)
        print("写入 %d 到寄存器 %d ..." % (value, REG))

        w = client.write_register(
            address=REG,
            value=value,
            device_id=SLAVE_ID,
        )
        print("  写响应:", "失败 -> %s" % w if w.isError() else "成功")

        r = client.read_holding_registers(
            address=REG,
            count=1,
            device_id=SLAVE_ID,
        )
        if r.isError():
            print("  读回失败:", r)
        else:
            ok = "OK" if r.registers[0] == value else "不一致！"
            print("  读回: %s   %s" % (r.registers, ok))

finally:
    client.close()
    print("=" * 45)
