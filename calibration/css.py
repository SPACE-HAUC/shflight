#!/usr/bin/env python3

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

# Function to select channels on I2C Mux
def channel(s, chn):
    if -0 <= chn <= 7:
        s.write_byte_data(0x70, 0x00, 1 << chn)
    else:
        s.write_byte_data(0x70, 0x00,0x0)
    return

# Function to initialize TSL2561
def tsl2561_init(s, chn, addr):
    if addr not in [0x29, 0x39, 0x49]:
        raise AttributeError("Address out of bound")
    channel(s, chn) # access the selected channel
    s.write_byte_data(addr, 0x80, 0x03) # Config register reads 0x03 for powerup    time.sleep(0.1) # sleep for 100 ms before ensuring the device is on
    recv = s.read_byte_data(addr, 0x80)
    if recv != 0x33:
        print("Init: Received 0x%02x as power state from %d->%d, returning."%(recv, chn, addr))
        return -1
    s.write_byte_data(addr, 0x81, 0x00) # Config register reads 0x00 for continuous conversion at 13.7 ms rate
    time.sleep(0.1)
    recv = s.read_byte_data(addr, 0x81)
    if recv != 0x00:
        print("Init: Received 0x%02x as config state from %d->%d, returning."%(recv, chn, addr))
        return -1
    return 1

def tsl2561_destroy(s, chn, addr):
    if addr not in [0x29, 0x39, 0x49]:
        raise AttributeError("Address out of bound")
    channel(s, chn)
    s.write_byte_data(addr, 0x80, 0x00)

def tsl2561_measure(s, chn, addr):
    if addr not in [0x29, 0x39, 0x49]:
        raise AttributeError("Address out of bound")
    channel(s, chn) # access the selected channel
    ch0 = s.read_word_data(addr, 0xac) # broadband data
    ch1 = s.read_word_data(addr, 0xae) # ir data
    return (ch0, ch1)

def tsl2561_get_lux(ch0, ch1):
    chScale = int(0)

    # /* Make sure the sensor isn't saturated! */
    clipThreshold = 4900 # TSL2561_CLIPPING_13MS
    broadband = ch0
    ir = ch1
    # /* Return 65536 lux if the sensor is saturated */
    if broadband > clipThreshold or ir > clipThreshold:
        return 65536

    # /* Get the correct scale depending on the intergration time */

    chScale = 0x7517

    #/* Scale for gain (1x or 16x) */
    #// if (!_tsl2561Gain)
    #// Gain 1x -> _tsl2561Gain == 0 so the if statement evaluates true
    chScale = chScale << 4

    # /* Scale the channel values */
    channel0 = int(broadband * chScale) >> 10 # TSL2561_LUX_CHSCALE;
    channel1 = int(ir * chScale) >> 10 #TSL2561_LUX_CHSCALE;

    # /* Find the ratio of the channel values (Channel1/Channel0) */
    ratio1 = 0
    if channel0 != 0:
        ratio1 = (channel1 << 10) // channel0 # (channel1 << (TSL2561_LUX_RATIOSCALE + 1)) / channel0

    # /* round the ratio value */
    ratio = (ratio1 + 1) >> 1

    b = int(0)
    m = int(0)

    if ((ratio >= 0) and (ratio <= 0x0040)):
        b = 0x01f2 # TSL2561_LUX_B1T
        m = 0x01be # TSL2561_LUX_M1T
    elif (ratio <= 0x0080):
        b = 0x0214 # TSL2561_LUX_B2T
        m = 0x02b1 # TSL2561_LUX_M2T
    elif (ratio <= 0x00c0):
        b = 0x023f # TSL2561_LUX_B3T
        m = 0x037b # TSL2561_LUX_M3T
    elif (ratio <= 0x0100):
        b = 0x0270 # TSL2561_LUX_B4T
        m = 0x03fe # TSL2561_LUX_M4T
    elif (ratio <= 0x0138):
        b = 0x016f # TSL2561_LUX_B5T
        m = 0x01fc # TSL2561_LUX_M5T
    elif (ratio <= 0x019a):
        b = 0x00d2 # TSL2561_LUX_B6T
        m = 0x00fb # TSL2561_LUX_M6T
    elif (ratio <= 0x029a):
        b = 0x0018 # TSL2561_LUX_B7T
        m = 0x0012 # TSL2561_LUX_M7T
    elif (ratio > 0x029a):
        b = 0x0000 # TSL2561_LUX_B8T
        m = 0x0000 # TSL2561_LUX_M8T

    channel0 = channel0 * b
    channel1 = channel1 * m

    temp = 0
    # /* Do not allow negative lux value */
    if (channel0 > channel1):
        temp = channel0 - channel1

    # /* Round lsb (2^(LUX_SCALE-1)) */
    temp += 1 << 13 #(1 << (TSL2561_LUX_LUXSCALE - 1))

    # /* Strip off fractional portion */
    lux = temp >> 14 # TSL2561_LUX_LUXSCALE

    # /* Signal I2C had no errors */
    return lux

if __name__ == '__main__':
    global done
    done = 0
    signal.signal(signal.SIGINT, sigHandler)

    if len(sys.argv) < 2:
        print("Invocation: sudo ./css_calib.py <Serial Device>")
        sys.exit(0)

    s = smbus.SMBus(1) # RPi default

    chns = [0, 1, 2]
    addrs = [0x29, 0x39, 0x49]

    # Initialize all sensors
    for chn in chns:
        for addr in addrs:
            print("Init from %d->%d: %d"%(chn, addr, tsl2561_init(s, chn, addr)))

    ser = serial.Serial(sys.argv[1], 230400)

    val = int(0)

    calib = []
    count = 0

    while not done:
        if val > 0x0fff:
            break
        # Write the value to serial port
        val = 0x0fff & val # 12 bit number only
        calib.append([])
        calib[count].append(val)
        for i in range(11):
            ser.write(val.to_bytes(2, byteorder='little'))
        ser.flush()
        time.sleep(0.1)
        for chn in chns:
            for addr in addrs:
                ch0, ch1 = tsl2561_measure(s, chn, addr)
                print("%d"%(tsl2561_get_lux(ch0, ch1)), end=" ")
                calib[count].append(tsl2561_get_lux(ch0, ch1))
        print()
        val += 10 # increment by 10
        count += 1
        time.sleep(0.9)


    # Turn off all sensors
    for chn in chns:
        for addr in addrs:
            tsl2561_destroy(s, chn, addr)
    ser.close()
    channel(s, -1)
    print("Turned off all sensors and mux, exiting...")
    np.savetxt('calib.txt', np.array(calib, dtype=int),fmt='%d',delimiter=',')      
    sys.exit(0)