#include <eps_telem.h>

void * eps_telem(void * id)
{
    return NULL ;
}

int p31u_init(p31u *dev)
{
    int status;
    dev = NULL;
    dev = (p31u *)malloc(sizeof(p31u));
    if (dev == NULL)
    {
        perror("EPS"
               "Could not allocate memory for device, out of memory!");
        return -1;
    }
    snprintf(dev->fname, 39, EPS_I2C_BUS); // store bus name to struct
    dev->file = open(dev->fname, O_RDWR);  // open file descriptor
    if (dev->file < 0)
    {
        perror("EPS"
               "Error opening I2C bus!");
        return dev->file;
    }
    dev->addr = EPS_I2C_ADDR;

    if ((status = ioctl(dev->file, I2C_SLAVE, dev->addr)) < 0)
    {
        perror("EPS"
               "Error setting IOCTL for I2C_SLAVE at address");
        return status;
    }
    return 1;
}

void p31u_destroy(p31u *dev)
{
    close(dev->file);
    free(dev);
}

int eps_ping(p31u *dev)
{
    int status;
    uint8_t val = rand();
    char buf[3];
    buf[0] = PING;
    buf[1] = val;
    if ((status = write(dev->file, buf, 2)) != 2)
    {
        perror("EPS"
               "I2C Write Failed in PING");
        return status;
    }
    usleep(10000); // 10 ms
    if ((status = read(dev->file, buf, 3)) != 3)
    {
        perror("EPS"
               "I2C Read Failed in PING");
        return status;
    }
    if (buf[0] != PING)
    {
        fprintf(stderr, "EPS"
                        "PING command failed with error code %d\n",
                buf[1]);
        return -1;
    }
    if (buf[2] != val)
    {
        fprintf(stderr, "EPS"
                        "PING command returned wrong value: %d != %d\n",
                val, buf[2]);
        return -1;
    }
    return 1;
}

int eps_reboot(p31u *dev)
{
    uint8_t buf[5];
    buf[0] = REBOOT;
    buf[1] = 0x80;
    buf[2] = 0x07;
    buf[3] = 0x80;
    buf[4] = 0x07;

    if ((write(dev->file, buf, 5)) != 5)
    {
        perror("EPS"
               "I2C Write Failed in REBOOT");
        return -1;
    }
    return 1; // never going to return on success
}
int eps_get_hk(p31u *dev, uint8_t mode)
{
    int status;
    uint8_t *inbuf, outbuf[2];
    outbuf[0] = GET_HK;
    outbuf[1] = mode;
    unsigned long readsize;
    switch (mode)
    {
    case 0: // mode 0 returns eps_hk_t
        inbuf = (uint8_t *)malloc(2 + sizeof(eps_hk_t));
        readsize = 2 + sizeof(eps_hk_t);
        break;

    case 1: // mode 1 returns eps_hk_vi_t
        inbuf = (uint8_t *)malloc(2 + sizeof(eps_hk_vi_t));
        readsize = 2 + sizeof(eps_hk_vi_t);
        break;

    case 2: // mode 2 returns eps_hk_out_t
        inbuf = (uint8_t *)malloc(2 + sizeof(eps_hk_out_t));
        readsize = 2 + sizeof(eps_hk_out_t);
        break;

    case 3: // mode 3 returns eps_hk_wdt_t
        inbuf = (uint8_t *)malloc(2 + sizeof(eps_hk_wdt_t));
        readsize = 2 + sizeof(eps_hk_wdt_t);
        break;

    case 4: // mode 4 returns eps_hk_basic_t
        inbuf = (uint8_t *)malloc(2 + sizeof(eps_hk_basic_t));
        readsize = 2 + sizeof(eps_hk_basic_t);
        break;

    default:
        inbuf = (uint8_t *)malloc(2 + sizeof(eps_hk_t));
        readsize = 2 + sizeof(eps_hk_t);
        break;
    }
    if (write(dev->file, outbuf, 2) != 2)
    {
        perror("EPS"
               "GET_HK Command failed!");
        free(inbuf);
        return -1;
    }
    else
    {
        usleep(10000); // wait for 10 ms for answer to be generated
        if (read(dev->file, inbuf, readsize) != readsize)
        {
            perror("EPS"
                   "GET_HK read failed!");
            free(inbuf);
            return -1;
        }
    }
    if (inbuf[0] != GET_HK)
    {
        fprintf(stderr, "EPS: GET_HK command return mismatch with error code %d\n", inbuf[1]);
        free(inbuf);
        return -1;
    }
    if (inbuf[1] != 0)
    {
        free(inbuf);
        fprintf(stderr, "EPS: GET_HK command error code %d\n", inbuf[1]);
        return -1;
    }
    switch (mode)
    {
    case 0:
        memcpy(&(dev->full_hk), &(inbuf[2]), sizeof(eps_hk_t));
        break;
    case 1:
        memcpy(&(dev->battpower_hk), &(inbuf[2]), sizeof(eps_hk_vi_t));
        break;
    case 2:
        memcpy(&(dev->outstats_hk), &(inbuf[2]), sizeof(eps_hk_out_t));
        break;
    case 3:
        memcpy(&(dev->wdtstats_hk), &(inbuf[2]), sizeof(eps_hk_wdt_t));
        break;
    case 4:
        memcpy(&(dev->basicstas_hk), &(inbuf[2]), sizeof(eps_hk_basic_t));
        break;

    default:
        break;
    }
    free(inbuf);
    return 1;
}

