#include "ncv7708.h"

// Initialize the SPI bus to communicate with the NCV7708
int ncv7708_init(ncv7708 * dev)
    {
        int file;
    __u8    mode, lsb, bits;
    __u32 speed=2500000;
    
    // allocate memory for data packets
    dev->pack = (ncv7708_packet*) malloc(sizeof(ncv7708_packet)) ;
    
        if ((file = open(dev->fname,O_RDWR)) < 0)
        {
            /* ERROR HANDLING; you can check errno to see what went wrong */
            perror("NCV7708: ""Opening bus");
            }
 
        ///////////////
        // Verifications
        ///////////////
        //possible modes: mode |= SPI_LOOP; mode |= SPI_CPHA; mode |= SPI_CPOL; mode |= SPI_LSB_FIRST; mode |= SPI_CS_HIGH; mode |= SPI_3WIRE; mode |= SPI_NO_CS; mode |= SPI_READY;
        //multiple possibilities using |
            mode =SPI_MODE_1 ;
            if (ioctl(file, SPI_IOC_WR_MODE, &mode)<0)   {
                perror("NCV7708: ""can't set spi mode");
                return 0;
            }
        
 
            if (ioctl(file, SPI_IOC_RD_MODE, &mode) < 0)
                {
                perror("NCV7708: ""SPI rd_mode");
                return 0;
            }
            lsb = 0 ;
            if (ioctl(file, SPI_IOC_WR_LSB_FIRST, &lsb) < 0)
                {
                perror("NCV7708: ""SPI rd_lsb_fist");
                return 0;
            }
            if (ioctl(file, SPI_IOC_RD_LSB_FIRST, &lsb) < 0)
                {
                perror("NCV7708: ""SPI rd_lsb_fist");
                return 0;
            }
            bits = 0 ;
        
            if (ioctl(file, SPI_IOC_WR_BITS_PER_WORD, &bits)<0)   
                {
                perror("NCV7708: ""can't set bits per word");
                return 0;
            }
        
            if (ioctl(file, SPI_IOC_RD_BITS_PER_WORD, &bits) < 0) 
                {
                perror("NCV7708: ""SPI bits_per_word");
                return 0;
            }
            speed = 2000000 ;        
            if (ioctl(file, SPI_IOC_WR_MAX_SPEED_HZ, &speed)<0)  
                {
                perror("NCV7708: ""can't set max speed hz");
                return 0;
                }
        
            if (ioctl(file, SPI_IOC_RD_MAX_SPEED_HZ, &speed) < 0) 
                {
                perror("NCV7708: ""SPI max_speed_hz");
                return 0;
                }
     
 
    fprintf(stderr, "%s: spi mode %d, %d bits %sper word, %d Hz max\n",dev->fname, mode, bits, lsb ? "(lsb first) " : "", speed);
    dev->mode = mode ;
    dev->bits = bits ;
    dev->lsb  = lsb ;
    dev->file = file ;
    //xfer[0].tx_buf = (unsigned long)buf;
    dev->xfer[0].len = 2; /* Length of  command to write*/
    dev->xfer[0].cs_change = 0; /* Keep CS activated */
    dev->xfer[0].delay_usecs = 0; //delay in us
    dev->xfer[0].speed_hz = 1000*1000; //speed
    dev->xfer[0].bits_per_word = 8; // bites per word 8
 
    //xfer[1].rx_buf = (unsigned long) buf2;
    //dev->xfer[1].len = 2; /* Length of Data to read */
    //dev->xfer[1].cs_change = 1; /* Keep CS activated */

    return 1; // successful
}
 
int ncv7708_transfer(ncv7708 * dev, uint16_t * data, uint16_t * cmd)
{
    int status = 0 ;
    char inbuf[2] , outbuf[2] ;
    outbuf[1] = ((char*)cmd)[0] ;
    outbuf[0] = ((char*)cmd)[1] ;//(char) ((*cmd) >> 8) ;
    dev->xfer[0].tx_buf = (unsigned long) outbuf ;
    dev->xfer[0].rx_buf = (unsigned long) inbuf  ;
    dev->xfer[0].len    = 2                      ;
    status = ioctl(dev->file, SPI_IOC_MESSAGE(1), dev->xfer) ;
    if (status < 0)
    {
        perror("NCV7708: ""SPI_IOC_MESSAGE");
        return -1;
    }
    *data = ((uint16_t)(inbuf[1]))|(((uint16_t)(inbuf[0]))<<8) ;
    return status ;
}

int ncv7708_xfer(ncv7708 * dev)
{
    return ncv7708_transfer(dev, &(dev->pack->data), &(dev->pack->cmd));
}

void ncv7708_destroy(ncv7708 * dev)
{
    close(dev->file);
    free(dev->pack);
    free(dev);
}
 
