#ifndef __SHFLIGHT_CONSTS_H
#define __SHFLIGHT_CONSTS_H

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
#endif // __SHFLIGHT_CONSTS_H