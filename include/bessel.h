/**
 * @file bessel.h
 * @author Sunip K. Mukherjee (sunipkmukherjee@gmail.com)
 * @brief Bessel filter implementation for Attitude Control System.
 * @version 0.1
 * @date 2020-03-19
 * 
 * @copyright Copyright (c) 2020
 * 
 */
#ifndef __SHFLIGHT_BESSEL_H
#define __SHFLIGHT_BESSEL_H
#include <acs_extern.h>
/**
 * @brief Bessel coefficient minimum value threshold for computation
 * 
 */
#ifndef BESSEL_MIN_THRESHOLD
#define BESSEL_MIN_THRESHOLD 0.001 // randomly chosen minimum value for valid coefficient
#endif
/**
 * @brief Bessel filter cutoff frequency
 * 
 */
#ifndef BESSEL_FREQ_CUTOFF
#define BESSEL_FREQ_CUTOFF 5 // cutoff frequency 5 == 5*DETUMBLE_TIME_STEP seconds cycle == 2 Hz at 100ms loop speed
#endif

extern float bessel_coeff[SH_BUFFER_SIZE]; // coefficients for Bessel filter, declared as floating point

/**
 * @brief Calculates discrete Bessel filter coefficients for the given order and cutoff frequency.
 * 
 * @param arr Stores the filter coefficients
 * @param size Size of the filter coefficients array
 * @param order Order of the Bessel filter
 * @param freq_cutoff Cut-off frequency of the Bessel filter
 */
void calculateBessel(float arr[], int size, int order, float freq_cutoff);

/**
 * @brief Returns the filtered value at the current index using past values
 * 
 * @param arr Input array
 * @param index Index of current value in the array
 * @return double Filtered value
 */
double dfilterBessel(double arr[], int index);

/**
 * @brief Returns the filtered value at the current index using past values
 * 
 * @param arr Input array
 * @param index Index of current value in the array
 * @return double Filtered value
 */
float ffilterBessel(float arr[], int index);

/**
 * @brief Applies double precision Bessel filter on a buffer declared using DECLARE_BUFFER(), and stores the filtered value at the current index.
 * 
 * @param name Name of the buffer
 * @param index Index of the current value in the buffer
 * 
 * 
 */
#define APPLY_DBESSEL(name, index)                    \
    x_##name[index] = dfilterBessel(x_##name, index); \
    y_##name[index] = dfilterBessel(y_##name, index); \
    z_##name[index] = dfilterBessel(z_##name, index)

/**
 * @brief Applies floating point Bessel filter on a buffer declared using DECLARE_BUFFER(), and stores the filtered value at the current index.
 * 
 * @param name Name of the buffer
 * @param index Index of the current value in the buffer
 * 
 * 
 */
#define APPLY_FBESSEL(name, index)                    \
    x_##name[index] = ffilterBessel(x_##name, index); \
    y_##name[index] = ffilterBessel(y_##name, index); \
    z_##name[index] = ffilterBessel(z_##name, index)

#endif // __SHFLIGHT_BESSEL_H