/**
 * @file acs_extern.h
 * @author Sunip K. Mukherjee (sunipkmukherjee@gmail.com)
 * @brief Header file including constants, extern variables and function prototypes
 * that are part of the Attitude Control System, used in other modules.
 * @version 0.1
 * @date 2020-03-19
 * 
 * @copyright Copyright (c) 2020
 * 
 */
#ifndef ACS_EXTERN_H
#define ACS_EXTERN_H
#include <pthread.h>
#include <macros.h> // Macro definitions and functions specific to SHFlight
/**
 * @brief Circular buffer size for ACS sensor data
 * 
 */
#define SH_BUFFER_SIZE 64 // Circular buffer size
// the following extern declarations will show up only in files other than acs.h
#ifndef ACS_H
// SITL
extern pthread_cond_t data_available;            // data available wakeup from SITL Comm
DECLARE_VECTOR2(g_readB, extern unsigned short); // storage to put helmhotz values
extern unsigned short g_readFS[2];               // storage to put FS X and Y angles
extern unsigned short g_readCS[9];               // storage to put CS led brightnesses
extern unsigned char g_Fire;                     // magnetorquer command
extern volatile int first_run;                   // first run
#endif                                           // ACS_H

#endif // ACS_EXTERN_H