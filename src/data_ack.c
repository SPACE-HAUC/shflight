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
void * data_ack(void * id)
{
    do {
        if ((!g_bootup)||(last_acquisition_status > 0)||(omega_index < 0)) // if booting up/last acquisition failed/omega is not measured, do not wait for release from other threads
        {
            fprintf(stderr, "Waiting on lock-release from other threads...\n") ;
            struct timespec timeout;
            timespec_get(&timeout, TIME_UTC);
            timeout.tv_sec += THREAD_TIMEOUT ;
            stat_condwait_data_ack = pthread_cond_timedwait(&cond_data_ack, &mutex_data_ack, &timeout) ;
        }
        if (stat_condwait_data_ack == ETIMEDOUT)
        {
            // if it was not woken up by ACS threads
            // timeout can occur during XBand/UHF phase.
            // In that case no state change should occur
            FLUSH_BUFFER_ALL ;
            g_bootup = 1 ;
            g_program_state = SH_STATE_CHANGE ;
        }
        switch (g_program_state)
        {
        case SH_STATE_CHANGE:
            g_bootup = 1 ; // indicate that the buffers have been flushed
            FLUSH_BUFFER_ALL;
            // BEGIN ACS DETUMBLE
            while(omega_ready < 1 || omega_index < MIN_OMEGA_STABLE_THRESHOLD){ // measure omega
                g_program_state = SH_ACS_DETUMBLE ; // used to calculate omega properly
                if((last_acquisition_status=getMagField())<=0) // if can not acquire data, go back to data acquisition again
                {
                    g_program_state = SH_STATE_CHANGE ;
                    break ;
                }
                getOmega() ;
                g_program_state = SH_STATE_CHANGE ;
                usleep(DETUMBLE_TIME_STEP-MAG_MEASURE_TIME) ; // total 100 ms sleep till next read
            }
            if((last_acquisition_status)<=0) // if can not acquire data, go back to data acquisition again
                break ; // break out of switch/case and go to beginning of loop
            DECLARE_VECTOR(avgOmega, float);
            for ( int i = omega_index ; i >= 0 ; i-- )
            {
                VECTOR_OP(avgOmega, avgOmega, g_W[i], +) ;
            }
            VECTOR_MIXED(avgOmega, avgOmega, omega_index, /) ; // average of the measurements
            DECLARE_VECTOR(zAxis, float); z_zAxis = 1 ; // Z axis vector for dot product
            if (DOT_PRODUCT(zAxis, avgOmega)<0.997){ // detumble criterion not met
                g_program_state = SH_ACS_DETUMBLE ;
                g_bootup = 0 ;
                break ; // state is determined so fall back to main loop.
            }
            // END ACS DETUMBLE

            // BEGIN SUNPOINTING
            // Measure sun vector at 100 msec interval
            while(sol_index < MIN_SOL_STABLE_THRESHOLD && (!g_nightmode))
            {
                getSunVec();
                usleep(100000) ;
            }
            if ( g_nightmode )
            {
                g_program_state = SH_ACS_NIGHT ;
                g_bootup = 0 ;
                break ;
                /* 
                 * SH_ACS_NIGHT should entertain the possibility that the sun vectors are damaged.
                 * SH_ACS_NIGHT will periodically check if the battery is being charged to ensure that
                 * the sun sensors are working.
                 */ 
            }
            // now we determine if we need sunpointing
            DECLARE_VECTOR(avgSun, float);
            for ( int i = sol_index ; i >= 0 ; i-- )
            {
                VECTOR_OP(avgSun, avgSun, g_S[i], +) ;
            }
            VECTOR_MIXED(avgSun, avgSun, sol_index, /) ; // average of the measurements
            float sunAngle = DOT_PRODUCT(zAxis, avgSun);
            if (sunAngle < FINE_POINTING_LIMIT && sunAngle > FINE_POINTING_THRESHOLD )
            {
                g_program_state = SH_ACS_FINE ;
                g_bootup = 0 ;
                break ;
            }
            if ( sunAngle < FINE_POINTING_THRESHOLD )
            {
                g_program_state = SH_ACS_COARSE;
                g_bootup = 0 ;
                break ;
            }
            // END SUNPOINTING
            // BEGIN SPIN CONTROL
            if ( z_avgOmega != z_g_W_target ) // target angular speed does not match
            {
                g_program_state = SH_SPIN_CTRL ;
                g_bootup = 0 ;
                break ;
            }
            // END SPIN CONTROL
            // put the system in ready state
            g_program_state = SH_SYS_READY ;
            g_bootup = 0 ;
            break ;
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

        /* 
         * This case deals with the detumble algorithm. 
         */
        case SH_ACS_DETUMBLE:
            if((last_acquisition_status=getMagField())<=0) // if can not acquire data, go back to data acquisition again
                break ;
            getOmega() ; // get omega if you made a measurement
            if (omega_index < 0 && g_bootup) // omega not ready
            {
                usleep(DETUMBLE_TIME_STEP-MAG_MEASURE_TIME) ; // total 100 ms sleep till next read
                break ; // fall back to data_ack
            }
            /*
             * error value index at max, we can check if we are in steady state.
             * This means the steady state condition will be checked every 64*100 ms = 6.4 seconds.
             * Steady state requires a specific threshold to be met. 
             * The steady state condition is set to be half of buffer size
             */ 
            if (L_err_index == SH_BUFFER_SIZE - 1)
            {
                int counter = 0 ;
                for (int i = SH_BUFFER_SIZE; i > 0 ; )
                {
                    i--;
                    if (g_L_pointing[i] > POINTING_THRESHOLD)
                        counter++ ;
                }
                if (counter > (SH_BUFFER_SIZE >> 1)) // SH_BUFFER_SIZE/2
                {
                    usleep(DETUMBLE_TIME_STEP-MAG_MEASURE_TIME);
                    g_program_state = SH_STATE_CHANGE;
                    break ;
                } 
            }
            pthread_cond_broadcast(&cond_acs); // signal ACS thread to wake up and perform action
            break; // fall back to data_ack
        case SH_SYS_INIT:
            // flush buffers
            // reset indices
            break;
        
        case SH_ACS_COARSE:
            break;
        
        case SH_ACS_FINE:

            break;

        case SH_ACS_NIGHT:
            
            break;
        
        case SH_SPIN_CTRL:

            break ;
        
        case SH_SYS_READY:

            break ;
        
        case SH_UHF_READY:

            break ;
        
        case SH_XBAND:

            break ;
        
        default: // Default is DETUMBLE
            break;
        }
    } while(1) ;
    pthread_exit((void*)NULL);
}
