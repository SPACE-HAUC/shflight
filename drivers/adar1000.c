/**
 * @file adar1000.c
 * @author Sunip K. Mukherjee (sunipkmukherjee@gmail.com)
 * @brief Function definitions for ADAR1000 SPI Driver (Linux)
 * ADAR1000 SPI operation mode is 0.
 * @version 0.1
 * @date 2020-07-20
 * 
 * @copyright Copyright (c) 2020
 */
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <linux/types.h>
#include <linux/spi/spidev.h>
#include <signal.h>
#include <math.h>
#include <adar1000.h>

int _adar1000_init(adar1000 *dev)
{
    int file;
    __u8 mode, lsb, bits;
    __u32 speed = 2500000;

    if ((file = open(SPI_BUS, O_RDWR)) < 0)
    {
        /* ERROR HANDLING; you can check errno to see what went wrong */
        perror("ADAR1000: Opening bus");
    }
    mode = SPI_MODE_0;
    if (ioctl(file, SPI_IOC_WR_MODE, &mode) < 0)
    {
        perror("ADAR1000: can't set spi mode");
        return 0;
    }

    if (ioctl(file, SPI_IOC_RD_MODE, &mode) < 0)
    {
        perror("ADAR1000: SPI rd_mode");
        return 0;
    }
    lsb = 0;
    if (ioctl(file, SPI_IOC_WR_LSB_FIRST, &lsb) < 0)
    {
        perror("ADAR1000: SPI rd_lsb_fist");
        return 0;
    }
    if (ioctl(file, SPI_IOC_RD_LSB_FIRST, &lsb) < 0)
    {
        perror("ADAR1000: SPI rd_lsb_fist");
        return 0;
    }
    bits = 0;

    if (ioctl(file, SPI_IOC_WR_BITS_PER_WORD, &bits) < 0)
    {
        perror("ADAR1000: can't set bits per word");
        return 0;
    }

    if (ioctl(file, SPI_IOC_RD_BITS_PER_WORD, &bits) < 0)
    {
        perror("ADAR1000: SPI bits_per_word");
        return 0;
    }
    speed = 2000000;
    if (ioctl(file, SPI_IOC_WR_MAX_SPEED_HZ, &speed) < 0)
    {
        perror("ADAR1000: can't set max speed hz");
        return 0;
    }

    if (ioctl(file, SPI_IOC_RD_MAX_SPEED_HZ, &speed) < 0)
    {
        perror("ADAR1000: SPI max_speed_hz");
        return 0;
    }

    dev->mode = mode;
    dev->bits = bits;
    dev->lsb = lsb;
    dev->file = file;

    dev->xfer[0].cs_change = 0;          /* Keep CS activated */
    dev->xfer[0].delay_usecs = 0;        //delay in us
    dev->xfer[0].speed_hz = 1000 * 1000; //speed
    dev->xfer[0].bits_per_word = 8;      // bites per word 8
    return 1;
}

static inline void invert_arr(unsigned char dest[], unsigned char src[])
{
    dest[0] = src[2];
    dest[1] = src[1];
    dest[2] = src[0];
}

