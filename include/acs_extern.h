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
/**
 * @brief Circular buffer size for ACS sensor data
 * 
 */
#define SH_BUFFER_SIZE 64 // Circular buffer size
// the following extern declarations will show up only in files other than acs.h

#ifndef ACS_H
#include <macros.h>
#include <stdbool.h>
DECLARE_VECTOR2(mag_mes, extern float);

extern uint32_t css_0, css_1, css_2, css_3, css_4, css_5, css_6, css_7;

extern float fss_0, fss_1;

extern bool mux_err_0, mux_err_1, mux_err_2;
#endif

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

#endif // ACS_EXTERN_H