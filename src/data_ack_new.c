#include <data_ack.h>

int last_acquisition_status = -1;

int readSensors(void)
{
    // read magfield
    int status = 1;
    mag_index = (mag_index + 1) % SH_BUFFER_SIZE;
    sol_index = (sol_index + 1) % SH_BUFFER_SIZE;
    pthread_mutex_lock(&serial_read);
    VECTOR_CLEAR(g_B[mag_index]);                                  // clear the current B
    VECTOR_OP(g_B[mag_index], g_B[mag_index], g_readB, +);         // load B - equivalent reading from sensor
    VECTOR_MIXED(g_B[mag_index], g_B[mag_index], 65e-6 / 2048, *); // recover B
    VECTOR_CLEAR(g_S[sol_index]);                                  // for now, this is all we are doing since sun vector unimplemented in sim
    pthread_mutex_unlock(&serial_read);
    // put values into g_Bx, g_By and g_Bz at [mag_index] and takes 18 ms to do so (implemented using sleep)
    if (g_bootup && mag_index < 1)
        return status;
    // if we have > 1 values, calculate Bdot
    bdot_index = (bdot_index + 1) % SH_BUFFER_SIZE;
    int8_t m0, m1;
    m1 = mag_index;
    m0 = (mag_index - 1) < 0 ? SH_BUFFER_SIZE - mag_index - 1 : mag_index - 1;
    double freq = 1. / DETUMBLE_TIME_STEP;
    VECTOR_OP(g_Bt[bdot_index], g_B[m1], g_B[m0], -);
    VECTOR_MIXED(g_Bt[bdot_index], g_Bt[bdot_index], freq, *);
    // also get omega in the same go
    getOmega() ;
    return status;
}

// Gyroscope function. Leverages measurements of B to generate omega
// with corrections using moment of inertia
void getOmega(void)
{
    if (g_bootup && mag_index < 2) // not enough measurements
        return;
    g_bootup = 0; // once we have measurements, we declare that we have booted up
    // now we measure omega
    omega_index = (1 + omega_index) % SH_BUFFER_SIZE;
    int8_t m0, m1;
    m1 = bdot_index;
    m0 = (bdot_index - 1) < 0 ? SH_BUFFER_SIZE - bdot_index - 1 : bdot_index - 1;
    float freq;
    freq = 1. / DETUMBLE_TIME_STEP;
    CROSS_PRODUCT(g_W[omega_index], g_Bt[m1], g_Bt[m0]); // apply cross product
    float invnorm = INVNORM(g_Bt[m0]);
    VECTOR_MIXED(g_W[omega_index], g_W[omega_index], freq * invnorm, *); // omega = (B_t dot x B_t-dt dot)*freq/Norm(B_t dot)
    // Apply correction
    DECLARE_VECTOR(omega_corr0, float);                            // declare temporary space for correction vector
    MATVECMUL(omega_corr0, MOI, g_W[m1]);                          // MOI X w[t-1]
    DECLARE_VECTOR(omega_corr1, float);                            // declare temporary space for correction vector
    CROSS_PRODUCT(omega_corr1, g_W[m1], omega_corr0);              // store into temp 1
    MATVECMUL(omega_corr1, MOI, omega_corr0);                      // store back into temp 0
    VECTOR_MIXED(omega_corr1, omega_corr1, -freq, *);              // omega_corr = freq*MOI*(-w[t-1] X MOI*w[t-1])
    VECTOR_OP(g_W[omega_index], g_W[omega_index], omega_corr1, +); // add the correction term to omega
    return;
}

void *data_ack(void *id)
{
    do // main loop 
    {
        // wait for a timeout if 1. we havea all sensor values, 2. last acquisition went well, 3. we have omega (someone acted!)
        if ((!g_bootup) || (last_acquisition_status > 0) || (omega_index < 0) /*|| (g_program_state != SH_ACS_NIGHT)*/) // if booting up/last acquisition failed/omega is not measured, do not wait for release from other threads
        {
            fprintf(stderr, "Waiting on lock-release from other threads...\n");
            struct timespec timeout;
            timespec_get(&timeout, TIME_UTC);
            if ( g_program_state != SH_XBAND )
                timeout.tv_sec += THREAD_TIMEOUT;
            else
                timeout.tv_sec += XBAND_TIMEOUT ;
            stat_condwait_data_ack = pthread_cond_timedwait(&cond_data_ack, &mutex_data_ack, &timeout);
        }
        if (stat_condwait_data_ack == ETIMEDOUT)
        {
            // timed out, so reset ACS.
            FLUSH_BUFFER_ALL;
            g_bootup = 1;
            g_program_state = SH_STATE_CHANGE;
        }
        switch (g_program_state)
        {
        case SH_STATE_CHANGE:
        {   
            last_acquisition_status = readSensors() ; // get new set of data

            break;
        }
        case SH_ACS_DETUMBLE:
        {
            if ((last_acquisition_status = readSensors()) <= 0) // if can not acquire data, go back to data acquisition again
                break;
            if (omega_index < 0 && g_bootup) // omega not ready
            {
                usleep(DETUMBLE_TIME_STEP - MAG_MEASURE_TIME); // total 100 ms sleep till next read
                break;                                         // fall back to data_ack
            }    
            /*
             * error value index at max, we can check if we are in steady state.
             * This means the steady state condition will be checked every 64*100 ms = 6.4 seconds.
             * Steady state requires a specific threshold to be met. 
             * The steady state condition is set to be half of buffer size
             */
            if (L_err_index == SH_BUFFER_SIZE - 1)
            {
                int counter = 0;
                for (int i = SH_BUFFER_SIZE; i > 0;)
                {
                    i--;
                    if (g_L_pointing[i] > POINTING_THRESHOLD)
                        counter++;
                }
                if (counter > (SH_BUFFER_SIZE >> 1)) // SH_BUFFER_SIZE/2
                {
                    usleep(DETUMBLE_TIME_STEP - MAG_MEASURE_TIME);
                    g_program_state = SH_STATE_CHANGE;
                    break;
                }
            }
            pthread_cond_broadcast(&cond_acs); // signal ACS thread to wake up and perform action
            break;                             // fall back to data_ack
        } 
        default:
            break;
        }
    } while (1 /*will be replaced with a ^C handler*/);
    pthread_exit(NULL);
}