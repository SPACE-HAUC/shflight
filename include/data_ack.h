#ifndef __DATA_ACK_H
#define __DATA_ACK_H
#include <main.h>

#define THREAD_TIMEOUT 5   // 5 seconds absolute timeout on the threads

DECLARE_BUFFER(g_B,float); // Raw magnetic field measurements

DECLARE_BUFFER(g_W,float); // Omega measurements from B

DECLARE_BUFFER(g_Bt,float); // bdot measurements

DECLARE_BUFFER(g_S,float); // Sun vector

/*
 * Measures magnetic field from the LSM9DF1 sensor and
 * returns the status word. Populates the cyclic magnetic 
 * field buffer and calculates Bdot, which also goes 
 * in a circular buffer.
 */
int getMagField(void)
{
    int status ;
    // put values into g_Bx, g_By and g_Bz at [mag_index] and takes 18 ms to do so (implemented using sleep)
    if (g_bootup && mag_index < 1)
        return status ;
    // if we have > 1 values, calculate Bdot
    bdot_index = (++bdot_index) % SH_BUFFER_SIZE ;
    uint8_t m0, m1;
    m1 = mag_index ;
    m0 = (mag_index - 1) < 0 ? SH_BUFFER_SIZE - mag_index + 1 : mag_index - 1 ;
    double freq ;
    switch (g_program_state)
    {
    case SH_ACS_DETUMBLE:
        freq = 1./DETUMBLE_TIME_STEP ;
        break;
    case SH_ACS_COARSE:
        freq = 1./COARSE_TIME_STEP ;
        break ;
    case SH_ACS_FINE:
        freq = 1./FINE_TIME_STEP ;
        break;
    case SH_ACS_NIGHT:
        freq = 1./DETUMBLE_TIME_STEP ;
        break ;
    default:
        freq = 1./DETUMBLE_TIME_STEP ;
        break;
    }
    VECTOR_OP(g_Bt[bdot_index], g_B[m1], g_B[m0], -);
    VECTOR_MIXED(g_Bt[bdot_index], g_Bt[bdot_index], freq, *);
    return status ;
}

// Populates the sun vector buffer at each timestep
int getSunVec(void)
{
    int status ;
    // put values into g_Sx, g_Sy, g_Sz at at [sol_index]
    return status ;
}

// Gyroscope function. Leverages measurements of B to generate omega
// with corrections using moment of inertia
void getOmega(void)
{
    if (g_bootup && mag_index < 2 ) // not enough measurements
        return ;
    g_bootup = 0 ; // once we have measurements, we declare that we have booted up
    // now we measure omega
    omega_index = (++omega_index)%SH_BUFFER_SIZE ;
    uint8_t m0, m1 ;
    m1 = bdot_index ;
    m0 = (bdot_index - 1) < 0 ? SH_BUFFER_SIZE - bdot_index + 1 : bdot_index - 1 ;
    float freq ;
    switch (g_program_state)
    {
    case SH_ACS_DETUMBLE:
        freq = 1./DETUMBLE_TIME_STEP ;
        break;
    case SH_ACS_COARSE:
        freq = 1./COARSE_TIME_STEP ;
        break ;
    case SH_ACS_FINE:
        freq = 1./FINE_TIME_STEP ;
        break;
    case SH_ACS_NIGHT:
        freq = 1./DETUMBLE_TIME_STEP ;
        break ;
    default:
        freq = 1./DETUMBLE_TIME_STEP ;
        break;
    }
    CROSS_PRODUCT(g_W[omega_index],g_Bt[m1],g_Bt[m0]); // apply cross product
    float norm = INVNORM(g_Bt[m0]) ;
    VECTOR_MIXED(g_W[omega_index],g_W[omega_index],freq*norm,*); // omega = (B_t dot x B_t-dt dot)*freq/Norm(B_t dot)
    // Apply correction
    DECLARE_VECTOR(omega_corr0,float); // declare temporary space for correction vector
    MATVECMUL(omega_corr0,MOI,g_W[m1]); // MOI X w[t-1]
    DECLARE_VECTOR(omega_corr1,float); // declare temporary space for correction vector
    CROSS_PRODUCT(omega_corr1,g_W[m1],omega_corr0); // store into temp 1
    MATVECMUL(omega_corr1,MOI,omega_corr0); // store back into temp 0
    VECTOR_MIXED(omega_corr1,omega_corr1,-freq,*); // omega_corr = freq*MOI*(-w[t-1] X MOI*w[t-1])
    VECTOR_OP(g_W[omega_index],g_W[omega_index],omega_corr1,+); // add the correction term to omega
    return ;
}

#endif // __DATA_ACK_H