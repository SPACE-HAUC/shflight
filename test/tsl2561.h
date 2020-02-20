/*
 *******************************************************************************
 * Title: tsl2561.h
 * Description: Device driver (I2C) for TSL2561 Coarse Sun Sensors
 * Update:
 *          0 - Module created [2020/16/1] [NS]
 * Status - REVIEW REQUIRED
 ******************************************************************************* 
 */

#ifndef TSL2561_H
#define TSL2561_H

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
// #include </usr/src/linux-headers-4.4.0-171-generic/include/config/i2c/smbus.h>
// #include <i2c/smbus.h>
#include <signal.h>

/******************************************************************************/
#define TSL2561_VISIBLE 2      ///< channel 0 - channel 1
#define TSL2561_INFRARED 1     ///< channel 1
#define TSL2561_FULLSPECTRUM 0 ///< channel 0

// I2C address options
#define TSL2561_ADDR_LOW (0x29)   ///< Default address (pin pulled low)
#define TSL2561_ADDR_FLOAT (0x39) ///< Default address (pin left floating)
#define TSL2561_ADDR_HIGH (0x49)  ///< Default address (pin pulled high)

// Lux calculations differ slightly for CS package
//#define TSL2561_PACKAGE_CS                ///< Chip scale package
#define TSL2561_PACKAGE_T_FN_CL ///< Dual Flat No-Lead package

#define TSL2561_COMMAND_BIT (0x80) ///< Must be 1
#define TSL2561_CLEAR_BIT (0x40)   ///< Clears any pending interrupt (write 1 to clear)
#define TSL2561_WORD_BIT (0x20)    ///< 1 = read/write word (rather than byte)
#define TSL2561_BLOCK_BIT (0x10)   ///< 1 = using block read/write

#define TSL2561_CONTROL_POWERON (0x03)  ///< Control register setting to turn on
#define TSL2561_CONTROL_POWEROFF (0x00) ///< Control register setting to turn off

#define TSL2561_LUX_LUXSCALE (14)          ///< Scale by 2^14
#define TSL2561_LUX_RATIOSCALE (9)         ///< Scale ratio by 2^9
#define TSL2561_LUX_CHSCALE (10)           ///< Scale channel values by 2^10
#define TSL2561_LUX_CHSCALE_TINT0 (0x7517) ///< 322/11 * 2^TSL2561_LUX_CHSCALE
#define TSL2561_LUX_CHSCALE_TINT1 (0x0FE7) ///< 322/81 * 2^TSL2561_LUX_CHSCALE

// T, FN and CL package values
#define TSL2561_LUX_K1T (0x0040) ///< 0.125 * 2^RATIO_SCALE
#define TSL2561_LUX_B1T (0x01f2) ///< 0.0304 * 2^LUX_SCALE
#define TSL2561_LUX_M1T (0x01be) ///< 0.0272 * 2^LUX_SCALE
#define TSL2561_LUX_K2T (0x0080) ///< 0.250 * 2^RATIO_SCALE
#define TSL2561_LUX_B2T (0x0214) ///< 0.0325 * 2^LUX_SCALE
#define TSL2561_LUX_M2T (0x02d1) ///< 0.0440 * 2^LUX_SCALE
#define TSL2561_LUX_K3T (0x00c0) ///< 0.375 * 2^RATIO_SCALE
#define TSL2561_LUX_B3T (0x023f) ///< 0.0351 * 2^LUX_SCALE
#define TSL2561_LUX_M3T (0x037b) ///< 0.0544 * 2^LUX_SCALE
#define TSL2561_LUX_K4T (0x0100) ///< 0.50 * 2^RATIO_SCALE
#define TSL2561_LUX_B4T (0x0270) ///< 0.0381 * 2^LUX_SCALE
#define TSL2561_LUX_M4T (0x03fe) ///< 0.0624 * 2^LUX_SCALE
#define TSL2561_LUX_K5T (0x0138) ///< 0.61 * 2^RATIO_SCALE
#define TSL2561_LUX_B5T (0x016f) ///< 0.0224 * 2^LUX_SCALE
#define TSL2561_LUX_M5T (0x01fc) ///< 0.0310 * 2^LUX_SCALE
#define TSL2561_LUX_K6T (0x019a) ///< 0.80 * 2^RATIO_SCALE
#define TSL2561_LUX_B6T (0x00d2) ///< 0.0128 * 2^LUX_SCALE
#define TSL2561_LUX_M6T (0x00fb) ///< 0.0153 * 2^LUX_SCALE
#define TSL2561_LUX_K7T (0x029a) ///< 1.3 * 2^RATIO_SCALE
#define TSL2561_LUX_B7T (0x0018) ///< 0.00146 * 2^LUX_SCALE
#define TSL2561_LUX_M7T (0x0012) ///< 0.00112 * 2^LUX_SCALE
#define TSL2561_LUX_K8T (0x029a) ///< 1.3 * 2^RATIO_SCALE
#define TSL2561_LUX_B8T (0x0000) ///< 0.000 * 2^LUX_SCALE
#define TSL2561_LUX_M8T (0x0000) ///< 0.000 * 2^LUX_SCALE

