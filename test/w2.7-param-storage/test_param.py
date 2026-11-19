# -*- coding: utf-8 -*-
"""
W2.7 参数掉电存储测试

用法：
  python test/w2.7-param-storage/test_param.py --read
  python test/w2.7-param-storage/test_param.py --write 1200
  python test/w2.7-param-storage/test_param.py --write 0

说明：
  - 读写 Modbus 寄存器 4（光照阈值）；
  - 写入后等待 2 秒，让固件的 1 秒防抖保存完成；
  - 然后读回并打印；
  - 要验证掉电保持：写入后按复位，再运行 --read。

运行前：
  - 关闭串口助手里占用 COM9 的窗口；
  - 板子运行，Modbus 接 COM9。
"""

import argparse
import sys
import time

from pymodbus.client import ModbusSerialClient

PORT = "COM9"
BAUDRATE = 115200
SLAVE_ID = 1
REG = 4          # 光照阈值


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--read", action="store_true", help="只读寄存器 4")
    parser.add_argument("--write", type=int, help="写寄存器 4（0~65535）")
    parser.add_argument("--port", default=PORT, help="串口，默认 COM9")
    parser.add_argument("--wait", type=float, default=2.0,
                        help="写入后等待秒数，默认 2")
    args = parser.parse_args()

    if not args.read and args.write is None:
        parser.print_help()
        return 1

    if args.write is not None and (args.write < 0 or args.write > 65535):
        print("write 必须在 0~65535 之间")
        return 1

    client = ModbusSerialClient(port=args.port, baudrate=BAUDRATE, timeout=2)
    if not client.connect():
        print("串口打开失败: %s 是否被串口助手占用？" % args.port)
        return 1

    try:
        if args.write is not None:
            result = client.write_register(
                address=REG,
                value=args.write,
                device_id=SLAVE_ID,
            )
            if result.isError():
                print("写失败: %s" % result)
                return 1

            print("写入寄存器 4 = %d，等待 %.1f 秒让固件保存..." %
                  (args.write, args.wait))
            time.sleep(args.wait)

        result = client.read_holding_registers(
            address=REG,
            count=1,
            device_id=SLAVE_ID,
        )
        if result.isError():
            print("读失败: %s" % result)
            return 1

        print("寄存器 4 = %d" % result.registers[0])

        if args.write is not None:
            print("提示：按复位键，再运行 --read 验证掉电保持。")

    finally:
        client.close()

    return 0


if __name__ == "__main__":
    sys.exit(main())
