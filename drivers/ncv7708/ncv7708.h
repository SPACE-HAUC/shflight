#ifndef NCV7708_H
#define NCV7708_H
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

typedef struct {
    union
    {
        unsigned short cmd; //Combined bits
        struct //separated bits
        {
            unsigned char ovlo   : 1; //over voltage lockout
            unsigned char hbcnf1 : 1; //half bridge 1 configuration (1 -> LS1 off and HS1 on, 0 -> LS1 on and HS1 off)
            unsigned char hbcnf2 : 1;
            unsigned char hbcnf3 : 1;
            unsigned char hbcnf4 : 1;
            unsigned char hbcnf5 : 1;
            unsigned char hbcnf6 : 1;
            unsigned char hben1  : 1; //half bridge 1 enable (1 -> bridge in use, 0 -> bridge not in use)
            unsigned char hben2  : 1;
            unsigned char hben3  : 1;
            unsigned char hben4  : 1;
            unsigned char hben5  : 1;
            unsigned char hben6  : 1;
            unsigned char uldsc  : 1; //under load detection shutdown
            unsigned char hbsel  : 1; //half bridge selection (needs to be set to 0)
            unsigned char srr    : 1; //status reset register: 1 -> clear all faults and reset
        };
    };
    union
    {
        unsigned short data;
        struct
        {
            unsigned char tw      : 1; //thermal warning
            unsigned char hbcr1   : 1; //half bridge 1 configuration reporting (mirrors command)
            unsigned char hbcr2   : 1;
            unsigned char hbcr3   : 1;
            unsigned char hbcr4   : 1;
            unsigned char hbcr5   : 1;
            unsigned char hbcr6   : 1;
            unsigned char hbst1   : 1; //half bridge 1 enable status (mirrors command)
            unsigned char hbst2   : 1;
            unsigned char hbst3   : 1;
            unsigned char hbst4   : 1;
            unsigned char hbst5   : 1;
            unsigned char hbst6   : 1;
            unsigned char uld     : 1; //under load detection (1 -> fault)
            unsigned char psf     : 1; //power supply failure
            unsigned char ocs     : 1; //over current shutdown
        };
    };
} ncv7708_packet ;

typedef struct {
    struct spi_ioc_transfer xfer[1];
    int file;
    __u8 mode, lsb, bits ;
    __u32 speed ;
    char fname[40] ;
    ncv7708_packet * pack ;
} ncv7708 ;

int ncv7708_init(ncv7708 *) ;
int ncv7708_transfer(ncv7708 *, uint16_t * , uint16_t * );
int ncv7708_xfer(ncv7708 * );
void ncv7708_destroy(ncv7708 *);

#endif // NCV7708_H