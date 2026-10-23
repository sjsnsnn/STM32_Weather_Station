# -*- coding: utf-8 -*-
"""
Modbus 连续读取测试

用法：
  1. 关掉串口助手里 COM9 的窗口
  2. 保持 STM32 运行
  3. 执行：python test_modbus_loop.py
  4. 看到提示后，用手遮住光敏电阻 3~5 秒，再移开

期望：光照值明显下降，移开后恢复。
"""

import time
from pymodbus.client import ModbusSerialClient

PORT = 'COM9'
BAUDRATE = 115200
SLAVE_ID = 1
COUNT = 3          # 光照 + NTC + 运行秒数
TIMES = 20         # 读 20 次
INTERVAL = 1.0     # 每次间隔 1 秒

client = ModbusSerialClient(
    port=PORT,
    baudrate=BAUDRATE,
    timeout=1,
)

if not client.connect():
    print("串口打开失败：%s 是否被串口助手占用？" % PORT)
    raise SystemExit(1)

print("=" * 55)
print("开始读取，共 %d 次，每 %.1f 秒一次" % (TIMES, INTERVAL))
print("请在 第 5~10 次 之间，用手遮住光敏电阻")
print("=" * 55)

try:
    for i in range(TIMES):
        result = client.read_holding_registers(
            address=0,
            count=COUNT,
            device_id=SLAVE_ID,
        )

        if result.isError():
            print("[%2d] 读取失败: %s" % (i + 1, result))
        else:
            print("[%2d] 光照=%5d mV   NTC=%5d mV   运行=%4d s" % (
                i + 1,
                result.registers[0],
                result.registers[1],
                result.registers[2],
            ))

        time.sleep(INTERVAL)
finally:
    client.close()
    print("=" * 55)
    print("结束")