int adar1000_init(adar1000 *dev, unsigned char addr)
{
    if (!_adar1000_init(dev))
        return 0;
    if (addr > 4)
    {
        printf("ADAR1000: Address out of range\n");
        return 0;
    }

    adar_register reg;
    unsigned char buf[3];
    // reset
    reg.reg = INTERFACE_CONFIG_A;
    reg.data = 0xbd;
    reg.addr = addr;
    invert_arr(buf, reg.bytes);
    if (adar1000_xfer(dev, buf, 3) < 0)
        return -1;

    // Allow LDO adjustments from user settings
    reg.reg = LDO_TRIM_CTL_1;
    reg.data = 0x10;
    invert_arr(buf, reg.bytes);
    if (adar1000_xfer(dev, buf, 3) < 0)
        return -1;

    // Adjust LDOs
    reg.reg = LDO_TRIM_CTL_0;
    reg.data = 0x55;
    invert_arr(buf, reg.bytes);
    if (adar1000_xfer(dev, buf, 3) < 0)
        return -1;

    // TODO Set PA_BIAS1 to ~ -1.8V (subject to change)
    reg.reg = CH1_PA_BIAS_OFF;
    reg.data = 0x60;
    invert_arr(buf, reg.bytes);
    if (adar1000_xfer(dev, buf, 3) < 0)
        return -1;

    // TODO Set PA_BIAS2 to ~ -1.8V (subject to change)
    reg.reg = CH2_PA_BIAS_OFF;
    reg.data = 0x60;
    invert_arr(buf, reg.bytes);
    if (adar1000_xfer(dev, buf, 3) < 0)
        return -1;

    // TODO Set PA_BIAS3 to ~ -1.8V (subject to change)
    reg.reg = CH3_PA_BIAS_OFF;
    reg.data = 0x60;
    invert_arr(buf, reg.bytes);
    if (adar1000_xfer(dev, buf, 3) < 0)
        return -1;

    // TODO Set PA_BIAS4 to ~ -1.8V (subject to change)
    reg.reg = CH4_PA_BIAS_OFF;
    reg.data = 0x60;
    invert_arr(buf, reg.bytes);
    if (adar1000_xfer(dev, buf, 3) < 0)
        return -1;

    // TODO Set PA_BIAS1 to ~ -0.8V (subject to change)
    reg.reg = CH1_PA_BIAS_ON;
    reg.data = 0x28;
    invert_arr(buf, reg.bytes);
    if (adar1000_xfer(dev, buf, 3) < 0)
        return -1;

    // TODO Set PA_BIAS2 to ~ -0.8V (subject to change)
    reg.reg = CH2_PA_BIAS_ON;
    reg.data = 0x28;
    invert_arr(buf, reg.bytes);
    if (adar1000_xfer(dev, buf, 3) < 0)
        return -1;

    // TODO Set PA_BIAS3 to ~ -0.8V (subject to change)
    reg.reg = CH3_PA_BIAS_ON;
    reg.data = 0x28;
    invert_arr(buf, reg.bytes);
    if (adar1000_xfer(dev, buf, 3) < 0)
        return -1;

    // TODO Set PA_BIAS4 to ~ -0.8V (subject to change)
    reg.reg = CH4_PA_BIAS_ON;
    reg.data = 0x28;
    invert_arr(buf, reg.bytes);
    if (adar1000_xfer(dev, buf, 3) < 0)
        return -1;

    // TODO Set LNA bias to approx -0.8V (subject to change)
    reg.reg = LNA_BIAS_ON;
    reg.data = 0x28;
    invert_arr(buf, reg.bytes);
    if (adar1000_xfer(dev, buf, 3) < 0)
        return -1;

    // Enable LNA_BIAS select fixed output
    reg.reg = MISC_ENABLES;
    reg.data = 0x1f;
    invert_arr(buf, reg.bytes);
    if (adar1000_xfer(dev, buf, 3) < 0)
        return -1;

    return 1;
}

void adar1000_destroy(adar1000 *dev)
{
    adar_register reg;
    unsigned char buf[3];
    // reset
    reg.reg = INTERFACE_CONFIG_A;
    reg.data = 0xbd;
    reg.addr = dev->addr;
    invert_arr(buf, reg.bytes);
    while (adar1000_xfer(dev, buf, 3) < 0)
        ;
    close(dev->file);
    return;
}

