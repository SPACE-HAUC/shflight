#ifndef __SHFLIGHT_GLOBALS_H
#define __SHFLIGHT_GLOBALS_H

#include <main.h>

// thread-local status vector
__thread int sys_status;

pthread_mutex_t serial_read, serial_write, data_check;
pthread_cond_t data_available;

pthread_mutex_t datavis_mutex;
pthread_cond_t datavis_drdy;

volatile sig_atomic_t done = 0;
volatile int first_run = 1;

// SITL
DECLARE_VECTOR(g_readB, unsigned short); // storage to put helmhotz values
unsigned short g_readFS[2];              // storage to put FS X and Y angles
unsigned short g_readCS[9];              // storage to put CS led brightnesses
unsigned char g_Fire;                    // magnetorquer command

// HITL
lsm9ds1 *mag;     // magnetometer
ncv7708 *hbridge; // h-bridge
tca9458a *mux;    // I2C mux
tsl2561 **css;    // coarse sun sensors
ads1115 *adc;     // analog to digital converters
                                                              // SITL
DECLARE_BUFFER(g_W, float);                                            // omega global circular buffer
DECLARE_BUFFER(g_B, double);                                           // magnetic field global circular buffer
DECLARE_BUFFER(g_Bt, double);                                          // Bdot global circular buffer1
DECLARE_VECTOR(g_L_target, float);                                     // angular momentum target vector
DECLARE_VECTOR(g_W_target, float);                                     // angular velocity target vector
DECLARE_BUFFER(g_S, float);                                            // sun vector
float g_CSS[9];                                                        // current CSS lux values, in HITL this will be populated by TSL2561 code
float g_FSS[2];                                                        // current FSS angles, in rad; in HITL this will be populated by NANOSSOC A60 driver
int mag_index = -1, omega_index = -1, bdot_index = -1, sol_index = -1; // circular buffer indices, -1 indicates uninitiated buffer
int B_full = 0, Bdot_full = 0, W_full = 0, S_full = 0;                 // required to deal with the circular buffer problem
uint8_t g_night = 0;                                                   // night mode?
uint8_t g_acs_mode = 0;                                                // Detumble by default
uint8_t g_first_detumble = 1;                                          // first time detumble by default even at night
unsigned long long acs_ct = 0;                                         // counts the number of ACS steps

float MOI[3][3] = {{0.06467720404, 0, 0},
                   {0, 0.06474406267, 0},
                   {0, 0, 0.07921836177}};
float IMOI[3][3] = {{15.461398105297564, 0, 0},
                    {0, 15.461398105297564, 0},
                    {0, 0, 12.623336025344317}};

float bessel_coeff[SH_BUFFER_SIZE]; // coefficients for Bessel filter, declared as floating point

unsigned long long g_t_acs;

#ifdef ACS_DATALOG
FILE *acs_datalog;
#endif

#ifdef DATAVIS
data_packet g_datavis_st;
#endif // DATAVIS

unsigned long long t_comm = 0;
unsigned long long comm_time;

#endif // __SHFLIGHT_GLOBALS_H