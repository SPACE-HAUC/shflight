#ifndef MAIN_H
#define MAIN_H

#include <stdio.h>
#include <stdint.h>
#include <math.h>
#include <pthread.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <stdint.h>
#include <unistd.h>
#include <errno.h>
#include <main_helper.h>
#include <string.h>
#include <termios.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include <bessel.h>

#include "ncv7708.h"  // h-bridge
#include "lsm9ds1.h"  // magnetometer
#include "tsl2561.h"  // coarse sun sensor
#include "ads1115.h"  // fine sun sensor - adc
#include "tca9458a.h" // I2C mux

// enumeration of system states
typedef enum
{
    STATE_ACS_DETUMBLE, // Detumbling
    STATE_ACS_SUNPOINT, // Sunpointing
    STATE_ACS_NIGHT,    // Night
    STATE_ACS_READY,    // Do nothing
    STATE_XBAND_READY   // Ready to do X-Band things
} SH_ACS_MODES;

// system errors
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

// interrupt handler for SIGINT
void catch_sigint(int);

// number of threads in the system
#ifndef NUM_SYSTEMS
#ifdef SITL
#define DATAVIS
#define NUM_SYSTEMS 3
#else // HITL
#define DATAVIS
#define NUM_SYSTEMS 2
#endif
#endif

#include <acs.h>

#ifdef DATAVIS
#include <datavis.h>
#endif

#ifdef SITL
#include <sitl_comm.h>
#endif

// thread local error printing
void sherror(const char *);

#endif // MAIN_H