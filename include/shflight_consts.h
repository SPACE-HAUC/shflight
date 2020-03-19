/**
 * @file shflight_consts.h
 * @author Sunip K. Mukherjee (sunipkmukherjee@gmail.com)
 * @brief Describes all constants defined by the preprocessor for the code
 * @version 0.1
 * @date 2020-03-19
 * 
 * @copyright Copyright (c) 2020
 * 
 */
#ifndef __SHFLIGHT_CONSTS_H
#define __SHFLIGHT_CONSTS_H
#ifndef I2C_BUS
/**
 * @brief I2C Bus device file used for ACS sensors
 * 
 */
#define I2C_BUS "/dev/i2c-1" // default for Raspberry Pi, on flight computer use i2c-0
#endif                       // I2C_BUS

#ifndef SPIDEV_ACS
/**
 * @brief SPI device file for H-Bridge (ACS)
 * 
 */
#define SPIDEV_ACS "/dev/spidev0.0" // default SPI bus on RPi
#endif
/**
 * @brief Circular buffer size for ACS sensor data
 * 
 */
#define SH_BUFFER_SIZE 64 // Circular buffer size
/**
 * @brief Dipole moment of the magnetorquer rods
 * 
 */
#define DIPOLE_MOMENT 0.22 // A m^-2
/**
 * @brief ACS loop time period
 * 
 */
#define DETUMBLE_TIME_STEP 100000 // 100 ms for full loop
/**
 * @brief ACS readSensors() max execute time per cycle
 * 
 */
#define MEASURE_TIME 20000 // 20 ms to measure
/**
 * @brief ACS max actuation time per cycle
 * 
 */
#define MAX_DETUMBLE_FIRING_TIME (DETUMBLE_TIME_STEP - MEASURE_TIME) // Max allowed detumble fire time
/**
 * @brief Minimum magnetorquer firing time
 * 
 */
#define MIN_DETUMBLE_FIRING_TIME 10000 // 10 ms
/**
 * @brief Sunpointing magnetorquer PWM duty cycle
 * 
 */
#define SUNPOINT_DUTY_CYCLE 20000 // 20 msec, in usec
/**
 * @brief Course sun sensing mode loop time for ACS
 * 
 */
#define COARSE_TIME_STEP DETUMBLE_TIME_STEP // 100 ms, in usec
/**
 * @brief Coarse sun sensor minimum lux threshold for valid measurement
 * 
 */
#ifdef SITL
#define CSS_MIN_LUX_THRESHOLD 5000 * 0.5 // 5000 lux is max sun, half of that is our threshold (subject to change)
#else
#define CSS_MIN_LUX_THRESHOLD 5000 * 0.5 // 5000 lux is max sun, half of that is our threshold (subject to change)
#endif                                   // SITL
/**
 * @brief Acceptable leeway of the angular speed target
 * 
 */
#define OMEGA_TARGET_LEEWAY z_g_W_target * 0.1 // 10% leeway in the value of omega_z
/**
 * @brief Sunpointing angle target (in degrees)
 * 
 */
#define MIN_SOL_ANGLE 4 // minimum solar angle for sunpointing to be a success
/**
 * @brief Detumble angle target (in degrees)
 * 
 */
#define MIN_DETUMBLE_ANGLE 4 // minimum angle for detumble to be a success
/**
 * @brief DataVis port number
 * 
 */
#ifndef PORT
#define PORT 12376 // DataVis port number
#endif             // PORT
/**
 * @brief Bessel coefficient minimum value threshold for computation
 * 
 */
#ifndef BESSEL_MIN_THRESHOLD
#define BESSEL_MIN_THRESHOLD 0.001 // randomly chosen minimum value for valid coefficient
#endif
/**
 * @brief Bessel filter cutoff frequency
 * 
 */
#ifndef BESSEL_FREQ_CUTOFF
#define BESSEL_FREQ_CUTOFF 5 // cutoff frequency 5 == 5*DETUMBLE_TIME_STEP seconds cycle == 2 Hz at 100ms loop speed
#endif

#endif // __SHFLIGHT_CONSTS_H