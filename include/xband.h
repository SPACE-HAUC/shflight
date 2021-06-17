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
#ifdef XBAND_INTERNAL || _XBAND_IFACE_H
int xband_init(void);
/**
 * @brief XBand thread, takes care of thermal and time shutdown
 * 
 * @param in Unused
 * @return void* 
 */
void *xband_thread(void *in);
void xband_destroy(void);
#endif
#ifdef XBAND_INTERNAL || _XBAND_EXTERN_H
/**
 * @brief 
 * 
 * @param LO LO in MHz
 * @param bw BW in MHz
 * @param samp Sampling rate in kHz
 * @param phy_gain PHY gain (negative of the supplied number is applied)
 * @param ftr_name Filter name (string)
 * @param phase 16-length array for phase table
 * @param adar_gain Gain for channels
 * @return int 1 on success, negative on failure
 */
int xband_set_tx(float LO, float bw, uint16_t samp, uint8_t phy_gain, const char *ftr_name, float *phase, uint8_t adar_gain);
/**
 * @brief 
 * 
 * @param LO LO in MHz
 * @param bw BW in MHz
 * @param samp Sampling rate in kHz
 * @param phy_gain PHY gain (negative of the supplied number is applied)
 * @param ftr_name Filter name (string)
 * @param phase 16-length array for phase table
 * @param adar_gain Gain for channels
 * @return int 1 on success, negative on failure
 */
int xband_set_rx(float LO, float bw, uint16_t samp, uint8_t phy_gain, const char *ftr_name, float *phase, uint8_t adar_gain);
/**
 * @brief 
 * 
 * @param data 
 * @param len 
 * @param mtu 
 * @return int 
 */
int xband_do_tx(uint8_t *data, ssize_t len, int mtu);

int xband_do_rx(uint8_t *data, ssize_t len);

int xband_disable(void);
/**
 * @brief Set the maximum length of time the X-Band system will remain on for
 * 
 * @param t Time in minutes, minimum 1 minute, max 20 minutes
 * @return int8_t Time in minutes
 */
int8_t xband_set_max_on(int8_t t);
/**
 * @brief Get maximum length of time the system will be on
 * 
 * @return int8_t Time in minutes
 */
int8_t xband_get_max_on(void);
/**
 * @brief Set shutdown temperature
 * 
 * @param tmp Temperature, min 50, max 95 degrees
 * @return int8_t Temperature in degrees
 */
int8_t xband_set_tmp_shutdown(int8_t tmp);
/**
 * @brief Get shutdown temperature
 * 
 * @return int8_t Get thermal shutdown limit (degrees)
 */
int8_t xband_get_tmp_shutdown(void);
/**
 * @brief Get operating temperature to reset thermal shutdown
 * 
 * @param tmp Temperature, min TMAX - 20, max 75
 * @return int8_t Temperature in degrees
 */
int8_t xband_set_tmp_op(int8_t tmp);
/**
 * @brief Get reset temperature
 * 
 * @return int8_t Reset temperature in degrees
 */
int8_t xband_get_tmp_op(void);
/**
 * @brief Get loop timer for X Band thermal monitor thread
 * 
 * @return uint8_t Loop time in seconds
 */
uint8_t xband_get_loop_time(void);
/**
 * @brief Set loop timer for X Band thermal monitor thread
 * 
 * @param t Time in seconds, max (time on / 5), min 1 
 * @return uint8_t Set loop time
 */
uint8_t xband_set_loop_time(uint8_t t);
#endif
#endif