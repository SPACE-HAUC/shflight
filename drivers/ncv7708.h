/**
 * @file ncv7708.h
 * @author Sunip K. Mukherjee (sunipkmukherjee@gmail.com)
 * @brief Function prototypes and data structure for NCV77X8 SPI Driver (Linux)
 * @version 0.1
 * @date 2020-03-19
 * 
 * @copyright Copyright (c) 2020
 * 
 */
#ifndef NCV7708_H
#define NCV7708_H
#include <linux/types.h>
#include <linux/spi/spidev.h>
/**
 * @brief NCV77X8 Data packet (I/O)
 * 
 */
typedef struct
{
    union {
        unsigned short cmd; ///< Combined bits
        struct              ///< separated bits
        {
            unsigned char ovlo : 1;   ///< over voltage lockout
            unsigned char hbcnf1 : 1; ///< half bridge 1 configuration (1 -> LS1 off and HS1 on, 0 -> LS1 on and HS1 off)
            unsigned char hbcnf2 : 1;
            unsigned char hbcnf3 : 1;
            unsigned char hbcnf4 : 1;
            unsigned char hbcnf5 : 1;
            unsigned char hbcnf6 : 1;
            unsigned char hben1 : 1; ///< half bridge 1 enable (1 -> bridge in use, 0 -> bridge not in use)
            unsigned char hben2 : 1;
            unsigned char hben3 : 1;
            unsigned char hben4 : 1;
            unsigned char hben5 : 1;
            unsigned char hben6 : 1;
            unsigned char uldsc : 1; ///< under load detection shutdown
            unsigned char hbsel : 1; ///< half bridge selection (needs to be set to 0)
            unsigned char srr : 1;   ///< status reset register: 1 -> clear all faults and reset
        };
    };
    union {
        unsigned short data;
        struct
        {
            unsigned char tw : 1;    ///< thermal warning
            unsigned char hbcr1 : 1; ///< half bridge 1 configuration reporting (mirrors command)
            unsigned char hbcr2 : 1;
            unsigned char hbcr3 : 1;
            unsigned char hbcr4 : 1;
            unsigned char hbcr5 : 1;
            unsigned char hbcr6 : 1;
            unsigned char hbst1 : 1; ///< half bridge 1 enable status (mirrors command)
            unsigned char hbst2 : 1;
            unsigned char hbst3 : 1;
            unsigned char hbst4 : 1;
            unsigned char hbst5 : 1;
            unsigned char hbst6 : 1;
            unsigned char uld : 1; ///< under load detection (1 -> fault)
            unsigned char psf : 1; ///< power supply failure
            unsigned char ocs : 1; ///< over current shutdown
        };
    };
} ncv7708_packet;
/**
 * @brief NCV77X8 Device
 * 
 */
typedef struct
{
    struct spi_ioc_transfer xfer[1]; ///< SPI Transfer IO buffer
    int file;                        ///< File descriptor for SPI bus
    __u8 mode;                       ///< SPI Mode (Mode 0)
    __u8 lsb;                        ///< MSB First
    __u8 bits;                       ///< Number of bits per transfer (16)
    __u32 speed;                     ///< SPI Bus speed (1 MHz)
    char fname[40];                  ///< SPI device file name
    ncv7708_packet *pack;            ///< Pointer to ncv7708_packet for internal consistency
} ncv7708;

int ncv7708_init(ncv7708 *);
int ncv7708_transfer(ncv7708 *, uint16_t *, uint16_t *);
int ncv7708_xfer(ncv7708 *);
void ncv7708_destroy(ncv7708 *);

#endif // NCV7708_H