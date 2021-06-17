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
 * @brief 
 * 
 * @param moi 
 * @return int 
 */
int acs_get_moi(float *moi);
/**
 * @brief 
 * 
 * @param moi 
 * @return int 
 */
int acs_set_moi(float *moi);
/**
 * @brief 
 * 
 * @param imoi 
 * @return int 
 */
int acs_get_imoi(float *imoi);
/**
 * @brief 
 * 
 * @param imoi 
 * @return int 
 */
int acs_set_imoi(float *imoi);
/**
 * @brief 
 * 
 * @return float 
 */
float acs_get_dipole(void);
/**
 * @brief set dipole moment
 * 
 * @param d dipole moment in Am-2
 * @return float 
 */
float acs_set_dipole(float d);
/**
 * @brief Get ACS timestep
 * 
 * @return uint32_t timestep in us
 */
uint32_t acs_get_tstep(void);
/**
 * @brief Set ACS timestep
 * 
 * @param t time in ms
 * @return uint32_t time in us
 */
uint32_t acs_set_tstep(uint8_t t);
/**
 * @brief Measure time in us
 * 
 * @return uint32_t 
 */
uint32_t acs_get_measure_time(void);
/**
 * @brief Set measurement time 
 * 
 * @param t time in ms
 * @return uint32_t time set in us
 */
uint32_t acs_set_measure_time(uint8_t t);
/**
 * @brief Get detumble angular speed accuracy in percentage
 * 
 * @return uint8_t number between 1-100
 */
uint8_t acs_get_leeway(void);
/**
 * @brief Set detumble angular speed accuracy in percentage
 * 
 * @param leeway number between 1-100
 * @return uint8_t set leeway
 */
uint8_t acs_set_leeway(uint8_t leeway);
/**
 * @brief Get omega_z target
 * 
 * @return float angular speed in rad/s
 */
float acs_get_wtarget(void);
/**
 * @brief Set omega_z target (rad/s)
 * 
 * @param t omega_z target in rad/s
 * @return float set omega_z target
 */
float acs_set_wtarget(float t);
/**
 * @brief Get detumble accuracy
 * 
 * @return uint8_t angle in degrees
 */
uint8_t acs_get_detumble_ang();
/**
 * @brief Set detumble accuracy
 * 
 * @param ang angle in degrees
 * @return uint8_t set angle in degrees
 */
uint8_t acs_set_detumble_ang(uint8_t ang);
/**
 * @brief Get solar pointing accuracy
 * 
 * @return uint8_t angle in degrees
 */
uint8_t acs_get_sun_angle();
/**
 * @brief Set solar pointing accuracy
 * 
 * @param ang angle in degrees
 * @return uint8_t set angle in degrees
 */
uint8_t acs_set_sun_angle(uint8_t ang);

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
void *acs_thread(void *id);

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