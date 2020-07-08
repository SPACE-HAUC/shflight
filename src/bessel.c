/**
 * @file bessel.c
 * @author Sunip K. Mukherjee (sunipkmukherjee@gmail.com)
 * @brief Bessel filter implementation for Attitude Control System.
 * @version 0.2
 * @date 2020-03-19
 * 
 * @copyright Copyright (c) 2020
 * 
 */
#include <bessel.h>
#include <stdlib.h>

/**
 * @brief Coefficients for the Bessel filter, calculated using calculateBessel().
 * 
 */
float bessel_coeff[SH_BUFFER_SIZE]; // coefficients for Bessel filter, declared as floating point

/**
 * @brief Calculates factorial of the input. This function is inlined, and is available only in the scope of bessel.c.
 * 
 * @param i Input
 * @return float Factorial of input
 */
static inline float factorial(int i)
{
    float result = 1;
    i = i + 1;
    while (--i > 0)
        result *= i;
    return result;
}

void calculateBessel(float arr[], int size, int order, float freq_cutoff)
{
    if (order > 5) // max 5th order
        order = 5;
    float *coeff = (float *)calloc(order + 1, sizeof(float)); // declare array to hold numeric coeff
    if (__glibc_unlikely(coeff == NULL))
    {
        perror("[BESSEL] Coeff alloc failed, Bessel coeffs untrusted");
        return;
    }
    // evaluate coeff for order
    for (int i = 0; i < order + 1; i++)
    {
        coeff[i] = factorial(2 * order - i) / (((int)(1 << (order - i))) * factorial(i) * factorial(order - i)); // https://en.wikipedia.org/wiki/Bessel_filter
    }
    // evaluate transfer function coeffs
    for (int j = 0; j < size; j++)
    {
        arr[j] = 0;         // initiate value to 0
        double pow_num = 1; // (j/w_0)^0 is the start
        for (int i = 0; i < order + 1; i++)
        {
            arr[j] += coeff[i] * pow_num; // add the coeff
            pow_num *= j / freq_cutoff;   // multiply by (j/w_0) to avoid power function call
        }
        arr[j] = coeff[0] / arr[j]; // H(s) = T_n(0)/T_n(s/w_0)
    }
    free(coeff);
    return;
}

double dfilterBessel(double arr[], int index)
{
    double val = 0;
    // index is guaranteed to be a number between 0...SH_BUFFER_SIZE by the readSensors() or getOmega() function.
    int coeff_index = 0;
    double coeff_sum = 0; // sum of the coefficients to calculate weighted average
    for (int i = index;;) // initiate the loop, break condition will be dealt with inside the loop
    {
        val += bessel_coeff[coeff_index] * arr[i]; // add weighted value
        coeff_sum += bessel_coeff[coeff_index];    // sum the weights to average with
        i--;                                       // read the previous element
        i = i < 0 ? SH_BUFFER_SIZE - 1 : i;        // allow for circular buffer issues
        coeff_index++;                             // use the next coefficient
        // looped around to the same element, coefficient crosses threshold OR (should never come to this) coeff_index overflows, break loop
        if (i == index || bessel_coeff[coeff_index] < BESSEL_MIN_THRESHOLD || coeff_index > SH_BUFFER_SIZE)
            break;
    }
    return val / coeff_sum;
}

float ffilterBessel(float arr[], int index)
{
    float val = 0;
    // index is guaranteed to be a number between 0...SH_BUFFER_SIZE by the readSensors() or getOmega() function.
    int coeff_index = 0;
    float coeff_sum = 0;  // sum of the coefficients to calculate weighted average
    for (int i = index;;) // initiate the loop, break condition will be dealt with inside the loop
    {
        val += bessel_coeff[coeff_index] * arr[i]; // add weighted value
        coeff_sum += bessel_coeff[coeff_index];    // sum the weights to average with
        i--;                                       // read the previous element
        i = i < 0 ? SH_BUFFER_SIZE - 1 : i;        // allow for circular buffer issues
        coeff_index++;                             // use the next coefficient
        // looped around to the same element, coefficient crosses threshold OR (should never come to this) coeff_index overflows, break loop
        if (i == index || bessel_coeff[coeff_index] < BESSEL_MIN_THRESHOLD || coeff_index > SH_BUFFER_SIZE)
            break;
    }
    return val / coeff_sum;
}