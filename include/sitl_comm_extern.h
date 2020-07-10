/**
 * @file sitl_comm_extern.h
 * @author Sunip K. Mukherjee (sunipkmukherjee@gmail.com)
 * @brief Software-In-The-Loop (SITL) serial communication headers and function prototypes
 * @version 0.2
 * @date 2020-03-19
 * 
 * @copyright Copyright (c) 2020
 * 
 */
#ifndef SITL_COMM_EXTERN_H
#define SITL_COMM_EXTERN_H
#include <pthread.h>
extern pthread_mutex_t serial_read;
extern pthread_mutex_t serial_write;
extern unsigned long long t_comm;
extern unsigned long long comm_time;
#endif // SITL_COMM_EXTERN_H