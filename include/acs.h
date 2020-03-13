#ifndef ACS_H
#define ACS_H

#include <main.h>
#include <shflight_consts.h>
#include <shflight_externs.h>

// initialize devices and variables related to ACS
int acs_init(void);

// Attitude Control System Thread, accepts (int) thread ID as input (may change in the future)
void *acs_thread(void *);

// destroy devices and variables related to ACS
void acs_destroy(void);

// 3 element array sort, first array is guide that sorts both arrays
void insertionSort(int a1[], int a2[]);

// Fire magnetorquers in the directions dictated by input vector
#define HBRIDGE_ENABLE(name) \
    hbridge_enable(x_##name, y_##name, z_##name);

// fire hbridge direction
int hbridge_enable(int x, int y, int z);

// disable hbridge axis
int HBRIDGE_DISABLE(int num);

// get omega using magnetic field measurements (Bdot)
void getOmega(void);

// get sun vector using lux measurements
void getSVec(void);

// read all sensors
int readSensors(void);

// check if the program should transition from one state to another
void checkTransition(void);

#endif // ACS_H