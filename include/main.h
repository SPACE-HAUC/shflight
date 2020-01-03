#ifndef __MAIN_H
#define __MAIN_H
#include <stdio.h>
#include <stdint.h>
#include <pthread.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <stdint.h>
#include <unistd.h>
#include <errno.h>
#include <spacehauc_devices.h>

#include <main_helper.h>

#define SH_BUFFER_SIZE 64 // size of circular buffers

#define DETUMBLE_TIME_STEP 100000  // 100 msec, in usec
#define COARSE_TIME_STEP 1000000   // 1000 msec, in usec
#define FINE_TIME_STEP 200000      // 200 msec, in usec
#define MAG_MEASURE_TIME 20000     // 20 msec, in usec
#define SUNPOINT_DUTY_CYCLE 20000  // 20 msec, in usec
#define COARSE_MEASURE_TIME 100000 // 100 msec, in usec
#define FINE_MEASURE_TIME 5000     // 5 ms, in usec (single shot mode)
typedef enum
{
    SH_SYS_INIT,
    SH_STATE_CHANGE,
    SH_ACS_DETUMBLE,
    SH_ACS_COARSE,
    SH_ACS_FINE,
    SH_ACS_NIGHT,
    SH_SPIN_CTRL,
    SH_SYS_READY,
    SH_SYS_NIGHT,
    SH_UHF_READY,
    SH_XBAND
} SH_SYS_STATES;

volatile uint8_t g_boot_count;
volatile uint8_t g_program_state;
volatile uint8_t g_previous_state;
volatile uint8_t g_bootup;
volatile uint8_t g_SunSensorBroken;
volatile uint8_t g_BatteryCharging;
volatile uint8_t g_fineOmegaLimit; // this variable determines the limit at which the fine sunpointing resets to determine detumbling
volatile uint8_t g_coarseOmegaLimit;

uint64_t g_uptime;                                  // up time of the system from firstboot
uint64_t g_bootmoment;                              // moment of boot from epoch
#define UPTIME_LIMIT 2 * 24 * 60 * 60 * 1000 * 1000 // 2 days = 2 * 24 hours etc

int stat_condwait_data_ack, stat_condwait_acs, stat_condwait_xband, stat_condwait_uhf;

pthread_cond_t cond_data_ack, cond_acs, cond_xband, cond_uhf;
pthread_mutex_t mutex_data_ack, mutex_acs, mutex_xband, mutex_uhf;

extern int8_t mag_index, bdot_index, omega_index, sol_index, L_err_index;
extern uint8_t omega_ready;

DECLARE_BUFFER(g_B, extern float);

DECLARE_BUFFER(g_W, extern float);

DECLARE_BUFFER(g_Bt, extern float);

DECLARE_BUFFER(g_S, extern float);

extern volatile uint8_t g_nightmode; // determines if the program is at night

extern const float MOI[3][3];

// target angular speed
DECLARE_VECTOR2(g_W_target, float);

// target angular momentum
DECLARE_VECTOR2(g_L_target, float);

// angular momentum pointing error, angle between Z axis and angular momentum vector
extern float g_L_pointing[SH_BUFFER_SIZE];
#define POINTING_THRESHOLD 0.997 // cos(4 degrees)
// angular momentum magnitude error, which is the Z-angular momentum error
extern float g_L_mag[SH_BUFFER_SIZE];

typedef void (*func_list)(void *id);

#include <eps_telem.h>

#endif // __MAIN_H