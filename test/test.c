#include <stdio.h>
#include <stdlib.h>
#include <main_helper.h>
#include <pthread.h>
#include <signal.h>
#include <math.h>
#include <errno.h>

pthread_mutex_t serial_read, serial_write, data_check;
pthread_cond_t data_available ;

volatile sig_atomic_t done = 0 ;
volatile int first_run = 1 ;

void catch_sigint()
{
    done = 1 ;
}

#define SH_BUFFER_SIZE 64
#define DIPOLE_MOMENT 0.22             // A m^-2
#define MAX_DETUMBLE_FIRING_TIME 80000 // 80 ms
#define DETUMBLE_TIME_STEP 100000      // 100 ms
#define MIN_DETUMBLE_FIRING_TIME 10000 // 10 ms
DECLARE_BUFFER(g_W, float);
DECLARE_BUFFER(g_B, float);
DECLARE_BUFFER(g_Bt, float);
DECLARE_VECTOR(g_L_target, float);
int mag_index = -1, omega_index = -1, bdot_index = -1;
unsigned long long acs_ct = 0 ;
float MOI[3][3] = {{0.0647, 0, 0},{0, 0.0647, 0},{0, 0, 0.0792}};
unsigned char g_Fire;

void insertionSort(int a1[], int a2[])
{
  for (int step = 1; step < 3; step++)
  {
    int key1 = a1[step];
    int key2 = a2[step];
    int j = step - 1;
    while (key1 < a1[j] && j >= 0)
    {
      // For descending order, change key<array[j] to key>array[j].
      a1[j + 1] = a1[j];
      a2[j + 1] = a2[j];
      --j;
    }
    a1[j + 1] = key1;
    a2[j + 1] = key2;
  }
}

