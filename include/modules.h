/**
 * @file modules.h
 * @author Sunip K. Mukherjee (sunipkmukherjee@gmail.com)
 * @brief Includes all headers necessary to interface modules with the main program
 * ACS states (which are flight software states), error codes, and relevant error functions.
 * @version 0.1
 * @date 2020-03-19
 * 
 * @copyright Copyright (c) 2020
 * 
 */
#ifndef __SH_MODULES_H
#define __SH_MODULES_H
#ifdef MAIN_PRIVATE
#include <pthread.h>
#include <acs_iface.h>
#include <datavis_iface.h>
#include <sitl_comm_iface.h>
typedef int (*init_func)(void);     // typedef to create array of init functions
typedef void (*destroy_func)(void); // typedef to create array of destroy functions

/**
 * @brief Registers init functions of a given module
 */
init_func module_init[] = {
    &acs_init};
/**
 * @brief Number of modules with an associated init function
 */
const int num_init = sizeof(module_init) / sizeof(init_func);

/**
 * @brief Registers init functions of a given module
 */
destroy_func module_destroy[] = {
    &acs_destroy};
/**
 * @brief Number of modules with an associated destroy function
 */
const int num_destroy = sizeof(module_destroy) / sizeof(destroy_func);

/**
 * @brief Registers exec functions of a given module
 */
void *module_exec[] = {
    acs_thread
#ifdef DATAVIS
    ,
    datavis_thread
#endif
#ifdef SITL
    ,
    sitl_comm
#endif
};
/**
 * @brief Number of enabled modules
 */
const int num_systems = sizeof(module_exec) / sizeof(void *);

/**
 * @brief List of condition locks for modules to be woken up by signal handler
 */
pthread_cond_t *wakeups[] = {
#ifdef SITL
    &data_available
#endif // SITL
#ifdef DATAVIS
    ,
    &datavis_drdy
#endif // DATAVIS
};
const int num_wakeups = sizeof(wakeups) / sizeof(pthread_cond_t *);
#endif
#endif // __SH_MODULES_H