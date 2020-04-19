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

    # Default Arduino serial buffer 64 Byte, so we send 64 bytes and then pausefor a sec
    tot_data_len = len(data)
    tot_counter = 0
    pause_counter = 0

    print("{:3.4f}%".format(100 * tot_counter/tot_data_len), end='')

    for byte in data:
        ser.write(byte.encode("ASCII"))
        tot_counter += 1
        pause_counter += 1
        if pause_counter == 64:
            print("\r{:3.4f}%".format(100 * tot_counter/tot_data_len), end='')
            pause_counter = 0
            time.sleep(0.5)

    print("\r{:0>3.4f}%".format(100 * tot_counter/tot_data_len))

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

    hexdata = ""
    with open(file, 'rb') as fp:
        print("Opening " + file)
        hexdata = fp.read().hex().upper()

    if hexdata == "":
        print("Fail to read the file or the file is empty")
        return

    print("Start writing " + str(len(hexdata)) + " bytes")
    write_all(hexdata)

    return


if __name__ == "__main__":
    # execute only if run as a script
    main()
