#ifndef __SHFLIGHT_CONSTS_H
#define __SHFLIGHT_CONSTS_H

#ifndef I2C_BUS
#define I2C_BUS "/dev/i2c-1" // default for Raspberry Pi, on flight computer use i2c-0
#endif                       // I2C_BUS

#ifndef SPIDEV_ACS
#define SPIDEV_ACS "/dev/spidev0.0" // default SPI bus on RPi
#endif

#define SH_BUFFER_SIZE 64                                            // Circular buffer size
#define DIPOLE_MOMENT 0.22                                           // A m^-2
#define DETUMBLE_TIME_STEP 100000                                    // 100 ms for full loop
#define MEASURE_TIME 20000                                           // 20 ms to measure
#define MAX_DETUMBLE_FIRING_TIME (DETUMBLE_TIME_STEP - MEASURE_TIME) // Max allowed detumble fire time
#define MIN_DETUMBLE_FIRING_TIME 10000                               // 10 ms
#define SUNPOINT_DUTY_CYCLE 20000                                    // 20 msec, in usec
#define COARSE_TIME_STEP DETUMBLE_TIME_STEP                          // 100 ms, in usec
#ifdef SITL
#define CSS_MIN_LUX_THRESHOLD 5000 * 0.5 // 5000 lux is max sun, half of that is our threshold (subject to change)
#else
#define CSS_MIN_LUX_THRESHOLD 5000 * 0.5 // 5000 lux is max sun, half of that is our threshold (subject to change)
#endif                                   // SITL

#define OMEGA_TARGET_LEEWAY z_g_W_target * 0.1 // 10% leeway in the value of omega_z
#define MIN_SOL_ANGLE 4                        // minimum solar angle for sunpointing to be a success
#define MIN_DETUMBLE_ANGLE 4                   // minimum angle for detumble to be a success

#ifndef PORT
#define PORT 12376 // DataVis port number
#endif // PORT

#ifndef BESSEL_MIN_THRESHOLD
#define BESSEL_MIN_THRESHOLD 0.001 // randomly chosen minimum value for valid coefficient
#endif

#ifndef BESSEL_FREQ_CUTOFF
#define BESSEL_FREQ_CUTOFF 5       // cutoff frequency 5 == 5*DETUMBLE_TIME_STEP seconds cycle == 2 Hz at 100ms loop speed
#endif

#endif // __SHFLIGHT_CONSTS_H