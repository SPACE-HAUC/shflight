#ifndef __SHFLIGHT_BESSEL_H
#define __SHFLIGHT_BESSEL_H
#include "main.h"
#include "shflight_consts.h"

extern float bessel_coeff[SH_BUFFER_SIZE]; // coefficients for Bessel filter, declared as floating point

extern inline int factorial(int i)
{
    int result = 1;
    i = i + 1;
    while (--i > 0)
        result *= i;
    return result;
}

/*
 Calculates discrete Bessel filter coefficients for the given order and cutoff frequency.
 Stores the values in the supplied array with given size.
 bessel_coeff[0] = 1.
 Called once by main() at the beginning of execution.
 */
void calculateBessel(float arr[], int size, int order, float freq_cutoff);

#define BESSEL_MIN_THRESHOLD 0.001 // randomly chosen minimum value for valid coefficient
#define BESSEL_FREQ_CUTOFF 5       // cutoff frequency 5 == 5*DETUMBLE_TIME_STEP seconds cycle == 2 Hz at 100ms loop speed

/*
 Returns the Bessel average of the data at the requested array index, double precision.
 */
double dfilterBessel(double arr[], int index);

/*
 Returns the Bessel average of the data at the requested array index, floating point.
 */
float ffilterBessel(float arr[], int index);

// Apply Bessel filter over a vector buffer
#define APPLY_DBESSEL(name, index)                    \
    x_##name[index] = dfilterBessel(x_##name, index); \
    y_##name[index] = dfilterBessel(y_##name, index); \
    z_##name[index] = dfilterBessel(z_##name, index)

// Apply Bessel filter over a vector buffer
#define APPLY_FBESSEL(name, index)                    \
    x_##name[index] = ffilterBessel(x_##name, index); \
    y_##name[index] = ffilterBessel(y_##name, index); \
    z_##name[index] = ffilterBessel(z_##name, index)

// sort a1 and order a2 using the same order as a1.
void insertionSort(int a1[], int a2[]);

#endif // __SHFLIGHT_BESSEL_H