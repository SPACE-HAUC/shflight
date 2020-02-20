#include "lsm9ds1.h"
int lsm9ds1_init(lsm9ds1 *dev, uint8_t xl_addr, uint8_t mag_addr)
{
    uint8_t accel_stat = 1;
    dev->accel_file = open(dev->fname, O_RDWR);
    if (dev->mag_file < 0)
    {
        accel_stat = 0;
        perror("LSM9DS1: Could not open fd for mag");
    }
    dev->mag_file = open(dev->fname, O_RDWR);
    if (dev->mag_file < 0)
    {
        perror("LSM9DS1: Could not open fd for mag");
        return -1;
    }
    if (ioctl(dev->mag_file, I2C_SLAVE, mag_addr) < 0)
    {
        perror("LSM9DS1: MAG ioctl failed");
        return -1;
    }
    // Verify mag identity
    uint8_t buf = MAG_WHO_AM_I;
    if (write(dev->mag_file, &buf, 1) < 1)
    {
        perror("LSM9DS1_INIT: Could not write to magnetic field descriptor.");
        return -1;
    }
    if (read(dev->mag_file, &buf, 1) < 1)
    {
        perror("LSM9DS1_INIT: Could not write to magnetic field descriptor.");
        return -1;
    }
    if (buf != MAG_IDENT)
    {
        perror("LSM9DS1_INIT: Identity did not match");
        return -1;
    }
    // disable accel+gyro
    char obuf[2];
    obuf[0] = LSM9DS1_CTRL_REG1_G;
    obuf[1] = 0x00;
    write(dev->accel_file, &obuf, 2);
    obuf[0] = LSM9DS1_CTRL_REG5_XL;
    write(dev->accel_file, &obuf, 2);
    obuf[0] = LSM9DS1_CTRL_REG6_XL;
    write(dev->accel_file, &obuf, 2);

    // also configure magnetometer for SPACE HAUC use I2C_SLAVE
    MAG_DATA_RATE drate;
    drate.data_rate = 0b101;
    drate.fast_odr = 0;
    drate.operative_mode = 0b11;
    drate.temp_comp = 1;
    MAG_RESET rst;
    rst.full_scale = 0b00;
    rst.reboot = 0;
    rst.soft_rst = 0;
    MAG_DATA_READ dread;
    dread.bdu = 0;
    dread.fast_read = 0;
    int mag_stat = lsm9ds1_config_mag(dev, drate, rst, dread);

    return 1 & mag_stat;
}

int lsm9ds1_config_mag(lsm9ds1 *dev, MAG_DATA_RATE datarate, MAG_RESET rst, MAG_DATA_READ dread)
{
    int stat = 1;
    uint8_t buf[2];
    buf[0] = MAG_CTRL_REG1_M;
    buf[1] = *((char *)&datarate);
    if (write(dev->mag_file, &buf, 2) < 2)
    {
        perror("Data rate config failed.");
        stat = 0;
    }
    buf[0] = MAG_CTRL_REG2_M;
    buf[1] = *((char *)&rst);
    if (write(dev->mag_file, &buf, 2) < 2)
    {
        perror("Reset config failed.");
        stat = 0;
    }
    buf[0] = MAG_CTRL_REG3_M;
    buf[1] = 0x00;
    if (write(dev->mag_file, &buf, 2) < 2)
    {
        perror("Reg3 config failed.");
        stat = 0;
    }
    buf[0] = MAG_CTRL_REG4_M;
    buf[1] = MAG_CTRL_REG4_DATA;
    if (write(dev->mag_file, &buf, 2) < 2)
    {
        perror("Reg4 config failed.");
        stat = 0;
    }
    buf[0] = MAG_CTRL_REG5_M;
    buf[1] = *((char *)&dread);
    if (write(dev->mag_file, &buf, 2) < 2)
    {
        perror("Data read config failed.");
        stat = 0;
    }
    return stat;
}
int lsm9ds1_reset_mag(lsm9ds1 *dev)
{
    uint8_t buf[2];
    buf[0] = MAG_CTRL_REG2_M;
    buf[1] = 0x00;
    MAG_RESET rst = *((MAG_RESET *)&buf[1]);
    rst.reboot = 1;
    buf[1] = *((uint8_t *)&rst);
    if (write(dev->mag_file, &buf, 2) < 2)
    {
        perror("Reset failed.");
        return -1;
    }
    return 1;
}
int lsm9ds1_read_mag(lsm9ds1 *dev, short *B)
{
    uint8_t buf, reg = MAG_OUT_X_L - 1;
    for (int i = 0; i < 3; i++)
    {
        B[i] = 0x00; // initialize with 0
        buf = ++reg; // insert the command into buffer
        if (write(dev->mag_file, &buf, 1) < 1)
        {
            perror("read_mag failed");
            return -1;
        }
        if (read(dev->mag_file, &buf, 1) < 1)
        {
            perror("read_mag failed");
            return -1;
        }
        B[i] |= buf;
        buf = ++reg; // select the next register
        if (write(dev->mag_file, &buf, 1) < 1)
        {
            perror("read_mag failed");
            return -1;
        }
        if (read(dev->mag_file, &buf, 1) < 1)
        {
            perror("read_mag failed");
            return -1;
        }
        B[i] |= 0xff00 & ((short)buf << 8);
    }
    return 1;
}

int lsm9ds1_offset_mag(lsm9ds1 *dev, short *offset)
{
    uint8_t buf[2], reg = MAG_OFFSET_X_REG_L_M - 1;
    for (int i = 0; i < 3; i++)
    {
        buf[0] = ++reg; // insert the command into buffer
        buf[1] = (uint8_t)offset[i];
        if (write(dev->mag_file, &buf, 2) < 2)
        {
            perror("offset_mag failed");
            return -1;
        }
        buf[0] = ++reg; // insert the next reg address
        buf[1] = (uint8_t)(offset[i] >> 8);
        if (write(dev->mag_file, &buf, 2) < 2)
        {
            perror("offset_mag failed");
            return -1;
        }
    }
    return 1;
}

void lsm9ds1_destroy(lsm9ds1 *dev)
{
    close(dev->accel_file);
    close(dev->mag_file);
    free(dev);
}