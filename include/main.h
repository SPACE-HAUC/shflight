#ifndef MAIN_H
#define MAIN_H

#include <stdio.h>
#include <stdint.h>
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

#include "ncv7708.h"  // h-bridge
#include "lsm9ds1.h"  // magnetometer
#include "tsl2561.h"  // coarse sun sensor
#include "ads1115.h"  // fine sun sensor - adc
#include "tca9458a.h" // I2C mux

typedef enum
{
    STATE_ACS_DETUMBLE, // Detumbling
    STATE_ACS_SUNPOINT, // Sunpointing
    STATE_ACS_NIGHT,    // Night
    STATE_ACS_READY,    // Do nothing
    STATE_XBAND_READY   // Ready to do X-Band things
} SH_ACS_MODES;

#endif // MAIN_H