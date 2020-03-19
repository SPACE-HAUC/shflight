/**
 * @file main_helper.h
 * @author Sunip K. Mukherjee (sunipkmukherjee@gmail.com)
 * @brief Defines vector macros and other helper functions for the flight software.
 * @version 0.1
 * @date 2020-03-19
 * 
 * @copyright Copyright (c) 2020
 * 
 */
#ifndef __MAIN_HELPER_H
#define __MAIN_HELPER_H

#include <stdio.h>
#include <stdint.h>
#include <time.h>
#include <unistd.h>

/**
 * @brief Name of the file where bootcount is stored on the file system.
 * 
 */
#define BOOTCOUNT_FNAME "bootcount_fname.txt"

/**
 * @brief Function that returns the current bootcount of the system.
 * Returns current boot count, and increases by 1 and stores it in nvmem.
 * Expected to be invoked only by _main()
 * 
 * @return int Current boot count (C-style)
 */
int bootCount(void);

/**
 * @brief float q2isqrt(float): Returns the inverse square root of a floating point number.
 * Depending on whether MATH_SQRT is declared, it will use sqrt() function
 * from gcc-math or bit-level hack and 3 rounds of Newton-Raphson to directly
 * calculate inverse square root. The bit-level routine yields consistently better
 * performance and 0.00001% maximum error. Set MATH_SQRT at compile time to use the
 * sqrt() function.
 * 
 * @param x Floating point number (32-bit) whose inverse square root is calculated
 * 
 * @return float Inverse square root of the input
 * 
 */
#ifndef MATH_SQRT
inline float q2isqrt(float x)
{
    float xhalf = x * 0.5f;               // calculate 1/2 x before bit-level changes
    int i = *(int *)&x;                   // convert float to int for bit level operation
    i = 0x5f375a86 - ((*(int *)&x) >> 1); // bit level manipulation to get initial guess (ref: http://www.lomont.org/papers/2003/InvSqrt.pdf)
    x = *(float *)&i;                     // convert back to float
    x = x * (1.5f - xhalf * x * x);       // 1 round of Newton approximation
    x = x * (1.5f - xhalf * x * x);       // 2 round of Newton approximation
    x = x * (1.5f - xhalf * x * x);       // 3 round of Newton approximation
    return x;
}
#else // MATH_SQRT
#include <math.h>
inline float q2isqrt(float x)
{
    return 1.0 / sqrt(x);
};
#endif // MATH_SQRT

/**
 * @brief Returns time elapsed from 1970-1-1, 00:00:00 UTC to now (UTC) in microseconds.
 * Execution time ~18 us on RPi.
 * 
 * @return uint64_t Number of microseconds elapsed from epoch.
 */
inline uint64_t get_usec(void)
{
    struct timespec ts;
    timespec_get(&ts, TIME_UTC);
    return (uint64_t)ts.tv_sec * 1000000L + ((uint64_t)ts.tv_nsec) / 1000;
}

/**
 * @brief Calculates floating point average of a float array.
 * 
 * @param arr Pointer to array whose average is calculated
 * @param size Length of the input array
 * @return float Average of the input array
 */
inline float faverage(float arr[], int size)
{
    float result = 0;
    int count = size;
    while (count--)
        result += arr[count];
    return result / size;
}
/**
 * @brief Calculates double precision point average of a float array.
 * 
 * @param arr Pointer to array whose average is calculated
 * @param size Length of the input array
 * @return double Average of the input array
 */
inline double daverage(double arr[], int size)
{
    double result = 0;
    int count = size;
    while (count--)
        result += arr[count];
    return result / size;
}

// DECLARE_BUFFER(name, type): Declares a buffer with name and type. Prepends x_, y_, z_ to the names (vector buffer!)
/**
 * @brief Declares a buffer with name and type. Prepends x_, y_, z_ to the names (vector buffer!)
 * This macro allocates three arrays x_name, y_name and z_name of type and size SH_BUFFER_SIZE.
 * 
 * @param name Name of the buffer (prepends x_, y_, z_ for vector)
 * @param type Data type of the buffer
 */
#define DECLARE_BUFFER(name, type) \
    type x_##name[SH_BUFFER_SIZE], y_##name[SH_BUFFER_SIZE], z_##name[SH_BUFFER_SIZE]

/**
 * @brief Clears a vector.
 * 
 * @param name Name of the vector
 * 
 */
