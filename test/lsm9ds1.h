#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <linux/i2c-dev.h>
#include <errno.h>
#include <stdint.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <sys/ioctl.h>

#define MAG_I2C_FIle "/dev/i2c-1"

#define LSM9DS1_XL_ADDR 0x6b
#define LSM9DS1_MAG_ADDR 0x1e

/* 
 * Accelerometer and Gyro registers
 * 
 * NOTE: Few registers are used ONLY TO
 * power down the accelerometer and the gyroscope.
 */

#define LSM9DS1_CTRL_REG1_G 0x10 // Gyro control register
#define LSM9DS1_GYRO_PD 0x00     // Content of the gyro control register for power down
/*
 * Refer to documentation
 */
#define LSM9DS1_CTRL_REG5_XL 0x1f // Acceleration control register
#define LSM9DS1_XL_PD 0x00        // Disable outputs
/* 
 * ODR_XL[7:5]: Output data rate and power mode, 0 0 0 for power down
 * FS_XL[4:3]: Full scale selection
 * BW_SCAL_ODR[2:2]: Bandwidth selection, 0 -- default, 1 -- bandwidth from BW_XL
 * BW_XL[1:0]: Custom bandwidth
 */
#define LSM9DS1_CTRL_REG6_XL 0x20

/*
 * Magnetometer registers
 */
typedef enum
{
    MAG_OFFSET_X_REG_L_M = 0x05,
    MAG_OFFSET_X_REG_H_M,
    MAG_OFFSET_Y_REG_L_M,
    MAG_OFFSET_Y_REG_H_M,
    MAG_OFFSET_Z_REG_L_M,
    MAG_OFFSET_Z_REG_H_M,
} MAG_OFFSET_REGISTERS;

#define MAG_CTRL_REG1_M 0x20
typedef struct __attribute__((packed))
{
    uint8_t self_test : 1;
    // Data rate > 80 Hz
    uint8_t fast_odr : 1;
    /*
    * Set Data Rate in Hz.
    * 000: 0.625 Hz
    * 001: 1.25 Hz
    * 010: 2.5 Hz
    * 011: 5 Hz
    * 100: 10 Hz (Default)
    * 101: 20 Hz (SPACE HAUC setting)
    * 110: 40 Hz
    * 111: 80 Hz
    */
    uint8_t data_rate : 3;
    /*
    * Operative mode for X and Y axes.
    * 00: LP mode (Default)
    * 01: Medium perf
    * 10: High perf
    * 11: Ultra-high perf
    */
    uint8_t operative_mode : 2;
    uint8_t temp_comp : 1;
} MAG_DATA_RATE;

#define MAG_CTRL_REG2_M 0x21
typedef struct __attribute__((packed))
{
    uint8_t reserved : 2;
    /*
    * Configuration registers and user register reset.
    * 0: Default, 1: Reset
    */
    uint8_t soft_rst : 1;
    /*
    * Clear memory content.
    * 0: Default, 1: Reboot memory content
    */
    uint8_t reboot : 1;
    uint8_t reserved2 : 1;
    /*
    * Full scale config. Default: 00
    * 00: +/- 4 Gauss
    * 01: +/- 8 Gauss
    * 10: +/- 12 Gauss
    * 11: +/- 16 Gauss
    */
    uint8_t full_scale : 2;
    uint8_t reserved3 : 1;
} MAG_RESET;

#define MAG_CTRL_REG3_M 0x22 // write 0x00 to this

#define MAG_CTRL_REG4_M 0x23
#define MAG_CTRL_REG4_DATA 0x0c // [11][0 0], ultra high Z performance + little endian register data selection

#define MAG_CTRL_REG5_M 0x24
typedef struct __attribute__((packed))
{
    uint8_t reserved : 6;
    /*
    * Block data update for magnetic data.
    * 0: Continuous update,
    * 1: Output registers not updated until MSB and LSB has been read
    */
    uint8_t bdu : 1;
    /*
    * FAST_READ allows reading the high part of DATA OUT only in order to increase
    * reading efficiency. Default: 0
    * 0: FAST_READ disabled, 1: Enabled
    */
    uint8_t fast_read : 1;
} MAG_DATA_READ;

typedef enum
{
    MAG_OUT_X_L = 0x28,
    MAG_OUT_X_H,
    MAG_OUT_Y_L,
    MAG_OUT_Y_H,
    MAG_OUT_Z_L,
    MAG_OUT_Z_H
} MAG_OUT_DATA;

typedef struct
{
    // fd for accelerometer
    int accel_file;
    // fd for magnetometer
    int mag_file;
    // file name for bus
    char fname[40];
} lsm9ds1;

#define MAG_WHO_AM_I 0x0f
#define MAG_IDENT 0b00111101

/*
 * lsm9ds1_init: Takes the pointer to the device struct, XL address and M address,
 * returns 1 on success, negative numbers on failure.
 * 
 * Opens the file descriptor for the Accel+Gyro and Magnetometer, powers down the
 * Accel+Gyro.
 * 
 * Configures the magnetometer with SPACE-HAUC specific settings.
 */
int lsm9ds1_init(lsm9ds1 *, uint8_t, uint8_t);

/*
 * lsm9ds1_config_mag: Configure the data rate, reset vector and data granularity.
 */
int lsm9ds1_config_mag(lsm9ds1 *, MAG_DATA_RATE, MAG_RESET, MAG_DATA_READ);
/*
 * lsm9ds1_reset_mag: Reset the magnetometer memory.
 */
int lsm9ds1_reset_mag(lsm9ds1 *);
/*
 * lsm9ds1_read_mag(lsm9ds1*, short *):
 * Store the magnetic field readings in the array of shorts, order: X Y Z
 */
int lsm9ds1_read_mag(lsm9ds1 *, short *);
/*
 * lsm9ds1_offset_mag(lsm9ds1*, short *):
 * Set the mag field offsets using the array, order: X Y Z
 */
int lsm9ds1_offset_mag(lsm9ds1 *, short *);
/*
 * lsm9ds1_destroy(lsm9ds1*):
 * Closes the file descriptors for the mag and accel and frees the allocated memory.
 */
void lsm9ds1_destroy(lsm9ds1 *);