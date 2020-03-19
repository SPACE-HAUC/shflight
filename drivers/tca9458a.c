/**
 * @file tca9458a.c
 * @author Sunip K. Mukherjee (sunipkmukherjee@gmail.com)
 * @brief Function definitions for TCA9458A I2C driver
 * @version 0.1
 * @date 2020-03-19
 * 
 * @copyright Copyright (c) 2020
 * 
 */
#include "tca9458a.h"
/**
 * @brief Initialize a Mux device, returns 1 on success
 *  TODO: Implement a scan function at init where it checks all 3 CSS are
 * present on 3 buses?
 * 
 * @param dev 
 * @param addr TCA9458A device address (default: 0x70)
 * @return 1 on success, -1 on error
 */
int tca9458a_init(tca9458a *dev, uint8_t addr)
{
    int status = 1;
    if (dev == NULL)
    {
        perror("[TCA9458A] Device memory not allocated");
    }
    // open bus
    dev->fd = open(dev->fname, O_RDWR);
    if (dev->fd < 0)
    {
        status &= 0;
        perror("[TCA9458A] Error opening FD");
        return status;
    }
    // execute ioctl to associate FD with address
    if ((status = ioctl(dev->fd, I2C_SLAVE, addr)) < 0)
    {
        perror("[TCA9458A] I2C ioctl failed");
        return -1;
    }
    else
        status = 1;
    // disable all output
    status &= tca9458a_set(dev, 8);
    return status;
}
/**
 * @brief Disable all outputs, close file descriptor for the I2C Bus.
 * 
 * @param dev 
 */
void tca9458a_destroy(tca9458a *dev)
{
    if (dev == NULL)
        return;
    // disable mux
    tca9458a_set(dev, 8);
    // close file descriptor
    close(dev->fd);
    // free allocated memory
    free(dev);
    return;
}