#define VECTOR_CLEAR(name) \
    x_##name = 0;          \
    y_##name = 0;          \
    z_##name = 0

/**
 * @brief Declares a vector with the name and type. A vector is a three-variable entity with x_, y_, z_ prepended to the names.
 * This function initializes the variables to 0, which makes it not ideal for use in extern definitions.
 * 
 * @param name Name of the vector
 * @param type Data type of the vector
 * 
 */
#define DECLARE_VECTOR(name, type) \
    type x_##name = 0, y_##name = 0, z_##name = 0

/**
 * @brief Declares a vector with the name and type. A vector is a three-variable entity with x_, y_, z_ prepended to the names.
 * This function does not initialize the variables to 0, which makes it ideal for use in extern definitions.
 * 
 * @param name Name of the vector
 * @param type Data type of the vector
 * 
 */
#define DECLARE_VECTOR2(name, type) \
    type x_##name, y_##name, z_##name

/**
 * @brief Flushes a buffer declared using DECLARE_BUFFER().
 * Does not reset index counters or buffer full indicators,
 * which needs to be done by hand on a case by case basis.
 * 
 */
#define FLUSH_BUFFER(name)                                       \
    for (uint8_t sh__counter = SH_BUFFER_SIZE; sh__counter > 0;) \
    {                                                            \
        sh__counter--;                                           \
        x_##name[sh__counter] = 0;                               \
        y_##name[sh__counter] = 0;                               \
        z_##name[sh__counter] = 0;                               \
    }

/**
 * @brief Resets all buffers and resets indices, while not clearing buffer full indicators.
 * 
 */
#define FLUSH_BUFFER_ALL \
    FLUSH_BUFFER(g_B);   \
    FLUSH_BUFFER(g_Bt);  \
    FLUSH_BUFFER(g_W);   \
    FLUSH_BUFFER(g_S);   \
    mag_index = -1;      \
    sol_index = -1;      \
    bdot_index = -1;     \
    omega_index = -1;    \
    g_nightmode = 0;     \
    omega_ready = -1;

/**
 * @brief Calculates cross product of two vectors created using DECLARE_VECTOR().
 * The destination vector must be a different vector from any of the inputs.
 * 
 * @param dest Destination vector name, declared using DECLARE_VECTOR()
 * @param s1   First source vector name, declared using DECLARE_VECTOR()
 * @param s2   Second source vector name, declared using DECLARE_VECTOR()
 * 
 */
#define CROSS_PRODUCT(dest, s1, s2)               \
    x_##dest = y_##s1 * z_##s2 - z_##s1 * y_##s2; \
    y_##dest = z_##s1 * x_##s2 - x_##s1 * z_##s2; \
    z_##dest = x_##s1 * y_##s2 - y_##s1 * x_##s2

/**
 * @brief Calculates the floating point (32-bit) dot product of two vectors.
 * 
 * @param s1 Name of the first vector, declared using DECLARE_VECTOR()
 * @param s2 Name of the second vector, declared using DECLARE_VECTOR()
 */
