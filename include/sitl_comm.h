/**
 * @file sitl_comm.h
 * @author Sunip K. Mukherjee (sunipkmukherjee@gmail.com)
 * @brief Software-In-The-Loop (SITL) serial communication headers and function prototypes
 * @version 0.1
 * @date 2020-03-19
 * 
 * @copyright Copyright (c) 2020
 * 
 */
#ifndef __SITL_COMM_H
#define __SITL_COMM_H
/**
 * @brief Set speed and parity attributes for the serial device
 * 
 * @param fd Serial device file descriptor
 * @param speed Baud rate, is a constant of the form B#### defined in termios.h
 * @param parity Odd or even parity for the serial device (1, 0)
 * @return 0 on success, -1 on error
 */
int set_interface_attribs(int fd, int speed, int parity);

/**
 * @brief Set the serial device as blocking or non-blocking
 * 
 * @param fd Serial device file descriptor
 * @param should_block 0 for non-blocking, 1 for blocking mode operation
 */
void set_blocking(int fd, int should_block);

/**
 * @brief Set the up serial device
 * Opens the serial device /dev/ttyS0 (for RPi only)
 * 
 * @return file descriptor to the serial device
 */
int setup_serial(void);

/**
 * @brief Serial communication thread.
 * 
 * 
 * Communicates with the environment simulator over serial port.
 * The serial communication happens at 230400 bps, and this thread
 * is intended to loop at 200 Hz. The thread reads the packet over
 * serial (packet format: [0xa0 x 10] [uint8 x 28] [0xb0 x 2]).
 * The thread synchronizes to the 0xa0 in the beginning and checks
 * for the 0xb0 at the end at each iteration. The data is read into
 * global variables, and the magnetorquer
 * command is read out. All read-writes are atomic.
 * 
 * @param id Pointer to an int that specifies thread ID
 * @return NULL
 */
void *sitl_comm(void *id);
#endif