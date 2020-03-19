/**
 * @file shflight_externs.h
 * @author Sunip K. Mukherjee (sunipkmukherjee@gmail.com)
 * @brief Extern declaration of the shflight global variables for all threads.
 * @version 0.1
 * @date 2020-03-19
 * 
 * @copyright Copyright (c) 2020
 * 
 */
#ifndef __SHFLIGHT_EXTERNS_H
#define __SHFLIGHT_EXTERNS_H

#include <signal.h>
#include <main.h>
#include <main_helper.h>
#include <shflight_consts.h>
#ifdef DATAVIS
#include <datavis.h>
#endif

extern pthread_mutex_t serial_read, serial_write, data_check;
extern pthread_cond_t data_available;

extern pthread_mutex_t datavis_mutex;
extern pthread_cond_t datavis_drdy;

extern volatile sig_atomic_t done;
extern volatile int first_run;

DECLARE_VECTOR2(g_readB, extern unsigned short); // storage to put helmhotz values
extern unsigned short g_readFS[2];               // storage to put FS X and Y angles
extern unsigned short g_readCS[9];               // storage to put CS led brightnesses
extern unsigned char g_Fire;                     // magnetorquer command

extern lsm9ds1 *mag;                                      // magnetometer
extern ncv7708 *hbridge;                                  // h-bridge
extern tca9458a *mux;                                     // I2C mux
extern tsl2561 **css;                                     // coarse sun sensors
extern ads1115 *adc;                                      // analog to digital converters
                                                          // SITL
DECLARE_BUFFER(g_W, extern float);                        // omega global circular buffer
DECLARE_BUFFER(g_B, extern double);                       // magnetic field global circular buffer
DECLARE_BUFFER(g_Bt, extern double);                      // Bdot global circular buffer1
DECLARE_VECTOR2(g_L_target, extern float);                // angular momentum target vector
DECLARE_VECTOR2(g_W_target, extern float);                // angular velocity target vector
DECLARE_BUFFER(g_S, extern float);                        // sun vector
extern float g_CSS[9];                                    // current CSS lux values, in HITL this will be populated by TSL2561 code
extern float g_FSS[2];                                    // current FSS angles, in rad; in HITL this will be populated by NANOSSOC A60 driver
extern int mag_index, omega_index, bdot_index, sol_index; // circular buffer indices, -1 indicates uninitiated buffer
extern int B_full, Bdot_full, W_full, S_full;             // required to deal with the circular buffer problem
extern uint8_t g_night;                                   // night mode?
extern uint8_t g_acs_mode;                                // Detumble by default
extern uint8_t g_first_detumble;                          // first time detumble by default even at night
extern unsigned long long acs_ct;                         // counts the number of ACS steps

extern float bessel_coeff[SH_BUFFER_SIZE]; // coefficients for Bessel filter, declared as floating point

extern float MOI[3][3];
extern float IMOI[3][3];

extern unsigned long long g_t_acs;

#ifdef ACS_DATALOG
extern FILE *acs_datalog;
#endif

#ifdef DATAVIS
extern data_packet g_datavis_st;
#endif

extern unsigned long long t_comm;
extern unsigned long long comm_time;

#endif