#define DOT_PRODUCT(s1, s2) \
    (float)(x_##s1 * x_##s2 + y_##s1 * y_##s2 + z_##s1 * z_##s2)

/**
 * @brief Performs a vector operation on the source vectors and stores in destination vector.
 * Since the operations are performed element-by-element, the destination vector can be
 * the same as any of the source vectors.
 * 
 * @param dest Destination vector, declared using DECLARE_VECTOR()
 * @param s1   First vector, declared using DECLARE_VECTOR()
 * @param s2   Second vector, declared using DECLARE_VECTOR()
 * @param op   Operation to perform on an element-by-element basis, e.g. +, -, *, /.
 * Note: For division there is no check for division by zero.
 * 
 */
#define VECTOR_OP(dest, s1, s2, op) \
    x_##dest = x_##s1 op x_##s2;    \
    y_##dest = y_##s1 op y_##s2;    \
    z_##dest = z_##s1 op z_##s2

// dest = vector, s1 = vector, s2 = scalar, user needs to guarantee that s2 does not depend on s1
/**
 * @brief Performs element-by-element operation on a vector with a scalar and stores in the destination vector.
 * Since the operations are performed element-by-element, the scalar can not depend on the source vector.
 * 
 * @param dest Destination vector, declared using DECLARE_VECTOR()
 * @param s1   Input vector, declared using DECLARE_VECTOR()
 * @param s2   Input scalar
 * @param op   Operation to perform on an element-by-element basis, e.g. +, -, *, /.
 * Note: For division there is no check for division by zero.
 * 
 */
#define VECTOR_MIXED(dest, s1, s2, op) \
    x_##dest = x_##s1 op s2;           \
    y_##dest = y_##s1 op s2;           \
    z_##dest = z_##s1 op s2

/**
 * @brief Normalizes the input vector and stores it in the output vector.
 * Works for null vectors as well.
 * 
 * @param dest Destination vector, declared using DECLARE_VECTOR()
 * @param s1   Source vector, declared using DECLARE_VECTOR()
 * 
 */
#define NORMALIZE(dest, s1)                            \
    for (float sh__temp = INVNORM(s1); sh__temp != 0;) \
    {                                                  \
        x_##dest = x_##s1 * sh__temp;                  \
        y_##dest = y_##s1 * sh__temp;                  \
        z_##dest = z_##s1 * sh__temp;                  \
        break;                                         \
    }

/**
 * @brief Calculates the norm of the input vector in 32-bit floating point.
 * 
 * @param s Input vector, declared using DECLARE_VECTOR()
 * 
 * @return float Norm of the input vector
 * 
 */
#define NORM(s) sqrt(NORM2(s))

/**
 * @brief Calculates the square of the norm of the input vector in 32-bit floating point.
 * 
 * @param s Input vector, declared using DECLARE_VECTOR()
 * 
 * @return float Square of the norm of the input vector
 * 
 */
#define NORM2(s) x_##s *x_##s + y_##s *y_##s + z_##s *z_##s

/**
 * @brief Calculates the inverse norm of the input vector in 32-bit floating point. Does not check for null vectors.
 * 
 * @param s Input vector, declared using DECLARE_VECTOR()
 * 
 * @return float Inverse norm of the input vector
 * 
 */
#define INVNORM(s) q2isqrt(NORM2(s))

// MATVECMUL(dest, s1, s2) multiplies vector s2 by matrix s1 (3x3) and stores
// the result in vector dest. Uses the typical vector naming convention for dest, s2
// s1 is a 3x3 matrix defined in the usual C way (s1[0][0] is the first element)
// *** dest and s2 MUST BE different vectors ***
/**
 * @brief Muliplies the input vector by the input matrix (3x3) (left to right).
 * 
 * @param dest Output vector, declared using DECLARE_VECTOR()
 * @param s1   3 x 3 input matrix
 * @param s2   Input vector, declared using DECLARE_VECTOR(). Has to be different from the destination.
 * 
 */
#define MATVECMUL(dest, s1, s2)                                           \
    x_##dest = s1[0][0] * x_##s2 + s1[0][1] * y_##s2 + s1[0][2] * z_##s2; \
    y_##dest = s1[1][0] * x_##s2 + s1[1][1] * y_##s2 + s1[1][2] * z_##s2; \
    z_##dest = s1[2][0] * x_##s2 + s1[2][1] * y_##s2 + s1[2][2] * z_##s2

/**
 * @brief Calculates 32-bit float average of an input buffer.
 * 
 * @param dest Output vector, declared using DECLARE_VECTOR()
 * @param src  Input buffer, declared using DECLARE_BUFFER()
 * @param size Size of the input buffer (equals to SH_BUFFER_SIZE for a buffer declared using DECLARE_BUFFER())
 * 
 */
#define FAVERAGE_BUFFER(dest, src, size) \
    x_##dest = faverage(x_##src, size);  \
    y_##dest = faverage(y_##src, size);  \
    z_##dest = faverage(z_##src, size)

/**
 * @brief Calculates double precision average of an input buffer.
 * 
 * @param dest Output vector, declared using DECLARE_VECTOR()
 * @param src  Input buffer, declared using DECLARE_BUFFER()
 * @param size Size of the input buffer (equals to SH_BUFFER_SIZE for a buffer declared using DECLARE_BUFFER())
 * 
 */
#define DAVERAGE_BUFFER(dest, src, size) \
    x_##dest = daverage(x_##src, size);  \
    y_##dest = daverage(y_##src, size);  \
    z_##dest = daverage(z_##src, size)

#endif // __MAIN_HELPER_H