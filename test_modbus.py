# -*- coding: utf-8 -*-
"""
Modbus RTU 从机测试脚本

用法：
  1. 先关闭串口助手里的 COM9（一个串口同一时间只能被一个程序占用）
  2. 让 STM32 保持运行
  3. 在项目目录下执行：python test_modbus.py

期望输出：
  光照: 1674
  温度: 1836
"""

from pymodbus.client import ModbusSerialClient

PORT = 'COM9'
BAUDRATE = 115200
SLAVE_ID = 1
START = 0
COUNT = 2

client = ModbusSerialClient(
    port=PORT,
    baudrate=BAUDRATE,
    timeout=1,
)

if not client.connect():
    print("串口打开失败：%s 是否被串口助手占用？" % PORT)
    raise SystemExit(1)

try:
    result = client.read_holding_registers(
        address=START,
        count=COUNT,
        device_id=SLAVE_ID,      # pymodbus 3.16 用 device_id（旧版叫 slave / unit）
    )

    if result.isError():
        print("读取失败：", result)
    else:
        print("光照:", result.registers[0])
        print("温度:", result.registers[1])
        print("原始列表:", result.registers)
finally:
    client.close()
