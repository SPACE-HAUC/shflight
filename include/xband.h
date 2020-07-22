/**
 * @file xband.h
 * @author Sunip K. Mukherjee (sunipkmukherjee@gmail.com)
 * @brief SPACE-HAUC X-Band Transceiver function prototypes (Needs to be written)
 * @version 0.1
 * @date 2020-03-19
 * 
 * @copyright Copyright (c) 2020
 * 
 */
#ifndef XBAND_H
#define XBAND_H

#include <math.h>
#include <adar1000.h>

#define PATCH_FREQ 7.2e9 // 7.2 GHz
#define SPEED_LIGHT 2.9979e11 // speed of light in mm
#define PHASE_STEP 2.8125 * M_PI // deg
#define PATCH_DIST 19.34 // in mm
#define ABS_MAX_RSSI -20 // let's say anything over -20 dB is impossible with our GS
#define LAM (SPEED_LIGHT/PATCH_FREQ)
#define SPIRAL_MAX_LOOP 128 // looping 128 times max in spiral search before sleep

int beam_spiral_search(adar1000 devs[], float phase_out[], float *rssi_last);
float get_rssi();

/**
 * @brief X-band thread
 * 
 * @param id Pointer to integer containing thread ID
 * @return NULL
 */
void *xband(void *id);
#endif