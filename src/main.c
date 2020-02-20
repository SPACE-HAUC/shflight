#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <signal.h>
#include <math.h>
#include <errno.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <termios.h>
#include <main_helper.h>
#include <sys/socket.h>
#include <arpa/inet.h>

int main(void)
{
    // handle sigint
    struct sigaction saction;
    saction.sa_handler = &catch_sigint;
    sigaction(SIGINT, &saction, NULL);

    /* Set up data logging */
    int bc = bootCount();
    char fname[40] = {0};
    sprintf(fname, "logfile%d.txt", bc);

    datalog = fopen(fname, "w");
    /* End setup datalogging */

    z_g_W_target = 1;                       // 1 rad s^-1
    MATVECMUL(g_L_target, MOI, g_W_target); // calculate target angular momentum
    calculateBessel(bessel_coeff, SH_BUFFER_SIZE, 3, BESSEL_FREQ_CUTOFF);

    int rc0, rc1, rc2;
    pthread_t thread0, thread1, thread2;
    pthread_attr_t attr;
    void *status;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
    rc0 = pthread_create(&thread0, &attr, acs_detumble, (void *)0);
    if (rc0)
    {
        printf("Main: Error: Unable to create ACS thread %d: Errno %d\n", rc0, errno);
        exit(-1);
    }
    rc1 = pthread_create(&thread1, &attr, sitl_comm, (void *)0);
    if (rc1)
    {
        printf("Main: Error: Unable to create Serial thread %d: Errno %d\n", rc1, errno);
        exit(-1);
    }
    rc2 = pthread_create(&thread2, &attr, datavis_thread, (void *)0);
    if (rc2)
    {
        printf("Main: Error: Unable to create DataVis thread %d: Errno %d\n", rc2, errno);
        exit(-1);
    }

    pthread_attr_destroy(&attr);

    rc0 = pthread_join(thread0, &status);
    if (rc0)
    {
        printf("Main: Error: Unable to join ACS thread %d: Errno %d\n", rc0, errno);
    }

    rc1 = pthread_join(thread1, &status);
    if (rc1)
    {
        printf("Main: Error: Unable to join Serial thread %d: Errno %d\n", rc1, errno);
    }
    rc2 = pthread_join(thread2, &status);
    if (rc2)
    {
        printf("Main: Error: Unable to join DataVis thread %d: Errno %d\n", rc2, errno);
    }
    fflush(stdout);
    fflush(datalog);
    fclose(datalog);

    return 0;
}