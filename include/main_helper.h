#include <stdio.h>
#include <unistd.h>

#define BOOTCOUNT_FNAME "bootcount_fname.txt"
int bootCount(void);

inline float q2isqrt(float);

#define DECLARE_BUFFER(name, type)                                    \
type x_##name[SH_BUFFER_SIZE], y_##name[SH_BUFFER_SIZE], z_##name[SH_BUFFER_SIZE]

#define FLUSH_BUFFER(name)                                          \
for(uint8_t sh__counter = SH_BUFFER_SIZE; sh__counter > 0; )        \
{                                                                   \
    sh__counter--;                                                  \
    x_##name[sh__counter] = 0;                                      \
    y_##name[sh__counter] = 0;                                      \
    z_##name[sh__counter] = 0;                                      \
}        
// dest = s1 X s2; dest, s1, s2 are vectors with names x_dest, y_dest, z_dest and x_s1, y_s1, z_s1 etc
#define CROSS_PRODUCT(dest,s1,s2)                           \
x_##dest=y_##s1*z_##s2 - z_##s1*y_##s2;                     \
y_##dest=z_##s1*x_##s2 - x_##s1*z_##s2;                     \
z_##dest=x_##s1*y_##s2 - y_##s1*x_##s2                     

// dest = s1 (op) s2; dest, s1, s2 are vectors with names x_dest, y_dest, z_dest and x_s1, y_s1, z_s1 etc
#define MATRIX_OP(dest,s1,s2,op)                            \
x_##dest=x_##s1 op x_##s2;                                  \
y_##dest=y_##s1 op y_##s2;                                  \
z_##dest=z_##s1 op z_##s2

// dest = vector, s1 = vector, s2 = scalar, user needs to guarantee that s2 does not depend on s1
#define MATRIX_MIXED(dest, s1, s2, op)                  \
x_##dest=x_##s1 op s2;                                  \
y_##dest=y_##s1 op s2;                                  \
z_##dest=z_##s1 op s2

// dest = norm(source)
#define NORMALIZE(dest, s1)                             \
for(;;){                                                \
double sh__temp = INVNORM(s1);                          \
x_##dest=x_##s1/sh__temp ;                              \
y_##dest=y_##s1/sh__temp ;                              \
z_##dest=z_##s1/sh__temp ;                              \
break;                                                  \
}

// NORM(source) returns magnitude of vector in float32
#define NORM(s)  sqrt(NORM2(s))

// NORM2(source) returns norm^2 of vector in float32
#define NORM2(s) x_##s*x_##s+y_##s*y_##s+z_##s*z_##s

// INVNORM(source) returns the inverse of norm of vector in float32
#define INVNORM(s) q2isqrt(NORM2(s))