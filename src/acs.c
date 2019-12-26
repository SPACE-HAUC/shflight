#include <main.h>
#include <acs.h>
void * acs(void * id)
{
    do {
        pthread_cond_wait(&cond_acs, &mutex_acs);
        // do ACS things

        pthread_cond_broadcast(&cond_data_ack);
    } while(1) ;
    pthread_exit((void*)NULL) ;
}