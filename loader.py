#!/usr/bin/python3

import serial
import time
import sys


PORT = "/dev/ttyUSB0"
BAUD = 115200


def write_all(data):
    ser = serial.Serial(port=PORT, baudrate=BAUD)

    ser.isOpen()
    time.sleep(1)

    ser.flushInput()
    ser.flushOutput()

    time.sleep(2)

    ser.write("<".encode("ASCII"))
    ser.write(data.encode("ASCII"))
    ser.write(">\r\n".encode("ASCII"))

    time.sleep(1)

    out = ""

    while ser.inWaiting() > 0:
        out += ser.read(1).decode()

    if out != "":
        print(out)

    ser.close()


def main():
    if len(sys.argv) < 2:
        print("No input file")
        return

    file = sys.argv[1]
    print("Writing " + file)

    hexdata = ""
    with open(file, 'rb') as fp:
        hexdata = fp.read().hex().upper()

    if hexdata == "":
        print("Fail to read the file or the file is empty")
        return

    write_all(hexdata)

    return


if __name__ == "__main__":
    # execute only if run as a script
    main()
