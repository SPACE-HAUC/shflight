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

#include <main_helper.h>

#define SH_BUFFER_SIZE 64 // size of circular buffers

#define DETUMBLE_TIME_STEP 100000  // 100 msec, in usec
#define COARSE_TIME_STEP   1000000 // 1000 msec, in usec
#define FINE_TIME_STEP     200000  // 200 msec, in usec
#define MAG_MEASURE_TIME   20000   // 20 msec, in usec
#define SUNPOINT_DUTY_CYCLE 20000  // 20 msec, in usec
#define COARSE_MEASURE_TIME 100000 // 100 msec, in usec
#define FINE_MEASURE_TIME 5000     // 5 ms, in usec (single shot mode)
typedef enum {SH_SYS_INIT,
              SH_STATE_CHANGE,
              SH_ACS_DETUMBLE,
              SH_ACS_COARSE,
              SH_ACS_FINE,
              SH_ACS_NIGHT,
              SH_SPIN_CTRL,
              SH_SYS_READY,
              SH_SYS_NIGHT,
              SH_UHF_READY,
              SH_XBAND} SH_SYS_STATES ;

volatile uint8_t g_boot_count ;
volatile uint8_t g_program_state ;
volatile uint8_t g_previous_state ;
volatile uint8_t g_bootup ;

int stat_condwait_data_ack , stat_condwait_acs, stat_condwait_xband, stat_condwait_uhf ;

pthread_cond_t cond_data_ack, cond_acs, cond_xband, cond_uhf ;
pthread_mutex_t mutex_data_ack, mutex_acs, mutex_xband, mutex_uhf ;

int8_t mag_index = -1 , bdot_index = -1 , omega_index = -1, sol_index = -1, L_err_index = -1 ;
uint8_t omega_ready = 0 ;

DECLARE_BUFFER(g_B,extern float);

DECLARE_BUFFER(g_W,extern float);

DECLARE_BUFFER(g_Bt, extern float);

DECLARE_BUFFER(g_S, extern float);

volatile uint8_t g_nightmode = 0 ; // determines if the program is at night

const float MOI[3][3] = {{1, 0, 0},{0, 1, 0},{0, 0, 1}};

// target angular speed
DECLARE_VECTOR(g_W_target,float);

// target angular momentum
DECLARE_VECTOR(g_L_target,float);

// angular momentum pointing error, angle between Z axis and angular momentum vector
float g_L_pointing[SH_BUFFER_SIZE] ;
#define POINTING_THRESHOLD 0.997 // cos(4 degrees)
// angular momentum magnitude error, which is the Z-angular momentum error
float g_L_mag[SH_BUFFER_SIZE] ;

#endif // __MAIN_H