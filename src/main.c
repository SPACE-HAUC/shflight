/**
 * @file main.c
 * @author Sunip K. Mukherjee (sunipkmukherjee@gmail.com)
 * @brief main() symbol of the SPACE-HAUC Flight Software.
 * @version 0.2
 * @date 2020-03-19
 * 
 * @copyright Copyright (c) 2020
 * 
 */
#define MAIN_PRIVATE // enable prototypes in main.h and modules in modules.h
#include <main.h>
#include <modules.h>
#undef MAIN_PRIVATE
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#define _GNU_SOURCE
#include <pthread.h>
#include <signal.h>
#include <string.h>

#define THIS_MODULE "main"

int sys_boot_count = -1;
volatile sig_atomic_t done = 0;
pthread_mutex_t done_mutex[1] = {PTHREAD_MUTEX_INITIALIZER};
pthread_cond_t done_cond[1];
__thread int sys_status;

char *sys_bin_name;

volatile uint32_t vermagic = 0x666c75; // 0x666c74

void destroy_modules();

/**
 * @brief Main function executed when shflight.out binary is executed
 * 
 * @return int returns 0 on success, -1 on failure, error code on thread init failures
 */
int main(int argc, char *argv[])
{
    // Check which binary it is
    printf("[Main]: Binary magic: 0x%08x\n", vermagic);
    // wait for system to stabilize
    // sleep(60); // wait for another minute
    // Boot counter
    printf("[Main]: Binary name %s\n", argv[0]);
    sys_bin_name = malloc(strlen(argv[0]) + 1);
    memcpy(sys_bin_name, argv[0], strlen(argv[0]) + 1);
    sys_boot_count = bootCount(); // Holds bootCount to generate a different log file at every boot
    if (sys_boot_count < 0)
    {
        fprintf(stderr, "Boot count returned negative, fatal error. Exiting.\n");
        exit(-1);
    }
    // SIGINT handler register
    struct sigaction saction;
    saction.sa_handler = &catch_sigint;
    sigaction(SIGINT, &saction, NULL);
    // initialize modules
    for (int i = 0; i < num_init; i++)
    {
        int val = module_init[i]();
        if (val < 0)
        {
            sherror("Error in initialization!");
            exit(-1);
        }
    }
    printf("Done init modules\n");
    atexit(destroy_modules);
    // set up threads
    int rc[num_systems];                                         // fork-join return codes
    pthread_t thread[num_systems];                               // thread containers
    pthread_attr_t attr;                                         // thread attribute
    int args[num_systems];                                       // thread arguments (thread id in this case, but can be expanded by passing structs etc)
    void *status;                                                // thread return value
    pthread_attr_init(&attr);                                    // initialize attribute
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE); // create threads to be joinable
    for (int i = 0; i < num_systems; i++)
    {
        args[i] = i; // sending a pointer to i to every thread may end up with duplicate thread ids because of access times
        rc[i] = pthread_create(&thread[i], &attr, module_exec[i], (void *)(&args[i]));
        if (rc[i])
        {
            printf("[Main] Error: Unable to create thread %d: Errno %d\n", i, errno);
            exit(-1);
        }
    }
    pthread_attr_destroy(&attr); // destroy the attribute

    pthread_cond_wait(done_cond, done_mutex);

    for (int i = 0; i < num_systems; i++)
    {
        printf("Cancelling thread %d\n", i);
        fflush(stdout);
        rc[i] = pthread_cancel(thread[i]);
        if (rc[i])
        {
            printf("[Main] Error: Unable to join thread %d, retcode: %d, : Errno %d\n", i, rc[i], errno);
            exit(0);
        }
    }

    return 0;
}

void sys_exit(void)
{
    catch_sigint(SIGINT);
}

/**
 * @brief SIGINT handler, sets the global variable `done` as 1, so that thread loops can break.
 * Wakes up sitl_comm and datavis threads to ensure they exit.
 * 
 * @param sig Receives the signal as input.
 */
void catch_sigint(int sig)
{
    done = 1;
    for (int i = 0; i < num_wakeups; i++)
        pthread_cond_broadcast(wakeups[i]);
    sleep(1);
    pthread_cond_signal(done_cond);
}

void destroy_modules()
{
    // // destroy modules
    // for (int i = 0; i < num_destroy; i++)
    // {
    //     printf("[Main] Running module_destroy for %d\n", i);
    //     module_destroy[i]();
    //     printf("[Main] Run module_destroy for %d\n", i);
    // }
}

/**
 * @brief Prints errors specific to shflight in a fashion similar to perror
 * 
 * @param msg Input message to print along with error description
 */
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
        fprintf(stderr, "%s\n", msg);
        break;
    }
}

int bootCount()
{
    FILE *fp;
    int _bootCount = 0;                      // assume 0 boot
    if (access(BOOTCOUNT_FNAME, F_OK) != -1) // file exists
    {
        fp = fopen(BOOTCOUNT_FNAME, "r+");              // open file for reading
        int read_bytes = fscanf(fp, "%d", &_bootCount); // read bootcount
        if (read_bytes < 0)                             // if no bytes were read
        {
            perror("File not read"); // indicate error
            _bootCount = 0;          // reset boot count
        }
        fclose(fp); // close
    }
    // Update boot file
    fp = fopen(BOOTCOUNT_FNAME, "w"); // open for writing
    fprintf(fp, "%d", ++_bootCount);  // write 1
    fclose(fp);                       // close
    sync();                           // sync file system
    return --_bootCount;              // return 0 on first boot, return 1 on second boot etc
}
