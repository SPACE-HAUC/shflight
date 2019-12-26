#include <data_ack.h>

// last_acquisition_status: Stores the status of the last data acquisition. -1 indicates error.
int last_acquisition_status = -1;

// void* data_ack(void* id): posix thread in charge of measuring system sensors and transitioning
// between different states. Hands off control to other threads, which in turn wake up this thread.
// TO-DO: Implement a timed wait using absolute time from epoch so that this thread does not depend
// on the other threads
void * data_ack(void * id)
{
    do {
        if ((!g_bootup)||(last_acquisition_status > 0)||(omega_index < 0)) // if booting up/last acquisition failed/omega is not measured, do not wait for release from other threads
        {
            fprintf(stderr, "Waiting on lock-release from other threads...\n") ;
            pthread_cond_wait(&cond_data_ack, &mutex_data_ack) ;
        }
        switch (g_program_state)
        {
        case SH_STATE_CHANGE:
            g_bootup = 1 ; // indicate that the buffers have been flushed
            break;

        case SH_ACS_DETUMBLE:
            if((last_acquisition_status=getMagField())<=0) // if can not acquire data, go back to data acquisition again
                continue ;
            getOmega() ; // get omega if you made a measurement
            if (omega_index < 0) // omega not ready
                usleep(DETUMBLE_TIME_STEP) ; // 100 ms sleep till next read
            break;
        
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
