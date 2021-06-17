/**
 * @file main.h
 * @author Sunip K. Mukherjee (sunipkmukherjee@gmail.com)
 * @brief Includes all headers necessary for the core flight software, including ACS, and defines
 * ACS states (which are flight software states), error codes, and relevant error functions.
 * @version 0.1
 * @date 2020-03-19
 * 
 * @copyright Copyright (c) 2020
 * 
 */
#ifndef MAIN_H
#define MAIN_H
#include <signal.h>
#include <macros.h>
/**
 * @brief Describes ACS (system) states.
 * 
 */
typedef enum
{
    STATE_ACS_DETUMBLE, // Detumbling
    STATE_ACS_SUNPOINT, // Sunpointing
    STATE_ACS_NIGHT,    // Night
    STATE_ACS_READY,    // Do nothing
    STATE_XBAND_READY   // Ready to do X-Band things
} SH_ACS_MODES;

/**
 * @brief Describes possible system errors.
 * 
 */
typedef enum
{
    ERROR_MALLOC = -1,
    ERROR_HBRIDGE_INIT = -2,
    ERROR_MUX_INIT = -3,
    ERROR_CSS_INIT = -4,
    ERROR_MAG_INIT = -5,
    ERROR_FSS_INIT = -6,
    ERROR_FSS_CONFIG = -7
} SH_ERRORS;
/**
 * @brief Thread-local system status variable (similar to errno).
 * 
 */
extern __thread int sys_status;
// thread local error printing
void sherror(const char *);
/**
 * @brief Control variable for thread loops.
 * 
 */
extern volatile sig_atomic_t done;
/**
 * @brief System variable containing the current boot count of the system.
 * This variable is provided to all modules by main.
 */ 
extern int sys_boot_count;

#ifndef SHPRINT_FILE
#define SHPRINT_FILE stderr
#endif

#define shprintf(str, ...) \
{ \
    fprintf(SHPRINT_FILE, THIS_MODULE "bootct: %d tstamp %llu: %s, %d: " str, sys_boot_count, get_usec(), __func__, __LINE__, ##__VA_ARGS__); \
    fflush(SHPRINT_FILE); \
}

#ifdef MAIN_PRIVATE
/**
 * @brief Name of the file where bootcount is stored on the file system.
 * 
 */
#define BOOTCOUNT_FNAME "soft_bootcount.txt"

/**
 * @brief Function that returns the current bootcount of the system.
 * Returns current boot count, and increases by 1 and stores it in nvmem.
 * Expected to be invoked only by _main()
 * 
 * @return int Current boot count (C-style)
 */
int bootCount(void);

// interrupt handler for SIGINT
void catch_sigint(int);
#endif // MAIN_PRIVATE
#endif // MAIN_H