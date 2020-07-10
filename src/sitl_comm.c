/**
 * @file sitl_comm.c
 * @author Sunip K. Mukherjee (sunipkmukherjee@gmail.com)
 * @brief Software-In-The-Loop (SITL) serial communication codes
 * @version 0.2
 * @date 2020-03-19
 * 
 * @copyright Copyright (c) 2020
 * 
 */
#include <sitl_comm.h>
#include <acs_extern.h>
#include <main.h>
#include <stdio.h>
#include <stdint.h>
#include <pthread.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <stdint.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <termios.h>

/**
 * @brief Mutex to ensure atomicity of serial data read into the system.
 * 
 */
pthread_mutex_t serial_read;
/**
 * @brief Mutex to ensure atomicity of magnetorquer output for serial communication.
 * 
 */
pthread_mutex_t serial_write;
/**
 * @brief SITL communication time.
 * 
 */
unsigned long long t_comm = 0;
unsigned long long comm_time;

int set_interface_attribs(int fd, int speed, int parity)
{
    struct termios tty;
    if (tcgetattr(fd, &tty) != 0)
    {
        perror("error from tcgetattr");
        return -1;
    }

    cfsetospeed(&tty, speed);
    cfsetispeed(&tty, speed);

    tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8; // 8-bit chars
    // disable IGNBRK for mismatched speed tests; otherwise receive break
    // as \000 chars
    tty.c_iflag &= ~IGNBRK; // disable break processing
    tty.c_lflag = 0;        // no signaling chars, no echo,
                            // no canonical processing
    tty.c_oflag = 0;        // no remapping, no delays
    tty.c_cc[VMIN] = 0;     // read doesn't block
    tty.c_cc[VTIME] = 5;    // 0.5 seconds read timeout

    tty.c_iflag &= ~(IXON | IXOFF | IXANY); // shut off xon/xoff ctrl

    tty.c_cflag |= (CLOCAL | CREAD);   // ignore modem controls,
                                       // enable reading
    tty.c_cflag &= ~(PARENB | PARODD); // shut off parity
    tty.c_cflag |= parity;
    tty.c_cflag &= ~CSTOPB;
    tty.c_cflag &= ~CRTSCTS; // gnu99 compilation

    if (tcsetattr(fd, TCSANOW, &tty) != 0)
    {
        perror("error from tcsetattr");
        return -1;
    }
    return 0;
}

void set_blocking(int fd, int should_block)
{
    struct termios tty;
    memset(&tty, 0, sizeof tty);
    if (tcgetattr(fd, &tty) != 0)
    {
        perror("error from tggetattr");
        return;
    }

    tty.c_cc[VMIN] = should_block ? 1 : 0;
    tty.c_cc[VTIME] = 5; // 0.5 seconds read timeout

    if (tcsetattr(fd, TCSANOW, &tty) != 0)
        perror("error setting term attributes");
}

int setup_serial(void)
{
    int fd = open(SITL_COMM_IFACE, O_RDWR | O_NOCTTY | O_SYNC);
    if (fd < 0)
    {
        printf("error %d opening TTY: %s\n", errno, strerror(errno));
        return -1;
    }
    set_interface_attribs(fd, B230400, 0);
    set_blocking(fd, 0);
    return fd;
}

void *sitl_comm(void *id)
{
    int fd = setup_serial();
    if (fd < 0)
    {
        printf(__FILE__": Error getting serial fd\n");
        return NULL;
    }
    long long charsleep = 75; // 30 for 2500000; // 75 for 230400
    while (!done)
    {
        // unsigned long long s = get_usec();
        unsigned char inbuf[30], obuf, tmp;
        int frame_valid = 1, nr = 0;
        // read 10 0xa0 bytes to make sure you got the full frame
        int preamble_count = -10;
        do
        {
            if (done)
            {
                printf("Serial: Breaking preamble loop\n");
                break;
            }
            int rd = read(fd, &tmp, 1);
            usleep(charsleep); // wait till next byte arrives
            if (tmp == 0xa0 && rd == 1)
                preamble_count++;
            else
                preamble_count = -10;
        } while ((preamble_count < 0));
        // if (first_run)
        pthread_cond_broadcast(&data_available);
        if (done)
            continue;
        // read data
        usleep(charsleep * 30);      // wait till data is available
        int n = read(fd, inbuf, 30); // read actual data
        // read end frame
        for (int i = 28; i < 30; i++)
        {
            if (inbuf[i] != 0xb0)
                frame_valid = 2;
        }
        pthread_mutex_lock(&serial_write); // protect the g_Fire reading and avoid race condition with ACS thread
        obuf = g_Fire;
        pthread_mutex_unlock(&serial_write);
        nr = write(fd, &obuf, 1);
        tcflush(fd, TCOFLUSH); // flush the output buffer
        usleep(charsleep);

        unsigned long long s = get_usec();
        comm_time = s - t_comm;
        t_comm = s;

        if (n < 30 || nr != 1 || frame_valid != 1)
        {
            //printf("n: %d, nr: %d, frame_valid = %d\n", n, nr, frame_valid);
            continue; // go back to beginning of the loop if the frame is bad
        }
        // acquire lock before starting to assign to variables that are going to be read by data_acq thread
        pthread_mutex_lock(&serial_read);
        x_g_readB = inbuf[0] | ((unsigned short)inbuf[1]) << 8; // first element, little endian order
        y_g_readB = inbuf[2] | ((unsigned short)inbuf[3]) << 8; // second element
        z_g_readB = inbuf[4] | ((unsigned short)inbuf[5]) << 8; // third element
        // printf("Serial: Raw B: ");
        // for ( int i = 0 ; i < 6 ; i++ )
        // {
        //     printf("0x%02x ", inbuf[i]);
        // }
        // printf("0x%x 0x%x 0x%x", x_g_readB, y_g_readB, z_g_readB);
        // printf("\n");
        int offset = 6;
        for (int i = 0; i < 9; i++)
        {
            g_readCS[i] = inbuf[offset + 2 * i] | ((unsigned short)inbuf[offset + 2 * i + 1]) << 8; //*((unsigned short *)&inbuf[offset + 2 * i]);
        }
        offset += 18; // read the FS shorts
        for (int i = 0; i < 2; i++)
        {
            g_readFS[i] = inbuf[offset + 2 * i] | ((unsigned short)inbuf[offset + 2 * i + 1]) << 8; //*((unsigned short *)&inbuf[offset + 2 * i]);
        }
        pthread_mutex_unlock(&serial_read);
        // unsigned long long e = get_usec();
        // printf("[%llu ms] Serial read\n", (e-s)/1000);
    }
    close(fd); // close the file descriptor for serial
    pthread_exit(NULL);
}