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
            // flush buffers
            g_bootup = 1 ;
            g_program_state = SH_STATE_CHANGE ;
        }
        switch (g_program_state)
        {
        case SH_STATE_CHANGE:
            g_bootup = 1 ; // indicate that the buffers have been flushed
            // measure all sensors
            // determine next state. Should be independent of previous state
            // as the previous state guarantees meeting criterion for extended
            // period of time.
            // During this phase no action is performed. Six sequential magfield reads are performed to generate
            // data to get two omegas, provided that meets the DETUMBLE criterion.
            // SUNPOINTING is checked and asserted (depending on if the sun is in the sky)
            // Status of Z angular momentum is also checked. If Z angular speed exceeds target, only then SPIN_CTRL is invoked
            // before SUNPOINT. Otherwise SUNPOINT happens first and then Z angular momentum is adjusted.

            // Every 15 minutes the system should fall back to SH_STATE_CHANGE so that all sensors are measured
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