#define HBRIDGE_ENABLE(name) \
    hbridge_enable(x_##name, y_##name, z_##name);

int hbridge_enable(int x, int y, int z)
{
    uint8_t val = 0x00;
    // Set up Z
    val |= z > 0 ? 0x01 : (z < 0 ? 0x02 : 0x00);
    val << 2;
    // Set up Y
    val |= y > 0 ? 0x01 : (y < 0 ? 0x02 : 0x00);
    val << 2;
    // Set up X
    val |= x > 0 ? 0x01 : (x < 0 ? 0x02 : 0x00);
    val << 2;
    pthread_mutex_lock(&serial_write);
    g_Fire = val;
    pthread_mutex_unlock(&serial_write);
    return val;
}

int HBRIDGE_DISABLE(int i)
{
    int tmp = 0xff;
    tmp ^= 0x03 << 2 * i;
    pthread_mutex_lock(&serial_write);
    g_Fire &= tmp;
    pthread_mutex_unlock(&serial_write);
    return tmp;
}

#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <pthread.h>
#include <main_helper.h>
DECLARE_VECTOR(g_readB, short); // storage to put helmhotz values
short g_readFS[2];               // storage to put FS X and Y angles
unsigned short g_readCS[9];      // storage to put CS led brightnesses

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

void *sitl_comm(void* id)
{
    int fd = setup_serial();
    if (fd < 0)
    {
        printf("Error getting serial fd\n");
        return NULL;
    }
    long long charsleep = 100;
    while (!done)
    {
        printf("Serial: Beginning loop\n");
        unsigned char inbuf[30], obuf, tmp, val = 0;
        int frame_valid = 1, nr = 0;
#ifdef RPI
        digitalWrite(READ_SIG, 1);
#endif
        // read 10 0xa0 bytes to make sure you got the full frame
        int preamble_count = -10;
        do
        {
            if ( done ){
                printf("Serial: Breaking preamble loop\n");
                break ;
            }
            int rd = read(fd, &tmp, 1);
            usleep(charsleep); // wait till next byte arrives
            if (tmp == 0xa0 && rd == 1)
                preamble_count++;
            else
                preamble_count = -10;
        } while ((preamble_count < 0));
        if (first_run)
            pthread_cond_broadcast(&data_available);
        if ( done )
            continue ;
        // read data
        usleep(charsleep * 30);      // wait till data is available
        int n = read(fd, inbuf, 30); // read actual data
        // read end frame
        for (int i = 28; i < 30; i++)
        {
            if (inbuf[i] != 0xb0)
                frame_valid = 2;
        }
#ifdef RPI
        digitalWrite(READ_SIG, 0);
#endif
        pthread_mutex_lock(&serial_write); // protect the g_Fire reading and avoid race condition with ACS thread
        obuf = g_Fire;
        pthread_mutex_unlock(&serial_write);
#ifdef RPI
        digitalWrite(WRITE_SIG, 1);
#endif
        nr = write(fd, &obuf, 1);
        tcflush(fd, TCOFLUSH); // flush the output buffer
#ifdef RPI
        digitalWrite(WRITE_SIG, 0);
#endif
        usleep(charsleep);
        if (n < 36 || nr != 1 || frame_valid != 0)
            continue; // go back to beginning of the loop if the frame is bad
        // acquire lock before starting to assign to variables that are going to be read by data_acq thread
        pthread_mutex_lock(&serial_read);
        x_g_readB = *((unsigned short *)inbuf);      // first element, little endian order
        y_g_readB = *((unsigned short *)&inbuf[2]);  // second element
        z_g_readB = *((unsigned short *)&inbuf[4]);  // third element
        int offset = 6;
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
        // convert B to proper units
        VECTOR_MIXED(g_readB, g_readB, 2047, -);
        VECTOR_MIXED(g_readB, g_readB, 65e-6/2047, *);
    }
    pthread_exit(NULL);
}

void getOmega(void)
{
    if (mag_index < 2) // not enough measurements
        return;
    // once we have measurements, we declare that we proceed
    omega_index = (1 + omega_index) % SH_BUFFER_SIZE;
    int8_t m0, m1;
    m1 = bdot_index;
    m0 = (bdot_index - 1) < 0 ? SH_BUFFER_SIZE - bdot_index - 1 : bdot_index - 1;
    float freq;
    freq = 1. / DETUMBLE_TIME_STEP;
    CROSS_PRODUCT(g_W[omega_index], g_Bt[m1], g_Bt[m0]); // apply cross product
    float invnorm = INVNORM(g_Bt[m0]);
    VECTOR_MIXED(g_W[omega_index], g_W[omega_index], freq * invnorm, *); // omega = (B_t dot x B_t-dt dot)*freq/Norm(B_t dot)
    // Apply correction
    DECLARE_VECTOR(omega_corr0, float);                            // declare temporary space for correction vector
    MATVECMUL(omega_corr0, MOI, g_W[m1]);                          // MOI X w[t-1]
    DECLARE_VECTOR(omega_corr1, float);                            // declare temporary space for correction vector
    CROSS_PRODUCT(omega_corr1, g_W[m1], omega_corr0);              // store into temp 1
    MATVECMUL(omega_corr1, MOI, omega_corr0);                      // store back into temp 0
    VECTOR_MIXED(omega_corr1, omega_corr1, -freq, *);              // omega_corr = freq*MOI*(-w[t-1] X MOI*w[t-1])
    VECTOR_OP(g_W[omega_index], g_W[omega_index], omega_corr1, +); // add the correction term to omega
    return;
}

int readSensors(void)
{
    // read magfield
    int status = 1;
    mag_index = (mag_index + 1) % SH_BUFFER_SIZE;
    pthread_mutex_lock(&serial_read);
    VECTOR_CLEAR(g_B[mag_index]);                                  // clear the current B
    VECTOR_OP(g_B[mag_index], g_B[mag_index], g_readB, +);         // load B - equivalent reading from sensor
    VECTOR_MIXED(g_B[mag_index], g_B[mag_index], 65e-6 / 2048, *); // recover B
    pthread_mutex_unlock(&serial_read);
    // put values into g_Bx, g_By and g_Bz at [mag_index] and takes 18 ms to do so (implemented using sleep)
    if (mag_index < 1)
        return status;
    // if we have > 1 values, calculate Bdot
    bdot_index = (bdot_index + 1) % SH_BUFFER_SIZE;
    int8_t m0, m1;
    m1 = mag_index;
    m0 = (mag_index - 1) < 0 ? SH_BUFFER_SIZE - mag_index - 1 : mag_index - 1;
    double freq = 1. / DETUMBLE_TIME_STEP;
    VECTOR_OP(g_Bt[bdot_index], g_B[m1], g_B[m0], -);
    VECTOR_MIXED(g_Bt[bdot_index], g_Bt[bdot_index], freq, *);
    getOmega();
    return status;
}

void *acs_detumble(void *id)
{
    while (!done)
    {
        // wait till there is available data on serial
        if ( first_run )
        {
            printf("ACS: Waiting for release...\n");
            pthread_cond_wait(&data_available, &data_check) ;
            printf("ACS: Released!\n");
            first_run = 0 ;
        }
        unsigned long long s = get_usec();
        readSensors();
        time_t now ; time(&now);
        if ( omega_index > 0 )
            printf("%s ACS step: %llu | Wx = %f Wy = %f Wz = %f\n", ctime(&now), acs_ct++ , x_g_W[omega_index], y_g_W[omega_index], z_g_W[omega_index]);
        unsigned long long e = get_usec();
        usleep(20000 - e + s);                   // sleep for total 20 ms with read
        DECLARE_VECTOR(currL, float);            // vector for current angular momentum
        MATVECMUL(currL, MOI, g_W[omega_index]); // calculate current angular momentum
        VECTOR_OP(currL, g_L_target, currL, -);  // calculate angular momentum error
        DECLARE_VECTOR(currLNorm, float);
        NORMALIZE(currLNorm, currL);                                                    // normalize the angular momentum error vector
        DECLARE_VECTOR(currB, float);                                                   // current normalized magnetic field TMP
        NORMALIZE(currB, g_B[mag_index]);                                               // normalize B
        DECLARE_VECTOR(firingDir, float);                                               // firing direction vector
        CROSS_PRODUCT(firingDir, currB, currLNorm);                                     // calculate firing direction
        int8_t x_fire = (x_firingDir < 0 ? -1 : 1) * (abs(x_firingDir) > 0.01 ? 1 : 0); // if > 0.01, then fire in the direction of input
        int8_t y_fire = (y_firingDir < 0 ? -1 : 1) * (abs(y_firingDir) > 0.01 ? 1 : 0); // if > 0.01, then fire in the direction of input
        int8_t z_fire = (z_firingDir < 0 ? -1 : 1) * (abs(z_firingDir) > 0.01 ? 1 : 0); // if > 0.01, then fire in the direction of input
        DECLARE_VECTOR(currDipole, float);
        VECTOR_MIXED(currDipole, fire, DIPOLE_MOMENT, *); // calculate dipole moment
        DECLARE_VECTOR(currTorque, float);
        CROSS_PRODUCT(currTorque, currDipole, g_B[mag_index]); // calculate current torque
        DECLARE_VECTOR(firingTime, float);                     // initially gives firing time in seconds
        VECTOR_OP(firingTime, currL, currTorque, /);           // calculate firing time based on current torque
        VECTOR_MIXED(firingTime, firingTime, 1000000, *);      // convert firing time to usec
        DECLARE_VECTOR(firingCmd, int);                        // integer firing time in usec
        x_firingCmd = x_firingTime > MAX_DETUMBLE_FIRING_TIME ? MAX_DETUMBLE_FIRING_TIME : (x_firingTime < MIN_DETUMBLE_FIRING_TIME ? 0 : (int)x_firingTime);
        y_firingCmd = y_firingTime > MAX_DETUMBLE_FIRING_TIME ? MAX_DETUMBLE_FIRING_TIME : (y_firingTime < MIN_DETUMBLE_FIRING_TIME ? 0 : (int)y_firingTime);
        z_firingCmd = z_firingTime > MAX_DETUMBLE_FIRING_TIME ? MAX_DETUMBLE_FIRING_TIME : (z_firingTime < MIN_DETUMBLE_FIRING_TIME ? 0 : (int)z_firingTime);
        int firingOrder[3] = {0, 1, 2}, firingTime[3]; // 0 == x, 1 == y, 2 == z
        firingTime[0] = x_firingCmd;
        firingTime[1] = y_firingCmd;
        firingTime[2] = z_firingCmd;
        insertionSort(firingTime, firingOrder); // sort firing order based on firing time
        int finalWait = MAX_DETUMBLE_FIRING_TIME - firingTime[2];
        firingTime[2] -= firingTime[1];                // time after second one turns off
        firingTime[1] -= firingTime[0];                // time after first one turns off
        HBRIDGE_ENABLE(fire);                          // Turns on the torque coils in the required directions determined by the fire vector
        usleep(firingTime[0] < 1 ? 1 : firingTime[0]); // sleep until first turnoff
        HBRIDGE_DISABLE(firingOrder[0]);               // first turn off
        usleep(firingTime[1] < 1 ? 1 : firingTime[1]); // sleep until second turnoff
        HBRIDGE_DISABLE(firingOrder[1]);               // second turnoff
        usleep(firingTime[2] < 1 ? 1 : firingTime[2]); // sleep until third turnoff
        HBRIDGE_DISABLE(firingOrder[2]);               // third turnoff
        usleep(finalWait < 1 ? 1 : finalWait);         // sleep for the remainder of the cycle
    }
    pthread_exit(NULL);
}

int main(void)
{
    // handle sigint
    struct sigaction saction ;
    saction.sa_handler = &catch_sigint;
    sigaction(SIGINT, &saction, NULL);

    int rc0, rc1;
    pthread_t thread0 , thread1 ;
    pthread_attr_t attr ;
    void* status ;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr,PTHREAD_CREATE_JOINABLE);
    rc0 = pthread_create(&thread0,&attr,acs_detumble,(void *)0);
    if (rc0){
        printf("Main: Error: Unable to create ACS thread %d: Errno %d\n", rc0, errno);
        exit(-1) ; 
    }
    rc1 = pthread_create(&thread1,&attr,sitl_comm,(void *)0);
    if (rc1){
        printf("Main: Error: Unable to create ACS thread %d: Errno %d\n", rc1, errno);
        exit(-1) ; 
    }

    pthread_attr_destroy(&attr) ;

    rc0 = pthread_join(thread0,&status) ;
    if (rc0){
        printf("Main: Error: Unable to join ACS thread %d: Errno %d\n", rc0, errno);
    }

    rc1 = pthread_join(thread1,&status) ;
    if (rc1){
        printf("Main: Error: Unable to join Serial thread %d: Errno %d\n", rc1, errno);
    }

    return 0 ;
}
