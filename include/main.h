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

#define SH_BUFFER_SIZE 64 // size of circular buffers

#define DETUMBLE_TIME_STEP 100000  // 100 msec, in usec
#define COARSE_TIME_STEP   1000000 // 1000 msec, in usec
#define FINE_TIME_STEP     200000  // 200 msec, in usec
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

pthread_cond_t cond_data_ack, cond_acs, cond_xband, cond_uhf ;
pthread_mutex_t mutex_data_ack, mutex_acs, mutex_xband, mutex_uhf ;

int8_t mag_index = -1 , bdot_index = -1 , omega_index = -1, sol_index = -1 ;
uint8_t omega_ready = 0 ;

extern int16_t x_gB[SH_BUFFER_SIZE], y_gB[SH_BUFFER_SIZE], z_gB[SH_BUFFER_SIZE] ; // Raw magnetic field measurements

extern float x_gW[SH_BUFFER_SIZE], y_gW[SH_BUFFER_SIZE], z_gW[SH_BUFFER_SIZE] ; // Omega measurements from B

extern float g_Sx[SH_BUFFER_SIZE], g_Sy[SH_BUFFER_SIZE], g_Sz[SH_BUFFER_SIZE] ; // sun vector measurements from B

volatile uint8_t g_nightmode = 0 ; // determines if the program is at night

#endif // __MAIN_H