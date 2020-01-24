#include <data_ack.h>

// last_acquisition_status: Stores the status of the last data acquisition. -1 indicates error.
int last_acquisition_status = -1;

/* void* data_ack(void* id): posix thread in charge of measuring system sensors and transitioning
 * between different states. Hands off control to other threads, which in turn wake up this thread.
 * TO-DO: 
 * 1. Implement a timed wait using absolute time from epoch so that this thread does not depend
 * on the other threads
 * 2. Implement state-transition checks at the beginning of each case
 */

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
    return status;
}
// redundant now
int getMagField(void)
{
    int status = 1;
    // put values into g_Bx, g_By and g_Bz at [mag_index] and takes 18 ms to do so (implemented using sleep)
    if (g_bootup && mag_index < 1)
        return status;
    // if we have > 1 values, calculate Bdot
    bdot_index = (bdot_index + 1) % SH_BUFFER_SIZE;
    uint8_t m0, m1;
    m1 = mag_index;
    m0 = (mag_index - 1) < 0 ? SH_BUFFER_SIZE - mag_index + 1 : mag_index - 1;
    double freq;
    switch (g_program_state)
    {
    case SH_ACS_DETUMBLE:
        freq = 1. / DETUMBLE_TIME_STEP;
        break;
    case SH_ACS_COARSE:
        freq = 1. / COARSE_TIME_STEP;
        break;
    case SH_ACS_FINE:
        freq = 1. / FINE_TIME_STEP;
        break;
    case SH_ACS_NIGHT:
        freq = 1. / DETUMBLE_TIME_STEP;
        break;
    default:
        freq = 1. / DETUMBLE_TIME_STEP;
        break;
    }
    VECTOR_OP(g_Bt[bdot_index], g_B[m1], g_B[m0], -);
    VECTOR_MIXED(g_Bt[bdot_index], g_Bt[bdot_index], freq, *);
    return status;
}
// redundant now
// Populates the sun vector buffer at each timestep
int getSunVec(void)
{
    int status;
    if (g_SunSensorBroken)
    {
        g_nightmode = 0;
        return -1;
    }
    // put values into g_Sx, g_Sy, g_Sz at at [sol_index]
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
    do
    {
        if ((!g_bootup) || (last_acquisition_status > 0) || (omega_index < 0) || (g_program_state != SH_SYS_NIGHT)) // if booting up/last acquisition failed/omega is not measured, do not wait for release from other threads
        {
            fprintf(stderr, "Waiting on lock-release from other threads...\n");
            struct timespec timeout;
            timespec_get(&timeout, TIME_UTC);
            timeout.tv_sec += THREAD_TIMEOUT;
            stat_condwait_data_ack = pthread_cond_timedwait(&cond_data_ack, &mutex_data_ack, &timeout);
        }
        if (stat_condwait_data_ack == ETIMEDOUT)
        {
            if (g_program_state == SH_ACS_NIGHT)
                continue;
            // if it was not woken up by ACS threads
            // timeout can occur during XBand/UHF phase.
            // In that case no state change should occur
            FLUSH_BUFFER_ALL;
            g_bootup = 1;
            g_program_state = SH_STATE_CHANGE;
        }
        switch (g_program_state)
        {
        case SH_STATE_CHANGE:
        {
            g_bootup = 1; // indicate that the buffers have been flushed
            FLUSH_BUFFER_ALL;
            // BEGIN ACS DETUMBLE
            while (omega_ready < 1 || omega_index < MIN_OMEGA_STABLE_THRESHOLD)
            {                                                       // measure omega
                g_program_state = SH_ACS_DETUMBLE;                  // used to calculate omega properly
                if ((last_acquisition_status = getMagField()) <= 0) // if can not acquire data, go back to data acquisition again
                {
                    g_program_state = SH_STATE_CHANGE;
                    break;
                }
                getOmega();
                g_program_state = SH_STATE_CHANGE;
                usleep(DETUMBLE_TIME_STEP - MAG_MEASURE_TIME); // total 100 ms sleep till next read
            }
            if ((last_acquisition_status) <= 0) // if can not acquire data, go back to data acquisition again
                break;                          // break out of switch/case and go to beginning of loop
            DECLARE_VECTOR(avgOmega, float);
            for (int i = omega_index; i >= 0; i--)
            {
                VECTOR_OP(avgOmega, avgOmega, g_W[i], +);
            }
            VECTOR_MIXED(avgOmega, avgOmega, omega_index, /); // average of the measurements
            DECLARE_VECTOR(zAxis, float);
            z_zAxis = 1; // Z axis vector for dot product
            if (DOT_PRODUCT(zAxis, avgOmega) < 0.997)
            { // detumble criterion not met
                g_program_state = SH_ACS_DETUMBLE;
                g_bootup = 0;
                break; // state is determined so fall back to main loop.
            }
            // END ACS DETUMBLE

            // BEGIN SUNPOINTING
            // Measure sun vector at 100 msec interval
            while (sol_index < MIN_SOL_STABLE_THRESHOLD && (!g_nightmode))
            {
                g_program_state = SH_ACS_COARSE; // so that getSunVec() measures only coarse sensors
                getSunVec();
                g_program_state = SH_STATE_CHANGE; // return to state change
                usleep(100000);
            }
            if (g_nightmode)
            {
                /*if ((g_uptime = get_usec()-g_bootmoment) < UPTIME_LIMIT )
                    g_program_state = SH_ACS_NIGHT;
                else*/
                g_program_state = SH_SYS_NIGHT;
                g_bootup = 0;
                break;
                /* 
                 * SH_ACS_NIGHT should entertain the possibility that the sun vectors are damaged.
                 * SH_ACS_NIGHT will periodically check if the battery is being charged to ensure that
                 * the sun sensors are working.
                 */
            }
            // now we determine if we need sunpointing
            DECLARE_VECTOR(avgSun, float);
            for (int i = sol_index; i >= 0; i--)
            {
                VECTOR_OP(avgSun, avgSun, g_S[i], +);
            }
            VECTOR_MIXED(avgSun, avgSun, sol_index, /); // average of the measurements
            float sunAngle = DOT_PRODUCT(zAxis, avgSun);
            if (sunAngle < FINE_POINTING_LIMIT && sunAngle > FINE_POINTING_THRESHOLD)
            {
                FLUSH_BUFFER_ALL;
                /*
                 * Flushing buffer is necessary to ensure that
                 * the system re-measures omega and the magnetic field
                 * with the right cadence.
                 */
                g_program_state = SH_ACS_FINE;
                g_bootup = 0;
                break;
            }
            if (sunAngle < FINE_POINTING_THRESHOLD)
            {
                FLUSH_BUFFER_ALL;
                /*
                 * Flushing buffer is necessary to ensure that
                 * the system re-measures omega and the magnetic field
                 * with the right cadence.
                 */
                g_program_state = SH_ACS_COARSE;
                g_bootup = 0;
                break;
            }
            // END SUNPOINTING
            // BEGIN SPIN CONTROL
            if (z_avgOmega != z_g_W_target) // target angular speed does not match
            {
                g_program_state = SH_SPIN_CTRL;
                g_bootup = 0;
                break;
            }
            // END SPIN CONTROL
            // put the system in ready state
            g_program_state = SH_SYS_READY;
            g_bootup = 0;
            break;
            // measure all sensors
            // determine next state. Should be independent of previous state
            // as the previous state guarantees meeting criterion for extended
            // period of time.
            // During this phase no action is performed. Six sequential magfield reads are performed to generate
            // data to get two omegas, provided that meets the DETUMBLE criterion.
            // SUNPOINTING is checked and asserted (depending on if the sun is in the sky)
            // Status of Z angular momentum is also checked. If Z angular speed exceeds target, only then SPIN_CTRL is invoked
            // before SUNPOINT. Otherwise SUNPOINT happens first and then Z angular momentum is adjusted.

            // Every 15 minutes the system should fall from SH_SYS_READY back to SH_STATE_CHANGE so that all sensors are measured
            // and a reasonable decision is made
            break;
        }

        /* 
         * This case deals with the detumble algorithm. 
         */
        case SH_ACS_DETUMBLE:
        {
            if ((last_acquisition_status = getMagField()) <= 0) // if can not acquire data, go back to data acquisition again
                break;
            getOmega();                      // get omega if you made a measurement
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

        case SH_SYS_INIT:
        {
            FLUSH_BUFFER_ALL;
            g_bootup = 1;
            break;
        }
        case SH_ACS_COARSE:
        {
            if ((last_acquisition_status = getMagField()) <= 0) // if can not acquire data, go back to data acquisition again
            {
                usleep(COARSE_TIME_STEP - MAG_MEASURE_TIME);
                break;
            }
            getOmega();
            if (!omega_ready) // this is going to spend ~3s doing nothing
            {
                usleep(COARSE_TIME_STEP - MAG_MEASURE_TIME);
                break;
            }
            /*
             * In coarse mode it should take ~ 4 minute to get the buffer to full.
             * This means every minute the system is going to fall back and check 
             * if the system is still detumbled.
             */
            g_coarseOmegaLimit++;
            if (g_coarseOmegaLimit == 4 * SH_BUFFER_SIZE - 1) // angular speed buffer is full
            {
                g_coarseOmegaLimit = 0;
                g_program_state = SH_STATE_CHANGE;
            }
            int status = getSunVec();
            if (status < 0)
            {
                usleep(COARSE_TIME_STEP - COARSE_MEASURE_TIME - MAG_MEASURE_TIME);
                break;
            }
            // Now we are ready to wake the ACS thread up
            pthread_cond_broadcast(&cond_acs);
            break;
        }

        case SH_ACS_FINE:
        {
            if ((last_acquisition_status = getMagField()) <= 0) // if can not acquire data, go back to data acquisition again
            {
                usleep(FINE_TIME_STEP - MAG_MEASURE_TIME);
                break;
            }
            getOmega();
            if (!omega_ready) // this is going to spend ~3s doing nothing
            {
                usleep(FINE_TIME_STEP - MAG_MEASURE_TIME);
                break;
            }
            /*
             * In fine mode it should take ~ 96*2 seconds to get the buffer to full.
             * This means every 96*2 seconds the system is going to fall back and check 
             * if the system is still detumbled.
             */
            g_fineOmegaLimit++;                              // every time a successful omega measurement is made increment this
            if (g_fineOmegaLimit == 16 * SH_BUFFER_SIZE - 1) // angular speed buffer is full
            {
                g_fineOmegaLimit = 0;
                g_program_state = SH_STATE_CHANGE;
            }
            int status = getSunVec();
            if (status < 0)
            {
                usleep(COARSE_TIME_STEP - COARSE_MEASURE_TIME - MAG_MEASURE_TIME);
                break;
            }
            // Now we are ready to wake the ACS thread up
            pthread_cond_broadcast(&cond_acs);
            break;
        }

        case SH_SYS_NIGHT:
        {
            g_program_state = SH_ACS_COARSE; // so that getSunVec() measures only coarse sensors
            getSunVec();
            g_program_state = SH_ACS_NIGHT; // return to state change
            while (g_nightmode)             // it is still night
            {
                if ((last_acquisition_status = getMagField()) <= 0) // if can not acquire data, go back to data acquisition again
                {
                    usleep(DETUMBLE_TIME_STEP - MAG_MEASURE_TIME);
                    break;
                }
                getOmega();
                if (!omega_ready)
                {
                    usleep(DETUMBLE_TIME_STEP - MAG_MEASURE_TIME);
                }
                // once we have omega, we check if we are tumbling. Then we flush B, Bt and W buffer and start over
                if (omega_index == SH_BUFFER_SIZE - 1)
                {
                    DECLARE_VECTOR(avgOmega, float);
                    for (int i = SH_BUFFER_SIZE - 1; i > SH_BUFFER_SIZE / 2; i--)
                    {
                        VECTOR_OP(avgOmega, avgOmega, g_W[i], +);
                    }
                    VECTOR_MIXED(avgOmega, avgOmega, SH_BUFFER_SIZE / 2, /);
                    NORMALIZE(avgOmega, avgOmega);
                    if (z_avgOmega < 0.996) // starting to tumble!
                    {
                        g_program_state = SH_STATE_CHANGE;
                        break;
                    }
                    FLUSH_BUFFER(g_B);
                    FLUSH_BUFFER(g_W);
                    FLUSH_BUFFER(g_Bt);
                    mag_index = -1;
                    bdot_index = -1;
                    omega_index = -1;
                    break; // exit the while loop
                }
            }
            break;
        }

        case SH_ACS_NIGHT:

            break;

        case SH_SPIN_CTRL:

            break;

        case SH_SYS_READY:

            break;

        case SH_UHF_READY:

            break;

        case SH_XBAND:

            break;

        default: // Default is DETUMBLE
            break;
        }
    } while (1);
    pthread_exit((void *)NULL);
}