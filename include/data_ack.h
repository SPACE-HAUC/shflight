#ifndef __DATA_ACK_H
#define __DATA_ACK_H
#include <main.h>

DECLARE_BUFFER(g_B,uint16_t); // Raw magnetic field measurements

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
    // put values into g_Bx, g_By and g_Bz at [mag_index]
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

int getSunVec(void)
{
    int status ;
    // put values into g_Sx, g_Sy, g_Sz at at [sol_index]
    return status ;
}

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
    
}

#endif // __DATA_ACK_H