// //////////
// // Read n bytes from the 2 bytes add1 add2 address
// //////////
 
// char * spi_read(int add1,int add2,int nbytes,int file)
//     {
//     int status;
 
//     memset(buf, 0, sizeof buf);
//     memset(buf2, 0, sizeof buf2);
//     buf[0] = 0x01;
//     buf[1] = add1;
//     buf[2] = add2;
//     xfer[0].tx_buf = (unsigned long)buf;
//     xfer[0].len = 3; /* Length of  command to write*/
//     xfer[1].rx_buf = (unsigned long) buf2;
//     xfer[1].len = nbytes; /* Length of Data to read */
//     status = ioctl(file, SPI_IOC_MESSAGE(2), xfer);
//     if (status < 0)
//         {
//         perror("NCV7708: ""SPI_IOC_MESSAGE");
//         return NULL;
//         }
//     //printf("env: %02x %02x %02x\n", buf[0], buf[1], buf[2]);
//     //printf("ret: %02x %02x %02x %02x\n", buf2[0], buf2[1], buf2[2], buf2[3]);
 
//     com_serial=1;
//     failcount=0;
//     return buf2;
//     }
 
// //////////
// // Write n bytes int the 2 bytes address add1 add2
// //////////
// void spi_write(int add1,int add2,int nbytes,char value[10],int file)
//     {
//     unsigned char   buf[32], buf2[32];
//     int status;
 
//     memset(buf, 0, sizeof buf);
//     memset(buf2, 0, sizeof buf2);
//     buf[0] = 0x00;
//     buf[1] = add1;
//     buf[2] = add2;
//     if (nbytes>=1) buf[3] = value[0];
//     if (nbytes>=2) buf[4] = value[1];
//     if (nbytes>=3) buf[5] = value[2];
//     if (nbytes>=4) buf[6] = value[3];
//     xfer[0].tx_buf = (unsigned long)buf;
//     xfer[0].len = nbytes+3; /* Length of  command to write*/
//     status = ioctl(file, SPI_IOC_MESSAGE(1), xfer);
//     if (status < 0)
//         {
//         perror("NCV7708: ""SPI_IOC_MESSAGE");
//         return;
//         }
//     //printf("env: %02x %02x %02x\n", buf[0], buf[1], buf[2]);
//     //printf("ret: %02x %02x %02x %02x\n", buf2[0], buf2[1], buf2[2], buf2[3]);
 
//     com_serial=1;
//     failcount=0;
//     }

// #include <stdio.h>

// int main()
// {   
//     // struct sigaction action;
//     // memset(&action, 0, sizeof(struct sigaction));
//     // action.sa_handler = term;
//     // sigaction(SIGTERM, &action, NULL);
    
//     ncv7708 * hbridge = (ncv7708*)malloc(sizeof(ncv7708));
//     snprintf(hbridge->fname,40,"/dev/spidev1.0");
//     int status = ncv7708_init(hbridge);
//     ncv7708_packet * x = (ncv7708_packet*)malloc(sizeof(ncv7708_packet)) ;
//     x->cmd = 0 ;
//     x->srr = 1 ;
//     uint16_t cmd = x->cmd , data ;
//     status = ncv7708_transfer(hbridge, &data, &cmd) ;
//     printf("Status: %d Cmd: 0x%x Data: 0x%x\n", status, cmd, data) ;
//     sleep(1);
//     x->srr = 0 ;
//     x->hben1 = 1 ;
//     x->hbsel = 0 ;
//     cmd = x->cmd ;
//     status = ncv7708_transfer(hbridge, &data, &cmd) ;
//     printf("Status: %d Cmd: 0x%x Data: 0x%x\n", status, cmd, data) ;
//     sleep(1);
//     int i = 0 ;
//     while(1){
//     x->hben1 = 1 ;
//     x->hbcnf1 = 1 ;
//     cmd = x->cmd ;
//     status = ncv7708_transfer(hbridge, &data, &cmd) ;
//     //ioctl(file, CMD_SPI_SET_CSLOW,1);
//     printf("Status: %d Cmd: 0x%x Data: 0x%x\n", status, cmd, data) ;
//     sleep(1);
//     x->hbcnf1 = 0 ;
//     x->hben1 = 1 ;
//     cmd = x->cmd ;
//     status = ncv7708_transfer(hbridge, &data, &cmd) ;
//     printf("Status: %d Cmd: 0x%x Data: 0x%x\n", status, cmd, data) ;
//     sleep(1);
//     i++ ;
//     }
//     x->hbcnf1 = 0 ;
//     cmd = x->cmd ;
//     status = ncv7708_transfer(hbridge, &data, &cmd) ;
//     printf("Status: %d Cmd: 0x%x Data: 0x%x\n", status, cmd, data) ;
//     ncv7708_destroy(hbridge) ;
//     free(x);
//     return 0 ;
// }