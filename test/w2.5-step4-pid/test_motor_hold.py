# -*- coding: utf-8 -*-
"""
W2.5 第 4 步：给定一个目标速度，保持一段时间后自动停止

用法：
  python test/w2.5-step4-pid/test_motor_hold.py
  python test/w2.5-step4-pid/test_motor_hold.py --speed 50 --hold 5
  python test/w2.5-step4-pid/test_motor_hold.py --speed 0

说明：
  - 默认写 20（目标 85 RPM），保持 5 秒，然后写 0；
  - 运行前关闭串口助手里占用 COM9 的窗口；
  - 同时打开 COM10 看 target/measured/u 打印。
"""

import argparse
import sys
import time

from pymodbus.client import ModbusSerialClient

PORT = "COM9"
BAUDRATE = 115200
SLAVE_ID = 1
REG = 5          # 0x0005 目标速度


def to_register_value(speed):
    """把 -100~100 转成 Modbus 的无符号 16 位值。"""
    if speed < 0:
        return speed + 0x10000
    return speed


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--speed", type=int, default=20,
                        help="目标速度百分比，-100~100，默认 20")
    parser.add_argument("--hold", type=float, default=5.0,
                        help="保持秒数，默认 5")
    parser.add_argument("--port", default=PORT,
                        help="串口，默认 COM9")
    args = parser.parse_args()

    if args.speed < -100 or args.speed > 100:
        print("speed 必须在 -100 ~ 100 之间")
        return 1
    if args.hold < 0:
        print("hold 不能为负")
        return 1

    value = to_register_value(args.speed)

    client = ModbusSerialClient(port=args.port, baudrate=BAUDRATE, timeout=2)
    if not client.connect():
        print("串口打开失败: %s 是否被串口助手占用？" % args.port)
        return 1

    try:
        print("写入 0x0005 = %d ..." % args.speed)
        result = client.write_register(
            address=REG,
            value=value,
            device_id=SLAVE_ID,
        )
        if result.isError():
            print("写失败: %s" % result)
            return 1

        readback = client.read_holding_registers(
            address=REG,
            count=1,
            device_id=SLAVE_ID,
        )
        if readback.isError():
            print("读回失败: %s" % readback)
        else:
            print("读回: %s" % readback.registers)

        print("保持 %.1f 秒，观察 COM10 的 target/measured/u ..." % args.hold)
        time.sleep(args.hold)

    except KeyboardInterrupt:
        print("\n用户中断")

    finally:
        # 无论正常结束还是 Ctrl+C，都写 0 停电机
        print("写入 0 停止电机 ...")
        try:
            client.write_register(address=REG, value=0, device_id=SLAVE_ID)
        except Exception as exc:
            print("写 0 失败: %s" % exc)
        client.close()
        print("已停止。")

    return 0


if __name__ == "__main__":
    sys.exit(main())
