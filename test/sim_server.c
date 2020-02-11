#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <errno.h>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <signal.h>

volatile sig_atomic_t done = 0 ;

pthread_mutex_t serial_read, serial_write, data_check;
pthread_cond_t data_available;

pthread_mutex_t datavis_mutex;
pthread_cond_t datavis_drdy;

char datavis_dat[88];

void catch_sigint()
{
    done = 1;
    pthread_cond_broadcast(&datavis_drdy);
    return;
}

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
unsigned long long t_comm = 0;
unsigned long long comm_time;
void *sitl_comm(void *id)
{
    int fd = setup_serial();
    if (fd < 0)
    {
        printf("Error getting serial fd\n");
        return NULL;
    }
    long long charsleep = 100; // 30 for 2500000; // 75 for 230400
    while (!done)
    {
        // unsigned long long s = get_usec();
        unsigned char inbuf[90], obuf, tmp, val = 0;
        int frame_valid = 1;
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
        if (done)
            continue;
        // read data
        usleep(charsleep * 90);      // wait till data is available
        int n = read(fd, inbuf, 90); // read actual data
        // read end frame
        for (int i = 88; i < 90; i++)
        {
            if (inbuf[i] != 0xb0)
                frame_valid = 2;
        }

        if (n < 90 || frame_valid != 1)
        {
            printf("n: %d, nr: %d, frame_valid = %d\n", n, nr, frame_valid);
            continue; // go back to beginning of the loop if the frame is bad
        }
        // acquire lock before starting to assign to variables that are going to be read by data_acq thread
        memcpy(datavis_dat, inbuf, 88);
        pthread_cond_broadcast(&datavis_drdy);
        // unsigned long long e = get_usec();
        // printf("[%llu ms] Serial read\n", (e-s)/1000);
    }
    pthread_exit(NULL);
}

/* Data visualization thread */
#include <sys/socket.h>
#include <arpa/inet.h>

#ifndef PORT
#define PORT 12380
#endif

int firstrun = 1 ;

typedef struct sockaddr sk_sockaddr;

void *datavis_thread(void *t)
{
    int server_fd, new_socket, valread;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);

    // Creating socket file descriptor
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        perror("socket failed");
        //exit(EXIT_FAILURE);
    }

    // Forcefully attaching socket to the port 8080
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT,
                   &opt, sizeof(opt)))
    {
        perror("setsockopt");
        //exit(EXIT_FAILURE);
    }

    struct timeval timeout;
    timeout.tv_sec = 1;
    timeout.tv_usec = 0;

    if (setsockopt(server_fd, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout,
                   sizeof(timeout)) < 0)
        perror("setsockopt failed\n");

    if (setsockopt(server_fd, SOL_SOCKET, SO_SNDTIMEO, (char *)&timeout,
                   sizeof(timeout)) < 0)
        perror("setsockopt failed\n");

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    // Forcefully attaching socket to the port 8080
    if (bind(server_fd, (sk_sockaddr *)&address, sizeof(address)) < 0)
    {
        perror("bind failed");
        //exit(EXIT_FAILURE);
    }
    if (listen(server_fd, 32) < 0)
    {
        perror("listen");
        //exit(EXIT_FAILURE);
    }
    // cerr << "DataVis: Main: Server File: " << server_fd << endl ;
    while (!done)
    {
        if ( firstrun )
        {
            printf("DataVis: Waiting for release...\n");
            firstrun = 0 ;
        }
        pthread_cond_wait(&datavis_drdy, &datavis_mutex);
        if ((new_socket = accept(server_fd, (sk_sockaddr *)&address, (socklen_t *)&addrlen)) < 0)
        {
            perror("accept");
            // cerr << "DataVis: Accept from socket error!" <<endl ;
        }
        ssize_t numsent = send(new_socket, &datavis_dat, 88, 0);
        //cerr << "DataVis: Size of sent data: " << PACK_SIZE << endl ;
        if (numsent > 0 && numsent != 88)
        {
            perror("DataVis: Send: ");
        }
        //cerr << "DataVis: Data sent" << endl ;
        //valread = read(sock,recv_buf,32);
        //cerr << "DataVis: " << recv_buf << endl ;
        close(new_socket);
        //sleep(1);
        // cerr << "DataVis thread: Sent" << endl ;
    }
    close(server_fd);
    pthread_exit(NULL);
}

int main(void)
{
    // handle sigint
    struct sigaction saction;
    saction.sa_handler = &catch_sigint;
    sigaction(SIGINT, &saction, NULL);

    int rc0, rc1, rc2;
    pthread_t thread0, thread1, thread2;
    pthread_attr_t attr;
    void *status;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

    rc1 = pthread_create(&thread1, &attr, sitl_comm, (void *)0);
    if (rc1)
    {
        printf("Main: Error: Unable to create Serial thread %d: Errno %d\n", rc1, errno);
        exit(-1);
    }
    rc2 = pthread_create(&thread2, &attr, datavis_thread, (void *)0);
    if (rc2)
    {
        printf("Main: Error: Unable to create DataVis thread %d: Errno %d\n", rc2, errno);
        exit(-1);
    }

    pthread_attr_destroy(&attr);

    rc1 = pthread_join(thread1, &status);
    if (rc1)
    {
        printf("Main: Error: Unable to join Serial thread %d: Errno %d\n", rc1, errno);
    }
    rc2 = pthread_join(thread2, &status);
    if (rc2)
    {
        printf("Main: Error: Unable to join DataVis thread %d: Errno %d\n", rc2, errno);
    }

    return 0;
}