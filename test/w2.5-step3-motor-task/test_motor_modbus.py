"""
W2.5 第 3 步：Modbus 0x0005 目标速度验证

运行前：
1. 关闭串口助手里占用 COM9 的窗口
2. 板子运行，Modbus 接 COM9，printf 可以留在 COM10
3. python test_motor_modbus.py
"""

import time
from pymodbus.client import ModbusSerialClient

PORT = "COM9"
BAUD = 115200

c = ModbusSerialClient(port=PORT, baudrate=BAUD, timeout=2)

print("connect:", c.connect())


def show(tag):
    r = c.read_holding_registers(address=5, count=1, device_id=1)
    if r.isError():
        print(f"{tag} 读失败: {r}")
    else:
        print(f"{tag} 读回: {r.registers}")


show("初始")

tests = [
    ("写 50", 50),
    ("写 -50 (0xFFCE)", 0xFFCE),
    ("写 0", 0),
]

for name, value in tests:
    r = c.write_register(address=5, value=value, device_id=1)
    if r.isError():
        print(f"{name} 写失败: {r}")
    else:
        print(f"{name} 写入成功")
    time.sleep(0.3)
    show(name + " 后")

c.close()
