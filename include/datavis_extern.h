/**
 * @file datavis_extern.h
 * @author Sunip K. Mukherjee (sunipkmukherjee@gmail.com)
 * @brief DataVis thread externs for other modules
 * @date 2020-03-19
 * 
 * @copyright Copyright (c) 2020
 * 
 */
#ifndef DATAVIS_EXTERN_H
#define DATAVIS_EXTERN_H
#include <datavis.h>
#include <pthread.h>
#ifndef DATAVIS_IFACE_H
extern data_packet g_datavis_st;
#endif // DATAVIS_IFACE_H
extern pthread_cond_t datavis_drdy;
extern pthread_mutex_t datavis_mutex;
#endif // DATAVIS_EXTERN_H