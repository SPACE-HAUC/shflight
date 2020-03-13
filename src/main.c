#include <main.h>
#include <main_helper.h>
#include <shflight_consts.h>
#include <shflight_globals.h>
#include <shflight_externs.h>

int main(void)
{
    // handle sigint
    struct sigaction saction;
    saction.sa_handler = &catch_sigint;
    sigaction(SIGINT, &saction, NULL);

    /* Set up data logging */
#ifdef ACS_DATALOG    
    int bc = bootCount();
    char fname[40] = {0};
    sprintf(fname, "logfile%d.txt", bc);

    acs_datalog = fopen(fname, "w");
#endif // ACS_DATALOG
    /* End setup datalogging */

    // init for different threads
    calculateBessel(bessel_coeff, SH_BUFFER_SIZE, 3, BESSEL_FREQ_CUTOFF);

    z_g_W_target = 1; // 1 rad s^-1

    sys_status = acs_init(); // initialize ACS devices
    if (sys_status < 0)
    {
        sherror("ACS Init");
        sys_status = 0;
        exit(-1);
    }
    // set up threads
    int rc[NUM_SYSTEMS];                                         // fork-join return codes
    pthread_t thread[NUM_SYSTEMS];                               // thread containers
    pthread_attr_t attr;                                         // thread attribute
    int args[NUM_SYSTEMS];                                       // thread arguments (thread id in this case, but can be expanded by passing structs etc)
    void *status;                                                // thread return value
    pthread_attr_init(&attr);                                    // initialize attribute
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE); // create threads to be joinable

    // create array of systems
    void *fn_array[NUM_SYSTEMS];

    // fill the array of systems
    fn_array[0] = acs_thread;
#ifdef DATAVIS
    fn_array[1] = datavis_thread;
#endif // DATAVIS
#ifdef SITL
    fn_array[2] = sitl_comm;
#endif // SITL
    for (int i = 0; i < NUM_SYSTEMS; i++)
    {
        args[i] = i; // sending a pointer to i to every thread may end up with duplicate thread ids because of access times
        rc[i] = pthread_create(&thread[i], &attr, fn_array[i], (void *)(&args[i]));
        if (rc[i])
        {
            printf("[Main] Error: Unable to create thread %d: Errno %d\n", i, errno);
            exit(-1);
        }
    }

    pthread_attr_destroy(&attr); // destroy the attribute

    for (int i = 0; i < NUM_SYSTEMS; i++)
    {
        rc[i] = pthread_join(thread[i], &status);
        if (rc[i])
        {
            printf("[Main] Error: Unable to join thread %d: Errno %d\n", i, errno);
            exit(-1);
        }
    }

    // destroy for different threads
    acs_destroy();
    return 0;
}

void catch_sigint(int sig)
{
    done = 1;
#ifdef SITL
    pthread_cond_broadcast(&data_available);
#endif // SITL
#ifdef DATAVIS
    pthread_cond_broadcast(&datavis_drdy);
#endif // DATAVIS
}

void sherror(const char *msg)
{
    switch (sys_status)
    {
    case ERROR_MALLOC:
        fprintf(stderr, "%s: Error allocating memory\n", msg);
        break;

    case ERROR_HBRIDGE_INIT:
        fprintf(stderr, "%s: Error initializing h-bridge\n", msg);
        break;

    case ERROR_MUX_INIT:
        fprintf(stderr, "%s: Error initializing mux\n", msg);
        break;

    case ERROR_CSS_INIT:
        fprintf(stderr, "%s: Error initializing CSS\n", msg);
        break;

    case ERROR_FSS_INIT:
        fprintf(stderr, "%s: Error initializing FSS\n", msg);
        break;

    case ERROR_FSS_CONFIG:
        fprintf(stderr, "%s: Error configuring FSS\n", msg);
        break;

    default:
        break;
    }
}