/**
 * @file acs_iface.h
 * @author Sunip K. Mukherjee (sunipkmukherjee@gmail.com)
 * @brief Header file including constants, mutexes and function prototypes
 * that initialize, destroy and execute the Attitude Control System module.
 * @version 0.1
 * @date 2020-03-19
 * 
 * @copyright Copyright (c) 2020
 * 
 */
#ifndef ACS_IFACE_H
#define ACS_IFACE_H
#include <pthread.h>
extern pthread_cond_t data_available; // wake up from lock to SIGINT
int acs_init(void);                   // init function for module
void acs_destroy(void);               // destroy function for module
void *acs_thread(void *);             // exec function for module
#endif                                // ACS_IFACE_H