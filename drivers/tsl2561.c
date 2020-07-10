/**
 * @file tsl2561.c
 * @author Sunip K. Mukherjee (sunipkmukherjee@gmail.com)
 * @brief TSL2561 I2C driver function definitions
 * @version 0.1
 * @date 2020-03-19
 * 
 * @copyright Copyright (c) 2020
 * 
 */
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <linux/types.h>
#include <linux/i2c-dev.h>
#include "tsl2561.h"
// #include </usr/src/linux-headers-4.4.0-171-generic/include/config/i2c/smbus.h>
// #include <i2c/smbus.h>
#include <signal.h>

/**
 * @brief write 8 bytes to the device represented by the file descriptor.
 * 
 * @param fd 
 * @param val 
 */
static inline void write8(int fd, uint8_t val)
{
    uint8_t buf = val;
    int res = write(fd, &buf, 1);
    if (res != 1)
        perror(__FUNCTION__);
}
/**
 * @brief Write a command to the register on the device represented by fd
 * 
 * @param fd File descriptor
 * @param reg Register address
 * @param val Value to write at register address
 */
static inline void writecmd8(int fd, uint8_t reg, uint8_t val)
{
    uint8_t buf[2] = {0x0};
    buf[0] = reg;
    buf[1] = val;
    int res = write(fd, &buf, 2);
    if (res != 2)
        perror(__FUNCTION__);
}
/**
 * @brief Read a byte from the specified register on the device represented by fd
 * 
 * @param fd 
 * @param reg Register address
 * @return Byte read over serial 
 */
static inline uint8_t read8(int fd, uint8_t reg)
{
    write8(fd, reg);
    uint8_t buf = 0x00;
    int res = read(fd, &buf, 1);
    if (res != 1)
        perror(__FUNCTION__);
    return buf;
}
/**
 * @brief Write 16 bits to the device (very similar to writecmd8())
 * 
 * @param fd 
 * @param val 
 */
static inline void write16(int fd, uint16_t val)
{
    uint8_t buf[2];
    buf[0] = val >> 8;
    buf[1] = 0x00ff & val;
    int res = write(fd, buf, 2);
    if (res != 2)
        perror(__FUNCTION__);
    return;
}
/**
 * @brief Read 2 bytes in LE format from reg on the device represented by fd 
 * 
 * @param fd 
 * @param cmd 
 * @return uint16_t 
 */
static inline uint16_t read16(int fd, uint8_t cmd)
{
    uint8_t buf[2] = {0x0};
    write8(fd, cmd);
    int res = read(fd, buf, 2);
    if (res != 2)
        perror(__FUNCTION__);
    return buf[0] | ((unsigned short)buf[1]) << 8;
}

/**
 * @brief Init function for the TSL2561 device. Default: I2C_BUS
 * TODO: Fix init + gain, figure out what goes wrong if ID
 * register is read
 * 
 * @param dev 
 * @param s_address Address for the device, values: 0x29, 0x39, 0x49
 * @return 1 on success, -1 on failure
 */
int tsl2561_init(tsl2561 *dev, uint8_t s_address)
{
    // Create the file descriptor handle to the device
    dev->fd = open(I2C_BUS, O_RDWR);
    if (dev->fd < 0)
    {
        perror("[ERROR] TSL2561 Could not create a file descriptor.");
        return -1;
    }

    // Configure file descriptor handle as a slave device w/input address
    if (ioctl(dev->fd, I2C_SLAVE, s_address) < 0)
    {
        perror("[ERROR] TSL2561 Could not assign device.");
        return -1;
    }

    // Power the device - write to control register
    writecmd8(dev->fd, 0x80, 0x03);
    usleep(100000);
    // Verify that device is powered
    if ((read8(dev->fd, 0x80) & 0x3) != 0x3)
    {
        perror("Device not powered up");
        return -1;
    }
    /* DO NOT READ THE DEVICE REGISTER */
#ifdef CSS_LOW_GAIN
    // Set the timing and gain
    writecmd8(dev->fd, 0x81, 0x00);
    if (read8(dev->fd, 0x81))
    {
        perror("Could not set timing and gain");
        return -1;
    }
#endif
    return dev->fd; // should be > 0 in this case
}
/**
 * @brief Read I2C data into the uint32_t measure var.\
 * Format: (MSB) broadband | ir (LSB)
 * 
 * @param dev 
 * @param measure Pointer to unsigned 32 bit integer where measurement is stored
 */
void tsl2561_measure(tsl2561 *dev, uint32_t *measure)
{
    *measure = ((uint32_t)read16(dev->fd, 0xac)) << 16 | read16(dev->fd, 0xae);
    return;
}
/**
 * @brief Calculate lux using value measured using tsl2561_measure()
 * 
 * @param measure 
 * @return Lux value 
 */
