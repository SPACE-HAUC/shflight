#ifndef __DATA_ACK_H
#define __DATA_ACK_H
#include <main.h>

#define THREAD_TIMEOUT 5              // 5 seconds absolute timeout on the threads
#define XBAND_TIMEOUT  1200            // 20 minutes for X Band
#define MIN_OMEGA_STABLE_THRESHOLD 3  // 4 readings during state change to ensure criteria holds
#define MIN_SOL_STABLE_THRESHOLD 3    // 4 readings during state change to ensure criteria holds
float FINE_POINTING_LIMIT = 0.996;    // cos(5 deg). Declared as a float as this value might change
                                      // with time, if it is determined that in a sufficiently long time
                                      // this limit can not be hit
#define FINE_POINTING_THRESHOLD 0.940 // cos(20 deg), which is less the FOV of the FSS

/*
 * Measures magnetic field from the LSM9DF1 sensor and
 * returns the status word. Populates the cyclic magnetic 
 * field buffer and calculates Bdot, which also goes 
 * in a circular buffer.
 */
int getMagField(void);

// Populates the sun vector buffer at each timestep
int getSunVec(void);

// Gyroscope function. Leverages measurements of B to generate omega
// with corrections using moment of inertia
void getOmega(void);

#endif // __DATA_ACK_H