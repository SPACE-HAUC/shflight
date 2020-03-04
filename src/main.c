#include <main.h>
#include <main_helper.h>
#include <shflight_consts.h>
#include <shflight_globals.h>

int main(void)
{
    // handle sigint
    struct sigaction saction;
    saction.sa_handler = &catch_sigint;
    sigaction(SIGINT, &saction, NULL);

    int rc0, rc1, rc2;
    pthread_t thread0, thread1, thread2;
    pthread_attr_t attr;
    void *status;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

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

    return 0;
}