uint32_t tsl2561_get_lux(uint32_t measure)
{
    unsigned long chScale;
    unsigned long channel1;
    unsigned long channel0;

    /* Make sure the sensor isn't saturated! */
    uint16_t clipThreshold = TSL2561_CLIPPING_13MS;
    uint16_t broadband = measure >> 16;
    uint16_t ir = measure;
    /* Return 65536 lux if the sensor is saturated */
    if ((broadband > clipThreshold) || (ir > clipThreshold))
    {
        return 65536;
    }

    /* Get the correct scale depending on the intergration time */

    chScale = TSL2561_LUX_CHSCALE_TINT0;

    /* Scale for gain (1x or 16x) */
    // if (!_tsl2561Gain)
    // Gain 1x -> _tsl2561Gain == 0 so the if statement evaluates true
    chScale = chScale << 4;

    /* Scale the channel values */
    channel0 = (broadband * chScale) >> TSL2561_LUX_CHSCALE;
    channel1 = (ir * chScale) >> TSL2561_LUX_CHSCALE;

    /* Find the ratio of the channel values (Channel1/Channel0) */
    unsigned long ratio1 = 0;
    if (channel0 != 0)
        ratio1 = (channel1 << (TSL2561_LUX_RATIOSCALE + 1)) / channel0;

    /* round the ratio value */
    unsigned long ratio = (ratio1 + 1) >> 1;

    unsigned int b, m;

#ifdef TSL2561_PACKAGE_CS
    if ((ratio >= 0) && (ratio <= TSL2561_LUX_K1C))
    {
        b = TSL2561_LUX_B1C;
        m = TSL2561_LUX_M1C;
    }
    else if (ratio <= TSL2561_LUX_K2C)
    {
        b = TSL2561_LUX_B2C;
        m = TSL2561_LUX_M2C;
    }
    else if (ratio <= TSL2561_LUX_K3C)
    {
        b = TSL2561_LUX_B3C;
        m = TSL2561_LUX_M3C;
    }
    else if (ratio <= TSL2561_LUX_K4C)
    {
        b = TSL2561_LUX_B4C;
        m = TSL2561_LUX_M4C;
    }
    else if (ratio <= TSL2561_LUX_K5C)
    {
        b = TSL2561_LUX_B5C;
        m = TSL2561_LUX_M5C;
    }
    else if (ratio <= TSL2561_LUX_K6C)
    {
        b = TSL2561_LUX_B6C;
        m = TSL2561_LUX_M6C;
    }
    else if (ratio <= TSL2561_LUX_K7C)
    {
        b = TSL2561_LUX_B7C;
        m = TSL2561_LUX_M7C;
    }
    else if (ratio > TSL2561_LUX_K8C)
    {
        b = TSL2561_LUX_B8C;
        m = TSL2561_LUX_M8C;
    }
#else
    if ((ratio >= 0) && (ratio <= TSL2561_LUX_K1T))
    {
        b = TSL2561_LUX_B1T;
        m = TSL2561_LUX_M1T;
    }
    else if (ratio <= TSL2561_LUX_K2T)
    {
        b = TSL2561_LUX_B2T;
        m = TSL2561_LUX_M2T;
    }
    else if (ratio <= TSL2561_LUX_K3T)
    {
        b = TSL2561_LUX_B3T;
        m = TSL2561_LUX_M3T;
    }
    else if (ratio <= TSL2561_LUX_K4T)
    {
        b = TSL2561_LUX_B4T;
        m = TSL2561_LUX_M4T;
    }
    else if (ratio <= TSL2561_LUX_K5T)
    {
        b = TSL2561_LUX_B5T;
        m = TSL2561_LUX_M5T;
    }
    else if (ratio <= TSL2561_LUX_K6T)
    {
        b = TSL2561_LUX_B6T;
        m = TSL2561_LUX_M6T;
    }
    else if (ratio <= TSL2561_LUX_K7T)
    {
        b = TSL2561_LUX_B7T;
        m = TSL2561_LUX_M7T;
    }
    else if (ratio > TSL2561_LUX_K8T)
    {
        b = TSL2561_LUX_B8T;
        m = TSL2561_LUX_M8T;
    }
#endif

    unsigned long temp;
    channel0 = channel0 * b;
    channel1 = channel1 * m;

    temp = 0;
    /* Do not allow negative lux value */
    if (channel0 > channel1)
        temp = channel0 - channel1;

    /* Round lsb (2^(LUX_SCALE-1)) */
    temp += (1 << (TSL2561_LUX_LUXSCALE - 1));

    /* Strip off fractional portion */
    uint32_t lux = temp >> TSL2561_LUX_LUXSCALE;

    /* Signal I2C had no errors */
    return lux;
}
/**
 * @brief Destroy function for the TSL2561 device. Closes the file descriptor
 *  and powers down the device
 * 
 * @param dev 
 */
void tsl2561_destroy(tsl2561 *dev)
{
    writecmd8(dev->fd, 0x80, 0x00);
    close(dev->fd);
    free(dev);
}