/**
 * @file acs.h
 * @author Sunip K. Mukherjee (sunipkmukherjee@gmail.com)
 * @brief Header file including headers and function prototypes of the Attitude Control System.
 * @version 0.1
 * @date 2020-03-19
 * 
 * @copyright Copyright (c) 2020
 * 
 */
#ifndef ACS_H
#define ACS_H
#include <acs_extern.h> // will define SH_BUFFER_SIZE
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
 * @brief Initializes the devices required to run the attitude control system.
 * 
 * This function initializes the target angular momentum using MOI defined in
 * shflight_globals.h and the target angular speed set in main.c. Then this
 * function initializes all the relevant devices for ACS to function. 
 * 
 * @return int 1 on success, error codes defined in SH_ERRORS on error.
 */
int acs_init(void);

/**
 * @brief Attitude Control System Thread.
 * 
 * This thread executes the ACS functions in a loop controlled by the
 * variable `done`, which is controlled by the interrupt handler.
 * 
 * @param id Thread ID passed as a pointer to an integer.
 * 
 * @return NULL
 */
void *acs_thread(void *);

/**
 * @brief Powers down ACS devices and closes relevant file descriptors.
 * 
 */
void acs_destroy(void);

/**
 * @brief Sorts the first array and reorders the second array according to the first array.
 * 
 * @param a1 Pointer to integer array to sort.
 * @param a2 Pointer to integer array to reorder.
 */
void insertionSort(int a1[], int a2[]);

/**
 * @brief Fire magnetorquer in the direction dictated by the input vector.
 * 
 * @param name Name of the input vector
 * 
 */
#define HBRIDGE_ENABLE(name) \
    hbridge_enable(x_##name, y_##name, z_##name);

/**
 * @brief Fire magnetorquer in X, Y, and Z directions using the input integers.
 * 
 * @param x Fires in the +X or -X direction depending on the input being +1 or -1, and does nothing if x = 0
 * @param y Fires in the +Y or -Y direction depending on the input being +1 or -1, and does nothing if y = 0
 * @param z Fires in the +Z or -Z direction depending on the input being +1 or -1, and does nothing if z = 0
 * @return int Status of the operation, returns 1 on success.
 */
int hbridge_enable(int x, int y, int z);

/**
 * @brief Disables magnetorquer in the axis indicated by the input.
 * 
 * @param num Integer, 0 indicates X axis, 1 indicates Y axis, 2 indicates Z axis. In hardware, a number > 2 causes all three torquers to shut down.
 * @return int Status of the operation, returns 1 on success.
 */
int HBRIDGE_DISABLE(int num);

/**
 * @brief Calculates \f$\omega\f$ using \f$\dot{\vec{B}}\f$ and stores in the circular buffer.
 * 
 * Calculates current angular speed. Requires current and previous measurements of \f$\dot{\vec{B}}\f$.
 * The calculated angular speed is put inside the global circular buffer. Sets W_full to indicate the
 * buffer becoming full the first time.
 * 
 */
void getOmega(void);

/**
 * @brief Calculates sun vector using coarse sun sensor and fine sun sensor measurements.
 * Favors the fine sun sensor measurements if exists. The value is inserted into a circular
 * buffer.
 * 
 * 
 */
void getSVec(void);

/**
 * @brief Reads hardware sensors and puts the values in the global storage, upon which
 * calls the getOmega() and getSVec() functions to calculate angular speed and sun vector.
 * 
 * @return int Returns 1 for success, and -1 for error.
 */
int readSensors(void);

/**
 * @brief This function checks if the ACS should transition from one state to the other at
 * every iteration. The function executes only when the \f$\vec{\omega}\f$ and sun vector
 * buffers are full.
 * 
 */
void checkTransition(void);

#endif // ACS_H