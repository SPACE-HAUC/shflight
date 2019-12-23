#ifndef __MAIN_H
#define __MAIN_H
#include <stdio.h>
#include <stdint.h>

typedef enum {SH_SYS_INIT, SH_ACS_DETUMBLE, SH_ACS_COARSE, SH_ACS_FINE, SH_ACS_NIGHT, SH_SYS_READY, SH_SYS_NIGHT, SH_UHF_READY, SH_XBAND} SH_SYS_STATES ;

volatile SH_SYS_STATES g_program_state ;
#endif // __MAIN_H