int eps_hk(p31u *dev)
{
    int status;
    uint8_t *inbuf, outbuf[1];
    outbuf[0] = GET_HK;
    unsigned long readsize = 2 + sizeof(hkparam_t);
    inbuf = (uint8_t *)malloc(readsize);
    if (write(dev->file, outbuf, 1) != 1)
    {
        perror("EPS"
               "GET_HK Command failed!");
        free(inbuf);
        return -1;
    }
    else
    {
        usleep(10000); // wait for 10 ms for answer to be generated
        if (read(dev->file, inbuf, readsize) != readsize)
        {
            perror("EPS"
                   "GET_HK read failed!");
            free(inbuf);
            return -1;
        }
    }
    if (inbuf[0] != GET_HK)
    {
        fprintf(stderr, "EPS: GET_HK command return mismatch with error code %d\n", inbuf[1]);
        free(inbuf);
        return -1;
    }
    if (inbuf[1] != 0)
    {
        free(inbuf);
        fprintf(stderr, "EPS: GET_HK command error code %d\n", inbuf[1]);
        return -1;
    }
    memcpy(&(dev->hkparam), &(inbuf[2]), sizeof(hkparam_t));
    free(inbuf);
    return 1;
}

int eps_set_output(p31u *dev, channel_t channels)
{
    int status;
    char buf[2];
    buf[0] = SET_OUTPUT;
    buf[1] = channels.reg;
    if ((status = write(dev->file, buf, 2)) != 2)
    {
        perror("EPS"
               "I2C Write Failed in SET_OUTPUT");
        return status;
    }
    usleep(10000); // 10 ms
    if ((status = read(dev->file, buf, 2)) != 2)
    {
        perror("EPS"
               "I2C Read Failed in SET_OUTPUT");
        return status;
    }
    if (buf[0] != SET_OUTPUT)
    {
        fprintf(stderr, "EPS"
                        "SET_OUTPUT command failed with error code %d\n",
                buf[1]);
        return -1;
    }
    return 1;
}

int eps_set_single(p31u *dev, uint8_t channel, uint8_t value, int16_t delay)
{
    int status;
    char buf[5];
    buf[0] = SET_SINGLE_OUTPUT;
    buf[1] = channel;
    buf[2] = value;
    buf[3] = ((uint8_t *)&delay)[1];
    buf[4] = ((uint8_t *)&delay)[0];
    if ((status = write(dev->file, buf, 5)) != 5)
    {
        perror("EPS"
               "I2C Write Failed in SET_SINGLE_OUTPUT");
        return status;
    }
    usleep(10000); // 10 ms
    if ((status = read(dev->file, buf, 2)) != 2)
    {
        perror("EPS"
               "I2C Read Failed in SET_SINGLE_OUTPUT");
        return status;
    }
    if (buf[0] != SET_SINGLE_OUTPUT)
    {
        fprintf(stderr, "EPS"
                        "SET_SINGLE_OUTPUT command failed with error code %d\n",
                buf[1]);
        return -1;
    }
    return 1;
}

int eps_reset_wdt(p31u* dev)
{
    int status ;
    char buf[2] ;
    buf[0] = RESET_WDT ;
    buf[1] = 0x78 ; // magic
    if ((status = write(dev->file, buf, 2)) != 2)
    {
        perror("EPS"
               "I2C Write Failed in RESET_WDT");
        return status;
    }
    return 1 ;
}