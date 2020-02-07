#ifndef __MAIN_HELPER_H
#define __MAIN_HELPER_H
#include <stdio.h>
#include <stdint.h>
#include <time.h>
#include <unistd.h>

// Name of the file where bootcount is stored on the file system
#define BOOTCOUNT_FNAME "bootcount_fname.txt"

// Function that returns the current bootcount of the system.
// returns current boot count, and increases by 1 and stores it in nvmem
// expected to be invoked only by _main()
int bootCount(void);

/* float q2isqrt(float): Returns the inverse square root of a floating point number.
 * Depending on whether MATH_SQRT is declared, it will use sqrt() function
 * from gcc-math or bit-level hack and 3 rounds of Newton-Raphson to directly
 * calculate inverse square root. The bit-level routine yields consistently better
 * performance and 0.00001% maximum error.
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
#else  // MATH_SQRT
inline float q2isqrt(float x)
{
    return 1.0 / sqrt(x);
};
#endif // MATH_SQRT

// returns time elapsed from 1970-1-1, 00:00:00 UTC to now (UTC) in microseconds.
inline uint64_t get_usec(void)
{
    struct timespec ts;
    timespec_get(&ts, TIME_UTC);
    return (uint64_t)ts.tv_sec * 1000000L + ((uint64_t)ts.tv_nsec) / 1000;
}

// calculate floating point average of a float buffer of size size
inline float faverage(float arr[], int size)
{
    float result = 0;
    int count = size;
    while (count--)
        result += arr[count];
    return result / size;
}

// calculate double precision average of a double buffer of size size
inline double daverage(double arr[], int size)
{
    double result = 0;
    int count = size;
    while (count--)
        result += arr[count];
    return result / size;
}

// DECLARE_BUFFER(name, type): Declares a buffer with name and type. Prepends x_, y_, z_ to the names (vector buffer!)
#define DECLARE_BUFFER(name, type) \
    type x_##name[SH_BUFFER_SIZE], y_##name[SH_BUFFER_SIZE], z_##name[SH_BUFFER_SIZE]

// VECTOR_CLEAR(name) : Clears a vector with name
#define VECTOR_CLEAR(name) \
    x_##name = 0;          \
    y_##name = 0;          \
    z_##name = 0

// Declares a vector with the name and type. A vector is a three-variable entity with x_, y_, z_ prepended to the names
#define DECLARE_VECTOR(name, type) \
    type x_##name = 0, y_##name = 0, z_##name = 0

// Declares a vector with the name and type. A vector is a three-variable entity with x_, y_, z_ prepended to the names
#define DECLARE_VECTOR2(name, type) \
    type x_##name, y_##name, z_##name

// FLUSH_BUFFER(name): Flushes buffer with name, prepended by standard x_, y_, z_ suffixes.
// FLUSH_BUFFER does _not_ reset the index counters, which needs to be done by hand
#define FLUSH_BUFFER(name)                                       \
    for (uint8_t sh__counter = SH_BUFFER_SIZE; sh__counter > 0;) \
    {                                                            \
        sh__counter--;                                           \
        x_##name[sh__counter] = 0;                               \
        y_##name[sh__counter] = 0;                               \
        z_##name[sh__counter] = 0;                               \
    }

//FLUSH_BUFFER_ALL: Resets all buffers and resets indices
// by default assumes it is daytime
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
// dest = s1 X s2; dest, s1, s2 are vectors with names x_dest, y_dest, z_dest and x_s1, y_s1, z_s1 etc
// *** dest must not be the same as either s1 or s2 ***
#define CROSS_PRODUCT(dest, s1, s2)               \
    x_##dest = y_##s1 * z_##s2 - z_##s1 * y_##s2; \
    y_##dest = z_##s1 * x_##s2 - x_##s1 * z_##s2; \
    z_##dest = x_##s1 * y_##s2 - y_##s1 * x_##s2

/*
 * returns the dot product of vectors s1 and s2 as a float
 */
#define DOT_PRODUCT(s1, s2) \
    (float)(x_##s1 * x_##s2 + y_##s1 * y_##s2 + z_##s1 * z_##s2)

// dest = s1 (op) s2; dest, s1, s2 are vectors with names x_dest, y_dest, z_dest and x_s1, y_s1, z_s1 etc
#define VECTOR_OP(dest, s1, s2, op) \
    x_##dest = x_##s1 op x_##s2;    \
    y_##dest = y_##s1 op y_##s2;    \
    z_##dest = z_##s1 op z_##s2

// dest = vector, s1 = vector, s2 = scalar, user needs to guarantee that s2 does not depend on s1
#define VECTOR_MIXED(dest, s1, s2, op) \
    x_##dest = x_##s1 op s2;           \
    y_##dest = y_##s1 op s2;           \
    z_##dest = z_##s1 op s2

// dest = norm(source), stores normalized vector of s1 in dest.
#define NORMALIZE(dest, s1)                            \
    for (float sh__temp = INVNORM(s1); sh__temp != 0;) \
    {                                                  \
        x_##dest = x_##s1 * sh__temp;                  \
        y_##dest = y_##s1 * sh__temp;                  \
        z_##dest = z_##s1 * sh__temp;                  \
        break;                                         \
    }

// NORM(source) returns magnitude of vector in float32
#define NORM(s) sqrt(NORM2(s))

// NORM2(source) returns norm^2 of vector in float32
#define NORM2(s) x_##s *x_##s + y_##s *y_##s + z_##s *z_##s

// INVNORM(source) returns the inverse of norm of vector in float32
#define INVNORM(s) q2isqrt(NORM2(s))

// MATVECMUL(dest, s1, s2) multiplies vector s2 by matrix s1 (3x3) and stores
// the result in vector dest. Uses the typical vector naming convention for dest, s2
// s1 is a 3x3 matrix defined in the usual C way (s1[0][0] is the first element)
// *** dest and s2 MUST BE different vectors ***
#define MATVECMUL(dest, s1, s2)                                           \
    x_##dest = s1[0][0] * x_##s2 + s1[0][1] * y_##s2 + s1[0][2] * z_##s2; \
    y_##dest = s1[1][0] * x_##s2 + s1[1][1] * y_##s2 + s1[1][2] * z_##s2; \
    z_##dest = s1[2][0] * x_##s2 + s1[2][1] * y_##s2 + s1[2][2] * z_##s2

/* 
 * Calculates float average of buffer of name src and size size into vector dest.
 */
#define FAVERAGE_BUFFER(dest, src, size) \
    x_##dest = faverage(x_##src, size);  \
    y_##dest = faverage(y_##src, size);  \
    z_##dest = faverage(z_##src, size)

/* 
 * Calculates double precision average of buffer of name src and size size into vector dest.
 */
#define DAVERAGE_BUFFER(dest, src, size) \
    x_##dest = daverage(x_##src, size);  \
    y_##dest = daverage(y_##src, size);  \
    z_##dest = daverage(z_##src, size)
#endif // __MAIN_HELPER_H