// CS package values
#define TSL2561_LUX_K1C (0x0043) ///< 0.130 * 2^RATIO_SCALE
#define TSL2561_LUX_B1C (0x0204) ///< 0.0315 * 2^LUX_SCALE
#define TSL2561_LUX_M1C (0x01ad) ///< 0.0262 * 2^LUX_SCALE
#define TSL2561_LUX_K2C (0x0085) ///< 0.260 * 2^RATIO_SCALE
#define TSL2561_LUX_B2C (0x0228) ///< 0.0337 * 2^LUX_SCALE
#define TSL2561_LUX_M2C (0x02c1) ///< 0.0430 * 2^LUX_SCALE
#define TSL2561_LUX_K3C (0x00c8) ///< 0.390 * 2^RATIO_SCALE
#define TSL2561_LUX_B3C (0x0253) ///< 0.0363 * 2^LUX_SCALE
#define TSL2561_LUX_M3C (0x0363) ///< 0.0529 * 2^LUX_SCALE
#define TSL2561_LUX_K4C (0x010a) ///< 0.520 * 2^RATIO_SCALE
#define TSL2561_LUX_B4C (0x0282) ///< 0.0392 * 2^LUX_SCALE
#define TSL2561_LUX_M4C (0x03df) ///< 0.0605 * 2^LUX_SCALE
#define TSL2561_LUX_K5C (0x014d) ///< 0.65 * 2^RATIO_SCALE
#define TSL2561_LUX_B5C (0x0177) ///< 0.0229 * 2^LUX_SCALE
#define TSL2561_LUX_M5C (0x01dd) ///< 0.0291 * 2^LUX_SCALE
#define TSL2561_LUX_K6C (0x019a) ///< 0.80 * 2^RATIO_SCALE
#define TSL2561_LUX_B6C (0x0101) ///< 0.0157 * 2^LUX_SCALE
#define TSL2561_LUX_M6C (0x0127) ///< 0.0180 * 2^LUX_SCALE
#define TSL2561_LUX_K7C (0x029a) ///< 1.3 * 2^RATIO_SCALE
#define TSL2561_LUX_B7C (0x0037) ///< 0.00338 * 2^LUX_SCALE
#define TSL2561_LUX_M7C (0x002b) ///< 0.00260 * 2^LUX_SCALE
#define TSL2561_LUX_K8C (0x029a) ///< 1.3 * 2^RATIO_SCALE
#define TSL2561_LUX_B8C (0x0000) ///< 0.000 * 2^LUX_SCALE
#define TSL2561_LUX_M8C (0x0000) ///< 0.000 * 2^LUX_SCALE

// Auto-gain thresholds
#define TSL2561_AGC_THI_13MS (4850)   ///< Max value at Ti 13ms = 5047
#define TSL2561_AGC_TLO_13MS (100)    ///< Min value at Ti 13ms = 100
#define TSL2561_AGC_THI_101MS (36000) ///< Max value at Ti 101ms = 37177
#define TSL2561_AGC_TLO_101MS (200)   ///< Min value at Ti 101ms = 200
#define TSL2561_AGC_THI_402MS (63000) ///< Max value at Ti 402ms = 65535
#define TSL2561_AGC_TLO_402MS (500)   ///< Min value at Ti 402ms = 500

// Clipping thresholds
#define TSL2561_CLIPPING_13MS (4900)   ///< # Counts that trigger a change in gain/integration
#define TSL2561_CLIPPING_101MS (37000) ///< # Counts that trigger a change in gain/integration
#define TSL2561_CLIPPING_402MS (65000) ///< # Counts that trigger a change in gain/integration

// Delay for integration times
#define TSL2561_DELAY_INTTIME_13MS (15)   ///< Wait 15ms for 13ms integration
#define TSL2561_DELAY_INTTIME_101MS (120) ///< Wait 120ms for 101ms integration
#define TSL2561_DELAY_INTTIME_402MS (450) ///< Wait 450ms for 402ms integration

