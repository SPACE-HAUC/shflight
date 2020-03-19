/**
 * @file shflight_globals.h
 * @author Sunip K. Mukherjee (sunipkmukherjee@gmail.com)
 * @brief Allocates memory for the global variables used in the flight code for inter-thread communication.
 * @version 0.1
 * @date 2020-03-19
 * 
 * @copyright Copyright (c) 2020
 * 
 */
#ifndef __SHFLIGHT_GLOBALS_H
#define __SHFLIGHT_GLOBALS_H

#include <main.h>

/**
 * @brief Thread-local system status variable (similar to errno).
 * 
 */
__thread int sys_status;

/**
 * @brief Mutex to ensure atomicity of serial data read into the system.
 * 
 */
pthread_mutex_t serial_read;
/**
 * @brief Mutex to ensure atomicity of magnetorquer output for serial communication.
 * 
 */
pthread_mutex_t serial_write;
pthread_mutex_t data_check;
/**
 * @brief Condition variable to synchronize ACS and Serial thread in SITL.
 * 
 */
pthread_cond_t data_available;

/**
 * @brief Mutex to ensure atomicity of DataVis and ACS variable access.
 * 
 */
pthread_mutex_t datavis_mutex;
/**
 * @brief Condition variable used by ACS to signal to DataVis that data is ready.
 * 
 */
pthread_cond_t datavis_drdy;
/**
 * @brief Control variable for thread loops.
 * 
 */
volatile sig_atomic_t done = 0;
/**
 * @brief This variable is unset by the ACS thread at first execution.
 * 
 */
volatile int first_run = 1;

// SITL
/**
 * @brief Declares vector to store magnetic field reading from serial.
 * 
 */
DECLARE_VECTOR(g_readB, unsigned short); // storage to put helmhotz values
/**
 * @brief Fine sun sensor angles read over serial.
 * 
 */
unsigned short g_readFS[2]; // storage to put FS X and Y angles
/**
 * @brief Coarse sun sensor lux values read over serial.
 * 
 */
unsigned short g_readCS[9]; // storage to put CS led brightnesses
/**
 * @brief Magnetorquer command, format: 0b00ZZYYXX, 00 indicates not fired, 01 indicates fire in positive dir, 10 indicates fire in negative dir.
 * 
 */
unsigned char g_Fire; // magnetorquer command

// HITL
/**
 * @brief Magnetometer device struct.
 * 
 */
lsm9ds1 *mag; // magnetometer
/**
 * @brief H-Bridge device struct.
 * 
 */
ncv7708 *hbridge; // h-bridge
/**
 * @brief I2C Mux device struct.
 * 
 */
tca9458a *mux; // I2C mux
/**
 * @brief Array of coarse sun sensor device struct.
 * 
 */
tsl2561 **css; // coarse sun sensors
/**
 * @brief I2C ADC struct for fine sun sensor.
 * 
 */
ads1115 *adc; // analog to digital converters
              // SITL
/**
 * @brief Creates buffer for \f$\vec{\omega}\f$.
 * 
 */
DECLARE_BUFFER(g_W, float); // omega global circular buffer
/**
 * @brief Creates buffer for \f$\vec{B}\f$.
 * 
 */
DECLARE_BUFFER(g_B, double); // magnetic field global circular buffer
/**
 * @brief Creates buffer for \f$\vec{\dot{B}}\f$.
 * 
 */
DECLARE_BUFFER(g_Bt, double); // Bdot global circular buffer1
/**
 * @brief Creates vector for target angular momentum.
 * 
 */
DECLARE_VECTOR(g_L_target, float); // angular momentum target vector
/**
 * @brief Creates vector for target angular speed.
 * 
 */
DECLARE_VECTOR(g_W_target, float); // angular velocity target vector
/**
 * @brief Creates buffer for sun vector.
 * 
 */
DECLARE_BUFFER(g_S, float); // sun vector
/**
 * @brief Storage for current coarse sun sensor lux measurements.
 * 
 */
float g_CSS[9]; // current CSS lux values, in HITL this will be populated by TSL2561 code
/**
 * @brief Storage for current fine sun sensor angle measurements.
 * 
 */
float g_FSS[2]; // current FSS angles, in rad; in HITL this will be populated by NANOSSOC A60 driver
/**
 * @brief Current index of the \f$\vec{B}\f$ circular buffer.
 * 
 */
int mag_index = -1;
/**
 * @brief Current index of the \f$\vec{\omega}\f$ circular buffer.
 * 
 */
int omega_index = -1;
/**
 * @brief Current index of the \f$\vec{\dot{B}}\f$ circular buffer.
 * 
 */
int bdot_index = -1;
/**
 * @brief Current index of the sun vector circular buffer.
 * 
 */
int sol_index = -1; // circular buffer indices, -1 indicates uninitiated buffer
/**
 * @brief Indicates if the \f$\vec{B}\f$ circular buffer is full.
 * 
 */
int B_full = 0;
/**
 * @brief Indicates if the \f$\vec{\dot{B}}\f$ circular buffer is full.
 * 
 */
int Bdot_full = 0;
/**
 * @brief Indicates if the \f$\vec{\omega}\f$ circular buffer is full.
 * 
 */
int W_full = 0;
/**
 * @brief Indicates if the sun vector circular buffer is full.
 * 
 */
int S_full = 0; // required to deal with the circular buffer problem
/**
 * @brief This variable is set by checkTransition() if the satellite does not detect the sun.
 * 
 */
uint8_t g_night = 0; // night mode?
/**
 * @brief This variable contains the current state of the flight system.
 * 
 */
uint8_t g_acs_mode = 0; // Detumble by default
/**
 * @brief This variable is unset when the system is detumbled for the first time after a power cycle.
 * 
 */
uint8_t g_first_detumble = 1; // first time detumble by default even at night
/**
 * @brief Counts the number of cycles on the ACS thread.
 * 
 */
unsigned long long acs_ct = 0; // counts the number of ACS steps
/**
 * @brief Moment of inertia of the satellite (SI).
 * 
 */
float MOI[3][3] = {{0.06467720404, 0, 0},
                   {0, 0.06474406267, 0},
                   {0, 0, 0.07921836177}};
/**
 * @brief Inverse of the moment of inertia of the satellite (SI).
 * 
 */
float IMOI[3][3] = {{15.461398105297564, 0, 0},
                    {0, 15.461398105297564, 0},
                    {0, 0, 12.623336025344317}};
/**
 * @brief Coefficients for the Bessel filter, calculated using calculateBessel().
 * 
 */
float bessel_coeff[SH_BUFFER_SIZE]; // coefficients for Bessel filter, declared as floating point
/**
 * @brief Current timestamp after readSensors() in ACS thread, used to keep track of time taken by ACS loop.
 * 
 */
unsigned long long g_t_acs;

#ifdef ACS_DATALOG
/**
 * @brief ACS Datalog file pointer.
 * 
 */
FILE *acs_datalog;
#endif

#ifdef DATAVIS
/**
 * @brief DataVis data structure.
 * 
 */
data_packet g_datavis_st;
#endif // DATAVIS
/**
 * @brief SITL communication time.
 * 
 */
unsigned long long t_comm = 0;
unsigned long long comm_time;

#endif // __SHFLIGHT_GLOBALS_H