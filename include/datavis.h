/**
 * @file datavis.h
 * @author Sunip K. Mukherjee (sunipkmukherjee@gmail.com)
 * @brief DataVis thread to visualize ACS data over TCP (uses client.py)
 * @version 0.1
 * @date 2020-03-19
 * 
 * @copyright Copyright (c) 2020
 * 
 */
#ifndef __DATAVIS_H
#define __DATAVIS_H

#ifndef PORT
/**
 * @brief TCP port on which DataVis transmission can be accessed.
 */ 
#define PORT 12376
#endif
#include <stdint.h>
#include <macros.h>
/**
 * @brief Internal data structure of a DataVis packet. 
 */
typedef struct
{
    char start[6];
    uint16_t eps_vbatt;
    uint16_t eps_mvboost;
    uint16_t eps_cursun;
    uint16_t eps_cursys;
    uint8_t eps_battmode;
    /**
     * @brief Current system state
     * 
     */
    uint8_t mode; // ACS Mode
    /**
     * @brief Current ACS step number
     * 
     */
    uint64_t step; // ACS Step
    /**
     * @brief Current time from UTC (usec)
     * 
     */
    uint64_t tnow; // Time now
    /**
     * @brief ACS start time from UTC (usec)
     * 
     */
    uint64_t tstart; // Time start
    /**
     * @brief Measured magnetic field
     * 
     */
    DECLARE_VECTOR2(B, float); // magnetic field
    /**
     * @brief Calculated value of \f$\vec{\dot{B}}\f$
     * 
     */
    DECLARE_VECTOR2(Bt, float); // B dot
    /**
     * @brief Calculated value of \f$\vec{\omega}\f$
     * 
     */
    DECLARE_VECTOR2(W, float); // Omega
    /**
     * @brief Calculated value of sun vector
     * 
     */
    DECLARE_VECTOR2(S, float); // Sun vector
    char end[4];
} datavis_p;
/**
 * @brief Size of the datavis_p struct
 * 
 */
#define PACK_SIZE sizeof(datavis_p)
/**
 * @brief Union of the datavis_p structure and an array of bytes for transport over TCP using send().
 * 
 */
typedef union {
    /**
     * @brief Data section of the data_packet where members of datavis_p can be accessed.
     * 
     */
    datavis_p data;
    /**
     * @brief Byte section of the data_packet for transport using send().
     * 
     */
    unsigned char buf[sizeof(datavis_p)];
} data_packet;

/**
 * @brief DataVis thread, sends data in g_datavis_st over TCP.
 * This thread loops over done, and at each wakeup from the ACS
 * thread sends the currently available data over TCP to the
 * listening connection.
 * 
 * @param t Pointer to an integer containing the thread ID.
 * @return NULL.
 */
void *datavis_thread(void *t);

#endif // __DATAVIS_H