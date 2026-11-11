# -*- coding: utf-8 -*-
"""
W2.5 第 4 步：开环占空比扫描，测最大转速

用途：
  1. 配合 main.c 的 MOTOR_OPEN_LOOP_TEST = 1 使用；
  2. 依次写入 20/50/80/100/0 到 Modbus 0x0005；
  3. 每个占空比保持 3 秒，让你在 COM10 上读 printf 的 rpm。

运行前：
  1. 关闭串口助手里占用 COM9 的窗口；
  2. 板子运行，Modbus 接 COM9，printf 接 COM10（115200）；
  3. python test/w2.5-step4-pid/test_motor_sweep.py

注意：
  - 100% 时如果板子复位/串口断开，说明 5V 电源带不动，改用 80% 数据；
  - 脚本结束会自动写 0 停电机。
"""

import sys
import time

from pymodbus.client import ModbusSerialClient

PORT = "COM9"
BAUDRATE = 115200
SLAVE_ID = 1
REG = 5          # 0x0005 目标速度
HOLD_SECONDS = 3.0

STEPS = [
    ("20%", 20),
    ("50%", 50),
    ("80%", 80),
    ("100%", 100),
]


def write_speed(client, value):
    result = client.write_register(
        address=REG,
        value=value,
        device_id=SLAVE_ID,
    )
    if result.isError():
        print("  写入 %d 失败: %s" % (value, result))
        return False
    return True


def read_speed(client):
    result = client.read_holding_registers(
        address=REG,
        count=1,
        device_id=SLAVE_ID,
    )
    if result.isError():
        return None
    return result.registers[0]


def main():
    client = ModbusSerialClient(port=PORT, baudrate=BAUDRATE, timeout=2)
    if not client.connect():
        print("串口打开失败: %s 是否被串口助手占用？" % PORT)
        return 1

    try:
        print("=" * 60)
        print("开环占空比扫描：请同时观察 COM10 上的 rpm 打印")
        print("=" * 60)

        for name, value in STEPS:
            print("\n[%s] 写入 0x0005 = %d ..." % (name, value))
            if not write_speed(client, value):
                break

            readback = read_speed(client)
            print("  读回: %s" % readback)

            print("  保持 %.0f 秒，请记录 COM10 最后 2~3 次 rpm" % HOLD_SECONDS)
            time.sleep(HOLD_SECONDS)

    finally:
        # 无论成功与否，退出前都发 0，避免电机一直转
        print("\n写入 0 停止电机 ...")
        write_speed(client, 0)
        client.close()
        print("脚本结束。")

    print("\n记录完成后，把 20/50/80/100%% 的 rpm 发我。")
    return 0


if __name__ == "__main__":
    sys.exit(main())