/** TSL2561 I2C Registers */
enum
{
    TSL2561_REGISTER_CONTROL = 0x00,          // Control/power register
    TSL2561_REGISTER_TIMING = 0x01,           // Set integration time register
    TSL2561_REGISTER_THRESHHOLDL_LOW = 0x02,  // Interrupt low threshold low-byte
    TSL2561_REGISTER_THRESHHOLDL_HIGH = 0x03, // Interrupt low threshold high-byte
    TSL2561_REGISTER_THRESHHOLDH_LOW = 0x04,  // Interrupt high threshold low-byte
    TSL2561_REGISTER_THRESHHOLDH_HIGH = 0x05, // Interrupt high threshold high-byte
    TSL2561_REGISTER_INTERRUPT = 0x06,        // Interrupt settings
    TSL2561_REGISTER_CRC = 0x08,              // Factory use only
    TSL2561_REGISTER_ID = 0x0A,               // TSL2561 identification setting
    TSL2561_REGISTER_CHAN0_LOW = 0x0C,        // Light data channel 0, low byte
    TSL2561_REGISTER_CHAN0_HIGH = 0x0D,       // Light data channel 0, high byte
    TSL2561_REGISTER_CHAN1_LOW = 0x0E,        // Light data channel 1, low byte
    TSL2561_REGISTER_CHAN1_HIGH = 0x0F        // Light data channel 1, high byte
};

/** Three options for how long to integrate readings for */
typedef enum
{
    TSL2561_INTEGRATIONTIME_13MS = 0x00,  // 13.7ms
    TSL2561_INTEGRATIONTIME_101MS = 0x01, // 101ms
    TSL2561_INTEGRATIONTIME_402MS = 0x02  // 402ms
} tsl2561IntegrationTime_t;

/** TSL2561 offers 2 gain settings */
typedef enum
{
    TSL2561_GAIN_1X = 0x00,  // No gain
    TSL2561_GAIN_16X = 0x10, // 16x gain
} tsl2561Gain_t;

/******************************************************************************/
#define I2C_BUS "/dev/i2c-1" // I2C bus name
#define TSL2561_BLOCK_READ 0x0B;

// TSL2561 COMMAND REGISTER - SEND FIRST
typedef union {
    struct
    {
        uint8_t cmd : 1;
        uint8_t clear : 1;
        uint8_t word : 1;
        uint8_t block : 1;
        uint8_t address : 4;
    };
    uint8_t raw;
} tsl2561_command;

/* TSL2561 DATA STRUCTURE */
typedef struct
{
    int fd;
    char fname[40];
} tsl2561;

/*
 * Init function for the TSL2561 device. Default: I2C_BUS
 */
int tsl2561_init(tsl2561 *dev, uint8_t s_address);
/*
 * Read I2C data into the uint32_t measure var.
 * Format: (MSB) broadband | ir (LSB)
 */
void tsl2561_measure(tsl2561 *dev, uint32_t *measure);
/*
 * Get lux using the measured value.
 */
uint32_t tsl2561_get_lux(uint32_t measure);
/*
 * Destroy function for the TSL2561 device. Closes the file descriptor and powers * down the device
 */
void tsl2561_destroy(tsl2561 *dev);

/* 
 * Internal functions to read the sensors using the I2C protocol
 */

/* 
 * write 8 bytes to the device represented by the file descriptor.
 */
static inline void write8(int fd, uint8_t val)
{
    uint8_t buf = val ;
    int res = write(fd, &buf, 1);
    if ( res != 1 )
        perror(__FUNCTION__);
}
/* 
 * Write a command to the register on the device represented by fd
 */
static inline void writecmd8(int fd, uint8_t reg, uint8_t val)
{
    uint8_t buf[2] = {0x0};
    buf[0] = reg ; buf[1] = val ;
    int res = write(fd, &buf, 2);
    if ( res != 2 )
        perror(__FUNCTION__);
}
/* 
 * Read a byte from the specified register on the device represented by fd
 */
static inline uint8_t read8(int fd, uint8_t reg)
{
    write8(fd, reg);
    uint8_t buf = 0x00 ;
    int res = read(fd, &buf, 1);
    if ( res != 1 )
        perror(__FUNCTION__);
    return buf;
}
/* 
 * Write 16 bits to the device (very similar to writecmd8())
 */
static inline void write16(int fd, uint16_t val)
{
    uint8_t buf[2] ;
    buf[0] = val >> 8 ;
    buf[1] = 0x00ff & val ;
    int res = write(fd, buf, 2);
    if ( res != 2 )
        perror(__FUNCTION__);
    return;
}
/* 
 * Read 2 bytes in LE format from reg on the device represented by fd 
 */
static inline uint16_t read16(int fd, uint8_t cmd)
{
    uint8_t buf[2] = {0x0};
    write8(fd, cmd);
    int res = read(fd, buf, 2);
    if ( res != 2 )
        perror(__FUNCTION__);
    return buf[0] | ((unsigned short)buf[1]) << 8 ;
}

#endif // TSL2561_H