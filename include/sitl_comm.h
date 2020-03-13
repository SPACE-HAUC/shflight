#ifndef __SITL_COMM_H
#define __SITL_COMM_H
#include <main.h>
#include <main_helper.h>
#include <shflight_consts.h>
#include <shflight_externs.h>

// set speed, parity of the serial interface
int set_interface_attribs(int fd, int speed, int parity);

// set blocking or non-blocking call on the serial file descriptor
void set_blocking(int fd, int should_block);

/*
 * Initialize serial comm for SITL test.
 * Input: Pass argc and argv from main; inputs are port and baud
 * Output: Serial file descriptor
 */
int setup_serial(void);

/*
 * This thread waits to read a dataframe, reads it over serial, writes the dipole action to serial and interprets the
 * dataframe that was read over serial. The values are atomically assigned.
 */
void *sitl_comm(void *id);
#endif