/**
 * @file tca9458a.h
 * @author Sunip K. Mukherjee (sunipkmukherjee@gmail.com)
 * @brief Function prototypes and struct declarations for TCA9458A I2C driver
 * @version 0.1
 * @date 2020-03-19
 * 
 * @copyright Copyright (c) 2020
 * 
 */
#ifndef TCA9458A_H
#define TCA9458A_H
#include <stdint.h>

#define MUX_I2C_FIle "/dev/i2c-1" ///< I2C Device for Mux
/**
 * @brief TCA9458A Device handle.
 * 
 */
typedef struct
{
    int fd;          ///< File descriptor for I2C Bus
    char fname[40];  ///< File name for I2C Bus
    uint8_t channel; ///< Current active channel
} tca9458a;

int tca9458a_init(tca9458a *, uint8_t);
/**
 * @brief Update active I2C channel (Inlined global symbol)
 * 
 * @param dev 
 * @param channel_id Channel to enable
 * @return Returns 1 on success, 0 or -1 on error (see write())
 */
inline int tca9458a_set(tca9458a *dev, uint8_t channel_id)
{
    dev->channel = channel_id < 8 ? 0x01 << channel_id : 0x00;
    return write(dev->fd, &(dev->channel), 1);
}
void tca9458a_destroy(tca9458a *);
#endif