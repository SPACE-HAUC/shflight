#!/usr/bin/env python3
import ctypes
import smbus
import serial
import signal
import time
import sys
import numpy as np

# Function to handle Ctrl+C


def sigHandler(sig, frame):
    global done
    print("SIGINT Received...")
    sys.stdout.flush()
    done = 1
    return


class lsm9ds1:
    def __init__(self, busnum, xl_addr, mag_addr):
        self.sbus = smbus.SMBus(busnum)
        self.xl_addr = xl_addr
        self.mag_addr = mag_addr

        # register 0x0f contains the ident data
        self.sbus.write_byte(mag_addr, 0x0f)
        ident = self.sbus.read_byte(mag_addr)
        if ident != 0b00111101:
            raise ValueError("[LSM9DS1] Init failed: Device not found")
        # disable accel + gyro
        self.sbus.write_byte_data(xl_addr, 0x10, 0x00)
        self.sbus.write_byte_data(xl_addr, 0x1f, 0x00)
        self.sbus.write_byte_data(xl_addr, 0x20, 0x00)

        # configure magnetometer
        self.sbus.write_byte_data(mag_addr, 0x20, 0b11110100)
        self.sbus.write_byte_data(mag_addr, 0x21, 0x00)
        self.sbus.write_byte_data(mag_addr, 0x22, 0x00)

        return

    def readMag(self):
        bx = self.sbus.read_word_data(self.mag_addr, 40)
        by = self.sbus.read_word_data(self.mag_addr, 42)
        bz = self.sbus.read_word_data(self.mag_addr, 44)
        bx = (ctypes.c_short(bx)).value/6.842
        by = (ctypes.c_short(by)).value/6.842
        bz = (ctypes.c_short(bz)).value/6.842
        return (bx, by, bz)

    def __del__(self):
        self.sbus.close()


if len(sys.argv) < 2:
    print("Usage: sudo ./helmholtz.py <Serial Device File/COM Port>\n\n")
    sys.exit(0)

print("Opened Serial Port")
s = serial.Serial(sys.argv[1], 115200)  # open Arduino device
print("Initialized magnetometer")
mag = lsm9ds1(1, 0x6b, 0x1e)
# Deal with any resetting
time.sleep(2)

count = 0
chn = 0

ofile = open("coil_data.csv", 'w')

global done

done = 0
signal.signal(signal.SIGINT, sigHandler)
for chn in range(3):
    # First ensure that all channels are off
    for x in range(3):
        val = x & 0x0003
        val <<= 12
        val &= 0xf000
        val |= 2048
        val &= 0xffff
        bt = val.to_bytes(2, byteorder='little')
        s.write(bt)
        s.write(bt)
        s.write(bt)
        s.flush()
        s.read()

    for count in range(0, 4096, 50):
        chn = chn & 0x0003  # 2 bit only!
        val = count & 0x0fff  # 12 bit only!
        val |= chn << 12  # shift chn by 12 bits
        val &= 0xffff  # make sure it is 16 bits
        s.write(val.to_bytes(2, byteorder='little'))
        s.write(val.to_bytes(2, byteorder='little'))
        s.write(val.to_bytes(2, byteorder='little'))
        s.flush()
        s.read()
        x = 0.0
        y = 0.0
        z = 0.0
        vx = 0.0
        vy = 0.0
        vz = 0.0
        for i in range(20):
            if done:
                break
            x_, y_, z_ = mag.readMag()
            print(chn, count, x_, y_, z_, sep=',')
            x += x_
            vx += x_*x_
            y += y_
            vy += y_*y_
            z += z_
            vz += z_*z_
            time.sleep(0.05)  # 50 ms sleep at 20 sps
        ofile.write('%d,%d,%f,%f,%f,%f,%f,%f\n' %
                    (chn, count, x/20, y/20, z/20, vx/20, vy/20, vz/20))
        # First ensure that all channels are off
    for x in range(3):
        val = x & 0x0003
        val <<= 12
        val &= 0xf000
        val |= 2048
        val &= 0xffff
        bt = val.to_bytes(2, byteorder='little')
        s.write(bt)
        s.write(bt)
        s.write(bt)
        s.flush()
        s.read()

s.close()
ofile.close()