int adar1000_enable_trx(adar1000 *dev, unsigned char trx)
{
    adar_register reg;
    reg.addr = dev->addr;
    unsigned char buf[3];
    // Enable SPI instead of internal RAM
    reg.reg = MEM_CTRL;
    reg.data = 0x60;
    invert_arr(buf, reg.bytes);
    if (adar1000_xfer(dev, buf, 3) < 0)
        return -1;

    // Select TR input for transmit/receive
    reg.reg = SW_CTRL;
    reg.data = 0x1c;
    invert_arr(buf, reg.bytes);
    if (adar1000_xfer(dev, buf, 3) < 0)
        return -1;

    switch (trx)
    {
    case 1: // transmit
        reg.reg = TX_ENABLES;
        reg.data = 0x7f;
        invert_arr(buf, reg.bytes);
        if (adar1000_xfer(dev, buf, 3) < 0)
            return -1;
        reg.reg = BIAS_CURRENT_TX;
        reg.data = 0x16;
        invert_arr(buf, reg.bytes);
        if (adar1000_xfer(dev, buf, 3) < 0)
            return -1;
        reg.reg = BIAS_CURRENT_TX_DRV;
        reg.data = 0x06;
        invert_arr(buf, reg.bytes);
        if (adar1000_xfer(dev, buf, 3) < 0)
            return -1;
        break;
    case 2: // receive
        reg.reg = RX_ENABLES;
        reg.data = 0x7f;
        invert_arr(buf, reg.bytes);
        if (adar1000_xfer(dev, buf, 3) < 0)
            return -1;
        reg.reg = BIAS_CURRENT_RX_LNA;
        reg.data = 0x08;
        invert_arr(buf, reg.bytes);
        if (adar1000_xfer(dev, buf, 3) < 0)
            return -1;
        reg.reg = BIAS_CURRENT_RX;
        reg.data = 0x16;
        invert_arr(buf, reg.bytes);
        if (adar1000_xfer(dev, buf, 3) < 0)
            return -1;
        break;
    default:
        printf("ADAR1000: 1 for TX and 2 for RX\n");
        break;
    }
    return 1;
}

int adar1000_set_tx_beam(adar1000 *dev, unsigned char chn, float phase)
{
    if (chn > 3)
    {
        printf("ADAR1000: Channel limit exceeded. Error.\n");
        return -chn;
    }
    int indx = round(phase / 2.8125);
    indx = indx < 0 ? 0 : (indx > 127 ? 127 : indx);
    unsigned char ph_i = adar_phase_to_i[indx];
    unsigned char ph_q = adar_phase_to_q[indx];

    adar_register reg;
    unsigned char buf[3];
    reg.addr = dev->addr;
    switch (chn)
    {
    case 0:
        reg.reg = CH1_TX_GAIN;
        reg.data = 0x16;
        invert_arr(buf, reg.bytes);
        if (adar1000_xfer(dev, buf, 3) < 0)
            return -1;
        reg.reg = CH1_TX_PHASE_I;
        reg.data = ph_i;
        invert_arr(buf, reg.bytes);
        if (adar1000_xfer(dev, buf, 3) < 0)
            return -1;
        reg.reg = CH1_TX_PHASE_Q;
        reg.data = ph_q;
        invert_arr(buf, reg.bytes);
        if (adar1000_xfer(dev, buf, 3) < 0)
            return -1;
        break;
    case 1:
        reg.reg = CH2_TX_GAIN;
        reg.data = 0x16;
        invert_arr(buf, reg.bytes);
        if (adar1000_xfer(dev, buf, 3) < 0)
            return -1;
        reg.reg = CH2_TX_PHASE_I;
        reg.data = ph_i;
        invert_arr(buf, reg.bytes);
        if (adar1000_xfer(dev, buf, 3) < 0)
            return -1;
        reg.reg = CH2_TX_PHASE_Q;
        reg.data = ph_q;
        invert_arr(buf, reg.bytes);
        if (adar1000_xfer(dev, buf, 3) < 0)
            return -1;
        break;
    case 2:
        reg.reg = CH3_TX_GAIN;
        reg.data = 0x16;
        invert_arr(buf, reg.bytes);
        if (adar1000_xfer(dev, buf, 3) < 0)
            return -1;
        reg.reg = CH3_TX_PHASE_I;
        reg.data = ph_i;
        invert_arr(buf, reg.bytes);
        if (adar1000_xfer(dev, buf, 3) < 0)
            return -1;
        reg.reg = CH3_TX_PHASE_Q;
        reg.data = ph_q;
        invert_arr(buf, reg.bytes);
        if (adar1000_xfer(dev, buf, 3) < 0)
            return -1;
        break;
    case 3:
        reg.reg = CH4_TX_GAIN;
        reg.data = 0x16;
        invert_arr(buf, reg.bytes);
        if (adar1000_xfer(dev, buf, 3) < 0)
            return -1;
        reg.reg = CH4_TX_PHASE_I;
        reg.data = ph_i;
        invert_arr(buf, reg.bytes);
        if (adar1000_xfer(dev, buf, 3) < 0)
            return -1;
        reg.reg = CH4_TX_PHASE_Q;
        reg.data = ph_q;
        invert_arr(buf, reg.bytes);
        if (adar1000_xfer(dev, buf, 3) < 0)
            return -1;
        break;
    }
    return 1;
}

