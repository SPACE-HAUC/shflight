#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <pthread.h>
#include <main_helper.h>

extern pthread_mutex_t serial_read, serial_write;
extern unsigned char dipole;
DECLARE_VECTOR(g_readBp, extern short); // storage to put helmhotz positives
DECLARE_VECTOR(g_readBn, extern short); // storage to put helmhotz negatives
extern short g_readFS[2];               // storage to put FS X and Y angles
extern unsigned short g_readCS[9];      // storage to put CS led brightnesses

#ifdef RPI
#include <wiringPi.h>
#define WRITE_SIG 2 // 27
#define READ_SIG 0  // 17
#endif

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
/*
 * Initialize serial comm for SITL test.
 * Input: Pass argc and argv from main; inputs are port and baud
 * Output: Serial file descriptor
 * 
 * TODO: Put the fd in a global for Use
 */
int setup_serial(void)
{
    int fd = open("/dev/ttyS0", O_RDWR | O_NOCTTY | O_SYNC);
    if (fd < 0)
    {
        printf("error %d opening TTY: %s", errno, strerror(errno));
        return -1;
    }
    set_interface_attribs(fd, B115200, 0);
    set_blocking(fd, 0);
    return fd;
}

/*
 * This thread waits to read a dataframe, reads it over serial, writes the dipole action to serial and interprets the
 * dataframe that was read over serial. The values are atomically assigned.
 */

void *sitl_comm(void)
{
    int fd = setup_serial();
    if (fd < 0)
    {
        printf("Error getting serial fd\n");
        return NULL;
    }
    long long charsleep = 100;
    while (1)
    {
        unsigned char inbuf[36], obuf, tmp, val = 0, frame_valid = 1;
        int frame_valid = 1, nr = 0;
#ifdef RPI
        digitalWrite(READ_SIG, 1);
#endif
        // read 10 0xa0 bytes to make sure you got the full frame
        int preamble_count = -10;
        do
        {
            int rd = read(fd, &tmp, 1);
            usleep(charsleep); // wait till next byte arrives
            if (tmp == 0xa0 && rd == 1)
                preamble_count++;
            else
                preamble_count = -10;
        } while ((preamble_count < 0));
        // read data
        usleep(charsleep * 36);      // wait till data is available
        int n = read(fd, inbuf, 36); // read actual data
        // read end frame
        for (int i = 34; i < 36; i++)
        {
            if (inbuf[i] != 0xb0)
                frame_valid = 2;
        }
#ifdef RPI
        digitalWrite(READ_SIG, 0);
#endif
        pthread_mutex_lock(&serial_write); // protect the dipole reading and avoid race condition with ACS thread
        obuf = dipole;
        pthread_mutex_unlock(&serial_write);
#ifdef RPI
        digitalWrite(WRITE_SIG, 1);
#endif
        nr = write(fd, &obuf, 1);
        tcoflush(fd, TCOFLUSH); // flush the output buffer
#ifdef RPI
        digitalWrite(WRITE_SIG, 0);
#endif
        usleep(charsleep);
        if (n < 36 || nr != 1 || frame_valid != 0)
            continue; // go back to beginning of the loop if the frame is bad
        // acquire lock before starting to assign to variables that are going to be read by data_acq thread
        pthread_mutex_lock(&serial_read);
        x_g_readBp = *((unsigned short *)inbuf);      // first element, little endian order
        y_g_readBp = *((unsigned short *)&inbuf[4]);  // second element
        z_g_readBp = *((unsigned short *)&inbuf[8]);  // third element
        x_g_readBn = *((unsigned short *)inbuf[2]);   // first element, little endian order
        y_g_readBn = *((unsigned short *)&inbuf[8]);  // second element
        z_g_readBn = *((unsigned short *)&inbuf[10]); // third element
        int offset = 12;
        for (int i = 0; i < 9; i++)
        {
            g_readCS[i] = *((unsigned short *)&inbuf[offset + 2 * i]);
        }
        offset += 18; // read the FS shorts
        for (int i = 0; i < 2; i++)
        {
            g_readFS[i] = *((unsigned short *)&inbuf[offset + 2 * i]);
        }
        pthread_mutex_unlock(&serial_read);
    }
    return NULL;
}