int adar1000_set_rx_beam(adar1000 *dev, unsigned char chn, float phase)
{
    if (chn > 3)
    {
        printf("ADAR1000: Channel limit exceeded. Error.\n");
        return -chn;
    }
    int indx = round(phase / 2.8125);
    indx = indx < 0 ? 0 : (indx > 127 ? 127 : indx);
    unsigned char ph_i = adar_phase_to_i[indx];
    unsigned char ph_q = adar_phase_to_q[indx];

    adar_register reg;
    unsigned char buf[3];
    reg.addr = dev->addr;
    switch (chn)
    {
    case 0:
        reg.reg = CH1_RX_GAIN;
        reg.data = 0x16;
        invert_arr(buf, reg.bytes);
        if (adar1000_xfer(dev, buf, 3) < 0)
            return -1;
        reg.reg = CH1_RX_PHASE_I;
        reg.data = ph_i;
        invert_arr(buf, reg.bytes);
        if (adar1000_xfer(dev, buf, 3) < 0)
            return -1;
        reg.reg = CH1_RX_PHASE_Q;
        reg.data = ph_q;
        invert_arr(buf, reg.bytes);
        if (adar1000_xfer(dev, buf, 3) < 0)
            return -1;
        break;
    case 1:
        reg.reg = CH2_RX_GAIN;
        reg.data = 0x16;
        invert_arr(buf, reg.bytes);
        if (adar1000_xfer(dev, buf, 3) < 0)
            return -1;
        reg.reg = CH2_RX_PHASE_I;
        reg.data = ph_i;
        invert_arr(buf, reg.bytes);
        if (adar1000_xfer(dev, buf, 3) < 0)
            return -1;
        reg.reg = CH2_RX_PHASE_Q;
        reg.data = ph_q;
        invert_arr(buf, reg.bytes);
        if (adar1000_xfer(dev, buf, 3) < 0)
            return -1;
        break;
    case 2:
        reg.reg = CH3_RX_GAIN;
        reg.data = 0x16;
        invert_arr(buf, reg.bytes);
        if (adar1000_xfer(dev, buf, 3) < 0)
            return -1;
        reg.reg = CH3_RX_PHASE_I;
        reg.data = ph_i;
        invert_arr(buf, reg.bytes);
        if (adar1000_xfer(dev, buf, 3) < 0)
            return -1;
        reg.reg = CH3_RX_PHASE_Q;
        reg.data = ph_q;
        invert_arr(buf, reg.bytes);
        if (adar1000_xfer(dev, buf, 3) < 0)
            return -1;
        break;
    case 3:
        reg.reg = CH4_RX_GAIN;
        reg.data = 0x16;
        invert_arr(buf, reg.bytes);
        if (adar1000_xfer(dev, buf, 3) < 0)
            return -1;
        reg.reg = CH4_RX_PHASE_I;
        reg.data = ph_i;
        invert_arr(buf, reg.bytes);
        if (adar1000_xfer(dev, buf, 3) < 0)
            return -1;
        reg.reg = CH4_RX_PHASE_Q;
        reg.data = ph_q;
        invert_arr(buf, reg.bytes);
        if (adar1000_xfer(dev, buf, 3) < 0)
            return -1;
        break;
    }
    return 1;
}

int adar1000_xfer(adar1000 *dev, void *data, ssize_t len)
{
    int status = 0;
    dev->xfer[0].tx_buf = (unsigned long)data;
    dev->xfer[0].len = 3;
    status = ioctl(dev->file, SPI_IOC_MESSAGE(1), dev->xfer);
    if (status < 0)
    {
        perror("ADAR1000: SPI_IOC_MESSAGE");
        return -1;
    }
    usleep(100); // sleep 100 us
    return status;
}

#ifdef UNIT_TEST
int main()
{
    return 0;
}
#endif // UNIT_TEST