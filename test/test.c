#include <stdio.h>
#include <stdlib.h>
#include <main_helper.h>
#include <pthread.h>
#include <signal.h>
#include <math.h>
#include <errno.h>
#include <stdint.h>

pthread_mutex_t serial_read, serial_write, data_check;
pthread_cond_t data_available;

pthread_mutex_t datavis_mutex;
pthread_cond_t datavis_drdy;

volatile sig_atomic_t done = 0;
volatile int first_run = 1;

void catch_sigint()
{
    done = 1;
    // broadcast conditions to unlock the threads in case they are locked
    pthread_cond_broadcast(&datavis_drdy);
    pthread_cond_broadcast(&data_available);
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
DECLARE_VECTOR(g_readB, unsigned short); // storage to put helmhotz values
unsigned short g_readFS[2];              // storage to put FS X and Y angles
unsigned short g_readCS[9];              // storage to put CS led brightnesses
unsigned char g_Fire;                    // magnetorquer command

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
    set_interface_attribs(fd, B230400, 0);
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
    long long charsleep = 70; // 30 for 2500000; // 75 for 230400
    while (!done)
    {
        // unsigned long long s = get_usec();
        unsigned char inbuf[30], obuf, tmp, val = 0;
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
    pthread_exit(NULL);
}

/* DataVis structures */
typedef struct
{
    uint8_t mode;               // ACS Mode
    uint64_t step;              // ACS Step
    DECLARE_VECTOR2(B, float);  // magnetic field
    DECLARE_VECTOR2(Bt, float); // B dot
    DECLARE_VECTOR2(W, float);  // Omega
    DECLARE_VECTOR2(S, float);  // Sun vector
} datavis_p;

#define PACK_SIZE sizeof(datavis_p)

typedef union {
    datavis_p data;
    unsigned char buf[sizeof(datavis_p)];
} data_packet;

data_packet global_p;

/* Starting ACS Thread and related functions */

#define SH_BUFFER_SIZE 64

#define DIPOLE_MOMENT 0.22                                             // A m^-2
#define DETUMBLE_TIME_STEP 100000                                      // 100 ms for full loop
#define MEASURE_TIME 20000                                             // 20 ms to measure
#define MAX_DETUMBLE_FIRING_TIME (DETUMBLE_TIME_STEP - MEASURE_TIME)   // Max allowed detumble fire time
#define MIN_DETUMBLE_FIRING_TIME 10000                                 // 10 ms
#define SUNPOINT_DUTY_CYCLE 20000                                      // 20 msec, in usec
#define COARSE_TIME_STEP DETUMBLE_TIME_STEP                            // 100 ms, in usec
DECLARE_BUFFER(g_W, float);                                            // omega global circular buffer
DECLARE_BUFFER(g_B, double);                                           // magnetic field global circular buffer
DECLARE_BUFFER(g_Bt, double);                                          // Bdot global circular buffer1
DECLARE_VECTOR(g_L_target, float);                                     // angular momentum target vector
DECLARE_VECTOR(g_W_target, float);                                     // angular velocity target vector
DECLARE_BUFFER(g_S, float);                                            // sun vector
float g_CSS[9];                                                        // current CSS lux values, in HITL this will be populated by TSL2561 code
float g_FSS[2];                                                        // current FSS angles, in rad; in HITL this will be populated by NANOSSOC A60 driver
int mag_index = -1, omega_index = -1, bdot_index = -1, sol_index = -1; // circular buffer indices, -1 indicates uninitiated buffer
int B_full = 0, Bdot_full = 0, W_full = 0, S_full = 0;                 // required to deal with the circular buffer problem
uint8_t g_night = 0;                                                   // night mode?
uint8_t g_acs_mode = 0;                                                // Detumble by default
uint8_t g_first_detumble = 1;                                          // first time detumble by default even at night
#define CSS_MIN_LUX_THRESHOLD 5000 * 0.5                               // 5000 lux is max sun, half of that is our threshold
unsigned long long acs_ct = 0;                                         // counts the number of ACS steps

typedef enum
{
    STATE_ACS_DETUMBLE, // Detumbling
    STATE_ACS_SUNPOINT, // Sunpointing
    STATE_ACS_NIGHT,    // Night
    STATE_ACS_READY,    // Do nothing
    STATE_XBAND_READY   // Ready to do X-Band things
} SH_ACS_MODES;

float MOI[3][3] = {{0.06467720404, 0, 0},
                   {0, 0.06474406267, 0},
                   {0, 0, 0.07921836177}};
float IMOI[3][3] = {{15.461398105297564, 0, 0},
                    {0, 15.461398105297564, 0},
                    {0, 0, 12.623336025344317}};

float bessel_coeff[SH_BUFFER_SIZE]; // coefficients for Bessel filter, declared as floating point

inline int factorial(int i)
{
    int result = 1;
    i = i + 1;
    while (--i > 0)
        result *= i;
    return result;
}
/*
 Calculates discrete Bessel filter coefficients for the given order and cutoff frequency.
 Stores the values in the supplied array with given size.
 bessel_coeff[0] = 1.
 Called once by main() at the beginning of execution.
 */
void calculateBessel(float arr[], int size, int order, float freq_cutoff)
{
    if (order > 5) // max 5th order
        order = 5;
    int *coeff = (int *)calloc(order + 1, sizeof(int)); // declare array to hold numeric coeff
    // evaluate coeff for order
    for (int i = 0; i < order + 1; i++)
    {
        coeff[i] = factorial(2 * order - i) / ((1 << (order - i)) * factorial(i) * factorial(order - i)); // https://en.wikipedia.org/wiki/Bessel_filter
    }
    // evaluate transfer function coeffs
    for (int j = 0; j < size; j++)
    {
        arr[j] = 0;         // initiate value to 0
        double pow_num = 1; //  (j/w_0)^0 is the start
        for (int i = 0; i < order + 1; i++)
        {
            arr[j] += coeff[i] * pow_num; // add the coeff
            pow_num *= j / freq_cutoff;   // multiply by (j/w_0) to avoid power function call
        }
        arr[j] = coeff[0] / arr[j]; // H(s) = T_n(0)/T_n(s/w_0)
    }
    if (coeff != NULL)
        free(coeff);
    else
        perror("[BESSEL] Coeff alloc failed, Bessel coeffs untrusted");
    return;
}

#define BESSEL_MIN_THRESHOLD 0.001 // randomly chosen minimum value for valid coefficient
#define BESSEL_FREQ_CUTOFF 5       // cutoff frequency 5 == 5*DETUMBLE_TIME_STEP seconds cycle == 2 Hz at 100ms loop speed

/*
 Returns the Bessel average of the data at the requested array index, double precision.
 */
double dfilterBessel(double arr[], int index)
{
    double val = 0;
    // index is guaranteed to be a number between 0...SH_BUFFER_SIZE by the readSensors() or getOmega() function.
    int coeff_index = 0;
    double coeff_sum = 0; // sum of the coefficients to calculate weighted average
    for (int i = index;;) // initiate the loop, break condition will be dealt with inside the loop
    {
        val += bessel_coeff[coeff_index] * arr[i]; // add weighted value
        coeff_sum += bessel_coeff[coeff_index];    // sum the weights to average with
        i--;                                       // read the previous element
        i = i < 0 ? SH_BUFFER_SIZE - 1 : i;        // allow for circular buffer issues
        coeff_index++;                             // use the next coefficient
        // looped around to the same element, coefficient crosses threshold OR (should never come to this) coeff_index overflows, break loop
        if (i == index || bessel_coeff[coeff_index] < BESSEL_MIN_THRESHOLD || coeff_index > SH_BUFFER_SIZE)
            break;
    }
    return val / coeff_sum;
}

/*
 Returns the Bessel average of the data at the requested array index, floating point.
 */
float ffilterBessel(float arr[], int index)
{
    float val = 0;
    // index is guaranteed to be a number between 0...SH_BUFFER_SIZE by the readSensors() or getOmega() function.
    int coeff_index = 0;
    float coeff_sum = 0;  // sum of the coefficients to calculate weighted average
    for (int i = index;;) // initiate the loop, break condition will be dealt with inside the loop
    {
        val += bessel_coeff[coeff_index] * arr[i]; // add weighted value
        coeff_sum += bessel_coeff[coeff_index];    // sum the weights to average with
        i--;                                       // read the previous element
        i = i < 0 ? SH_BUFFER_SIZE - 1 : i;        // allow for circular buffer issues
        coeff_index++;                             // use the next coefficient
        // looped around to the same element, coefficient crosses threshold OR (should never come to this) coeff_index overflows, break loop
        if (i == index || bessel_coeff[coeff_index] < BESSEL_MIN_THRESHOLD || coeff_index > SH_BUFFER_SIZE)
            break;
    }
    return val / coeff_sum;
}
// Apply Bessel filter over a buffer
#define APPLY_DBESSEL(name, index)                    \
    x_##name[index] = dfilterBessel(x_##name, index); \
    y_##name[index] = dfilterBessel(y_##name, index); \
    z_##name[index] = dfilterBessel(z_##name, index)

// Apply Bessel filter over a buffer
#define APPLY_FBESSEL(name, index)                    \
    x_##name[index] = ffilterBessel(x_##name, index); \
    y_##name[index] = ffilterBessel(y_##name, index); \
    z_##name[index] = ffilterBessel(z_##name, index)

// sort a1 and order a2 using the same order as a1.
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

// print individual bits, USE fflush() before and after
void print_bits(unsigned char octet)
{
    int z = 128, oct = octet;

    while (z > 0)
    {
        int wr;
        if (oct & z)
            wr = write(1, "1", 1);
        else
            wr = write(1, "0", 1);
        z >>= 1;
    }
}

// enable H bridge in the given direction using a vector input
#define HBRIDGE_ENABLE(name) \
    hbridge_enable(x_##name, y_##name, z_##name);
// enable H bridge in the given direction using a vector input
int hbridge_enable(int x, int y, int z)
{
    uint8_t val = 0x00;
    // Set up Z
    val |= z > 0 ? 0x01 : (z < 0 ? 0x02 : 0x00);
    val <<= 2;
    // printf("HBEnable: Z: 0b");
    // fflush(stdout);
    // print_bits(val);
    // printf(" ");
    // fflush(stdout);
    // Set up Y
    val |= y > 0 ? 0x01 : (y < 0 ? 0x02 : 0x00);
    val <<= 2;
    // printf("HBEnable: Y: 0b");
    // fflush(stdout);
    // print_bits(val);
    // printf(" ");
    // fflush(stdout);
    // Set up X
    val |= x > 0 ? 0x01 : (x < 0 ? 0x02 : 0x00);
    // printf("HBEnable: X: 0b");
    // fflush(stdout);
    // print_bits(val);
    // printf("\n");
    // fflush(stdout);
    pthread_mutex_lock(&serial_write);
    g_Fire = val;
    pthread_mutex_unlock(&serial_write);
    // printf("HBEnable: %d %d %d: 0x%x\n", x, y, z, g_Fire);
    return val;
}
// disable selected channel on the hbridge
int HBRIDGE_DISABLE(int i)
{
    int tmp = 0xff;
    tmp ^= 0x03 << 2 * i;
    // printf("HBDisable: tmp: 0b");
    // fflush(stdout);
    // print_bits(tmp);
    // fflush(stdout);
    pthread_mutex_lock(&serial_write);
    g_Fire &= tmp;
    pthread_mutex_unlock(&serial_write);
    // printf("HBDisable: 0b");
    // fflush(stdout);
    // print_bits(g_Fire);
    // printf("\n");
    // fflush(stdout);
    return tmp;
}
// get omega using magnetic field measurements (Bdot)
void getOmega(void)
{
    if (mag_index < 2 && B_full == 0) // not enough measurements
        return;
    // once we have measurements, we declare that we proceed
    if (omega_index == SH_BUFFER_SIZE - 1) // hit max, buffer full
        W_full = 1;
    omega_index = (1 + omega_index) % SH_BUFFER_SIZE;
    int8_t m0, m1;
    m1 = bdot_index;
    m0 = (bdot_index - 1) < 0 ? SH_BUFFER_SIZE - bdot_index - 1 : bdot_index - 1;
    float freq;
    freq = 1e6 / DETUMBLE_TIME_STEP;                     // time units!
    CROSS_PRODUCT(g_W[omega_index], g_Bt[m1], g_Bt[m0]); // apply cross product
    float norm2 = NORM2(g_Bt[m0]);
    VECTOR_MIXED(g_W[omega_index], g_W[omega_index], freq / norm2, *); // omega = (B_t dot x B_t-dt dot)*freq/Norm(B_t dot)
    // Apply correction // There is fast runaway with this on
    // DECLARE_VECTOR(omega_corr0, float);                            // declare temporary space for correction vector
    // MATVECMUL(omega_corr0, MOI, g_W[m1]);                          // MOI X w[t-1]
    // DECLARE_VECTOR(omega_corr1, float);                            // declare temporary space for correction vector
    // CROSS_PRODUCT(omega_corr1, g_W[m1], omega_corr0);              // store into temp 1
    // MATVECMUL(omega_corr1, IMOI, omega_corr0);                     // store back into temp 0
    // VECTOR_MIXED(omega_corr1, omega_corr1, -freq, *);              // omega_corr = freq*(MOI-1)*(-w[t-1] X MOI*w[t-1])
    // VECTOR_OP(g_W[omega_index], g_W[omega_index], omega_corr1, +); // add the correction term to omega
    APPLY_FBESSEL(g_W, omega_index); // Bessel filter of order 3
    return;
}
// get sun vector using lux measurements
void getSVec(void)
{
    if (sol_index == SH_BUFFER_SIZE - 1) // hit max, buffer full
        S_full = 1;
    sol_index = (sol_index + 1) % SH_BUFFER_SIZE;

    float fsx = g_FSS[0];
    float fsy = g_FSS[1];
#ifndef M_PI
#define M_PI 3.1415
#endif
    // check if FSS results are acceptable
    // if they are, use that to calculate the sun vector
    if (fabsf(fsx) <= 60. / (180 * M_PI) && fabsf(fsy) <= 60. / (180 * M_PI)) // angle inside FOV
    {
        x_g_S[sol_index] = tan(fsx); // Consult https://www.cubesatshop.com/wp-content/uploads/2016/06/nanoSSOC-A60-Technical-Specifications.pdf, section 4
        y_g_S[sol_index] = tan(fsy);
        z_g_S[sol_index] = 1;
        NORMALIZE(g_S[sol_index], g_S[sol_index]);
        return;
    }

    // get average -Z luminosity from 4 sensors
    float znavg = 0;
    for (int i = 5; i < 9; i++)
        znavg += g_CSS[i];
    znavg *= 0.250f;

    x_g_S[sol_index] = g_CSS[0] - g_CSS[1]; // +x - -x
    y_g_S[sol_index] = g_CSS[2] - g_CSS[3]; // +x - -x
    z_g_S[sol_index] = g_CSS[4] - znavg;    // +z - avg(-z)

    float css_mag = NORM(g_S[sol_index]); // norm of the CSS lux values

    if (css_mag < CSS_MIN_LUX_THRESHOLD) // night time logic
    {
        g_night = 1;
        VECTOR_CLEAR(g_S[sol_index]); // return 0 solar vector
    }
    else
    {
        g_night = 0;
        NORMALIZE(g_S[sol_index], g_S[sol_index]); // return normalized sun vector
    }
    // printf("[sunvec %d] %0.3f %0.3f | %0.3f %0.3f %0.3f\n", sol_index, fsx, fsy, x_g_S[sol_index], y_g_S[sol_index], z_g_S[sol_index]);
    return;
}
// read all sensors (right now only magnetic)
int readSensors(void)
{
    // read magfield, CSS, FSS
    int status = 1;
    if (mag_index == SH_BUFFER_SIZE - 1) // hit max, buffer full
        B_full = 1;
    mag_index = (mag_index + 1) % SH_BUFFER_SIZE;
    VECTOR_CLEAR(g_B[mag_index]); // clear the current B
    pthread_mutex_lock(&serial_read);
    VECTOR_OP(g_B[mag_index], g_B[mag_index], g_readB, +); // load B - equivalent reading from sensor
    for (int i = 0; i < 9; i++)                            // load CSS
        g_CSS[i] = (g_readCS[i] * 5000.0) / 0x0fff;
    g_FSS[0] = (g_readFS[0] * M_PI) / 0xffff - M_PI / 2; // load FSS angle 0
    g_FSS[1] = (g_readFS[1] * M_PI) / 0xffff - M_PI / 2; // load FSS angle 1
    // printf("[read]%04x %04x %04x %04x %04x %04x %04x %04x %04x %04x %04x\n", g_readFS[0], g_readFS[1], g_readCS[0], g_readCS[1], g_readCS[2], g_readCS[3], g_readCS[4], g_readCS[5], g_readCS[6], g_readCS[7], g_readCS[8]);
    pthread_mutex_unlock(&serial_read);
// convert B to proper units
#define B_RANGE 32767
    VECTOR_MIXED(g_B[mag_index], g_B[mag_index], B_RANGE, -);
    VECTOR_MIXED(g_B[mag_index], g_B[mag_index], 4e-4 * 1e7 / B_RANGE, *); // in milliGauss to have precision
    // printf("readSensors: Bx: %f By: %f Bz: %f\n", x_g_B[mag_index], y_g_B[mag_index], z_g_B[mag_index]);
    // put values into g_Bx, g_By and g_Bz at [mag_index] and takes 18 ms to do so (implemented using sleep)
    if (mag_index < 1 && B_full == 0)
        return status;
    // if we have > 1 values, calculate Bdot
    if (bdot_index == SH_BUFFER_SIZE - 1) // hit max, buffer full
        B_full = 1;
    bdot_index = (bdot_index + 1) % SH_BUFFER_SIZE;
    int8_t m0, m1;
    m1 = mag_index;
    m0 = (mag_index - 1) < 0 ? SH_BUFFER_SIZE - mag_index - 1 : mag_index - 1;
    double freq = 1e6 / (DETUMBLE_TIME_STEP * 1.0);
    VECTOR_OP(g_Bt[bdot_index], g_B[m1], g_B[m0], -);
    VECTOR_MIXED(g_Bt[bdot_index], g_Bt[bdot_index], freq, *);
    // printf("readSensors: m0: %d m1: %d Btx: %f Bty: %f Btz: %f\n", m0, m1, x_g_Bt[bdot_index], y_g_Bt[bdot_index], z_g_Bt[bdot_index]);
    getOmega();
    getSVec();
    return status;
}

#define OMEGA_TARGET_LEEWAY z_g_W_target * 0.1 // 10% leeway in the value of omega_z
#define MIN_SOL_ANGLE 4                        // minimum solar angle for sunpointing to be a success
#define MIN_DETUMBLE_ANGLE 4                   // minimum angle for detumble to be a success

// check if the program should transition from one state to another
void checkTransition(void)
{
    if (!W_full) // not enough data to take a decision
        return;
    if (!S_full) // not enough data to take a decision
        return;
    DECLARE_VECTOR(avgOmega, float);                // declare buffer to contain average value of omega
    FAVERAGE_BUFFER(avgOmega, g_W, SH_BUFFER_SIZE); // calculate time average of omega over buffer
    DECLARE_VECTOR(avgSun, float);                  // declare buffer to contain avg sun vector
    VECTOR_MIXED(avgSun, g_S[sol_index], 0, +);     // current sun angle

    DECLARE_VECTOR(body, float);                                     // Body frame vector oriented along Z axis
    z_body = 1;                                                      // Body frame vector is Z
    float W_target_diff = z_g_W_target - z_avgOmega;                 // difference of omega_z
    NORMALIZE(avgOmega, avgOmega);                                   // Normalize avg omega to get omega hat
    float z_w_ang = 180. * acos(DOT_PRODUCT(avgOmega, body)) / M_PI; // average omega angle in degrees

    float z_S_ang = 180. * acos(DOT_PRODUCT(avgSun, body)) / M_PI; // average Sun angle in degrees
    printf("[state %d] dW = %.3f, Ang = %.3f, DP = %.3f, |SUN| = %.3f\n", g_acs_mode, W_target_diff, z_w_ang, DOT_PRODUCT(avgOmega, body), NORM(avgSun));
    switch (g_acs_mode)
    {
    case STATE_ACS_DETUMBLE:
    {
        // If detumble criterion is met, go to Sunpointing mode
        if (fabsf(z_w_ang) < MIN_DETUMBLE_ANGLE && fabsf(W_target_diff) < OMEGA_TARGET_LEEWAY)
        {
            g_acs_mode = STATE_ACS_SUNPOINT;
            g_first_detumble = 0; // when system detumbles for the first time, unsets this variable
        }
        if (!g_first_detumble) // if this var is unset, the system does not do anything at night
        {
            if (NORM(avgSun) < 1)
            {
                printf("Here!");
                g_acs_mode = STATE_ACS_NIGHT;
            }      
        }
        break;
    }

    case STATE_ACS_SUNPOINT:
    {
        // If detumble criterion is not held, fall back to detumbling
        if (fabsf(z_w_ang) > MIN_DETUMBLE_ANGLE || fabsf(W_target_diff) < OMEGA_TARGET_LEEWAY)
        {
            g_acs_mode = STATE_ACS_DETUMBLE;
            break;
        }
        // if it is night, fall back to night mode. Should take SH_BUFFER_SIZE * DETUMBLE_TIME_STEP seconds for the actual state change to occur
        if (NORM(avgSun) < 1)
        {
            g_acs_mode = STATE_ACS_NIGHT;
            break;
        }
        // if the satellite is detumbled, it is not night and the sun angle is less than 4 deg, declare ACS is ready
        if (fabsf(z_S_ang) < MIN_SOL_ANGLE)
        {
            g_acs_mode = STATE_ACS_READY;
        }
        break;
    }

    case STATE_ACS_NIGHT:
    {
        if (NORM(avgSun) > CSS_MIN_LUX_THRESHOLD)
        {
            if (fabsf(z_w_ang) > MIN_DETUMBLE_ANGLE || fabsf(W_target_diff) < OMEGA_TARGET_LEEWAY)
            {
                g_acs_mode = STATE_ACS_DETUMBLE;
                break;
            }
            if (fabsf(z_S_ang) < MIN_SOL_ANGLE)
            {
                g_acs_mode = STATE_ACS_READY;
            }
            break;
        }
        else
        {
            break;
        }
    }

    default:
        break;
    }
}
// This function executes the detumble action
inline void detumbleAction(void)
{
    if (omega_index < 0)
    {
        usleep(DETUMBLE_TIME_STEP - MEASURE_TIME);
    }
    else
    {
        DECLARE_VECTOR(currL, double);           // vector for current angular momentum
        MATVECMUL(currL, MOI, g_W[omega_index]); // calculate current angular momentum
        VECTOR_OP(currL, g_L_target, currL, -);  // calculate angular momentum error
        DECLARE_VECTOR(currLNorm, float);
        NORMALIZE(currLNorm, currL); // normalize the angular momentum error vector
        // printf("Norm L error: %lf %lf %lf\n", x_currLNorm, y_currLNorm, z_currLNorm);
        DECLARE_VECTOR(currB, double);    // current normalized magnetic field TMP
        NORMALIZE(currB, g_B[mag_index]); // normalize B
        // printf("Norm B: %lf %lf %lf\n", x_currB, y_currB, z_currB);
        DECLARE_VECTOR(firingDir, float);           // firing direction vector
        CROSS_PRODUCT(firingDir, currB, currLNorm); // calculate firing direction
        // printf("Firing Dir: %lf %lf %lf\n", x_firingDir, y_firingDir, z_firingDir);
        // printf("Abs Firing Dir: %lf %lf %lf\n", x_firingDir*(x_firingDir < 0 ? -1 : 1), y_firingDir*(y_firingDir < 0 ? -1 : 1), z_firingDir*(z_firingDir < 0 ? -1 : 1));
        int8_t x_fire = (x_firingDir < 0 ? -1 : 1);                        // if > 0.01, then fire in the direction of input
        int8_t y_fire = (y_firingDir < 0 ? -1 : 1);                        // * (abs(y_firingDir) > 0.01 ? 1 : 0); // if > 0.01, then fire in the direction of input
        int8_t z_fire = (z_firingDir < 0 ? -1 : 1);                        // * (abs(z_firingDir) > 0.01 ? 1 : 0); // if > 0.01, then fire in the direction of input
        x_fire *= x_firingDir * (x_firingDir < 0 ? -1 : 1) > 0.01 ? 1 : 0; // if > 0.01, then fire in the direction of input
        y_fire *= y_firingDir * (y_firingDir < 0 ? -1 : 1) > 0.01 ? 1 : 0; // if > 0.01, then fire in the direction of input
        z_fire *= z_firingDir * (z_firingDir < 0 ? -1 : 1) > 0.01 ? 1 : 0; // if > 0.01, then fire in the direction of input
        // printf("Fire: %d %d %d\n", x_fire, y_fire, z_fire);
        DECLARE_VECTOR(currDipole, float);
        VECTOR_MIXED(currDipole, fire, DIPOLE_MOMENT * 1e-7, *); // calculate dipole moment, account for B in milliGauss
        // printf("Dipole: %f %f %f\n", x_currDipole, y_currDipole, z_currDipole );
        DECLARE_VECTOR(currTorque, float);
        CROSS_PRODUCT(currTorque, currDipole, g_B[mag_index]); // calculate current torque
        // printf("Torque: %f %f %f\n", x_currTorque, y_currTorque, z_currTorque );
        DECLARE_VECTOR(firingTime, float);           // initially gives firing time in seconds
        VECTOR_OP(firingTime, currL, currTorque, /); // calculate firing time based on current torque
        // printf("Firing time (s): %f %f %f\n", x_firingTime, y_firingTime, z_firingTime );
        VECTOR_MIXED(firingTime, firingTime, 1000000, *); // convert firing time to usec
        DECLARE_VECTOR(firingCmd, int);                   // integer firing time in usec
        x_firingCmd = x_firingTime > MAX_DETUMBLE_FIRING_TIME ? MAX_DETUMBLE_FIRING_TIME : (x_firingTime < MIN_DETUMBLE_FIRING_TIME ? 0 : (int)x_firingTime);
        y_firingCmd = y_firingTime > MAX_DETUMBLE_FIRING_TIME ? MAX_DETUMBLE_FIRING_TIME : (y_firingTime < MIN_DETUMBLE_FIRING_TIME ? 0 : (int)y_firingTime);
        z_firingCmd = z_firingTime > MAX_DETUMBLE_FIRING_TIME ? MAX_DETUMBLE_FIRING_TIME : (z_firingTime < MIN_DETUMBLE_FIRING_TIME ? 0 : (int)z_firingTime);
        // printf("Firing Time: %d %d %d\n", x_firingTime, y_firingTime, z_firingTime);
        int firingOrder[3] = {0, 1, 2}, firingTime[3]; // 0 == x, 1 == y, 2 == z
        firingTime[0] = x_firingCmd;
        firingTime[1] = y_firingCmd;
        firingTime[2] = z_firingCmd;
        // printf("Firing Time: %d %d %d\n", firingTime[0], firingTime[1], firingTime[2]);
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
}
// This function executes the sunpointing action
inline void sunpointAction(void)
{
    if (sol_index < 0)
    {
        usleep(DETUMBLE_TIME_STEP - MEASURE_TIME);
    }
    else
    {
        DECLARE_VECTOR(currB, float);
        DECLARE_VECTOR(currBNorm, float);
        DECLARE_VECTOR(currL, float);
        //DECLARE_VECTOR(currLNorm, float) ;
        DECLARE_VECTOR(currS, float);
        DECLARE_VECTOR(currSNorm, float);
        VECTOR_OP(currB, currB, g_B[mag_index], +); // get current magfield
        NORMALIZE(currBNorm, currB);                // normalize current magfield
        MATVECMUL(currL, MOI, g_W[omega_index]);    // calculate current angular momentum
        VECTOR_OP(currS, currS, g_S[sol_index], +); // get current sunvector
        NORMALIZE(currSNorm, currS);                // normalize sun vector
        // calculate S_B_hat
        DECLARE_VECTOR(SBHat, float);
        float SdotB = DOT_PRODUCT(currSNorm, currBNorm);
        VECTOR_MIXED(SBHat, currBNorm, SdotB, *);
        VECTOR_OP(SBHat, currSNorm, SBHat, -);
        NORMALIZE(SBHat, SBHat);
        // calculate L_B_hat
        DECLARE_VECTOR(LBHat, float);
        float LdotB = DOT_PRODUCT(currL, currBNorm);
        VECTOR_MIXED(LBHat, currBNorm, LdotB, *);
        VECTOR_OP(LBHat, currL, LBHat, -);
        NORMALIZE(LBHat, LBHat);
        // cross product the two vectors
        DECLARE_VECTOR(SxBxL, float);
        CROSS_PRODUCT(SxBxL, SBHat, LBHat);
        NORMALIZE(SxBxL, SxBxL);

        int time_on = (int)(DOT_PRODUCT(SxBxL, currBNorm) * SUNPOINT_DUTY_CYCLE); // essentially a duty cycle measure
        time_on = time_on > SUNPOINT_DUTY_CYCLE ? SUNPOINT_DUTY_CYCLE : time_on;  // safety measure
        int time_off = SUNPOINT_DUTY_CYCLE - time_on;
        int FiringTime = COARSE_TIME_STEP - MEASURE_TIME; // time allowed to fire
        DECLARE_VECTOR(fire, int);
        z_fire = 1; // z direction is the only direction of fire
        while (FiringTime > 0)
        {
            HBRIDGE_ENABLE(fire);
            usleep(time_on);
            HBRIDGE_DISABLE(3); // 3 == executes default, turns off ALL hbridges (safety)
            usleep(time_off);
            FiringTime -= SUNPOINT_DUTY_CYCLE;
        }
        usleep(FiringTime + SUNPOINT_DUTY_CYCLE); // sleep for the remainder of the time
    }
}
// measure thread execution time
unsigned long long t_acs = 0;
// detumble thread, will become master acs thread
void *acs_detumble(void *id)
{
    while (!done)
    {
        // wait till there is available data on serial
        if (first_run)
        {
            printf("ACS: Waiting for release...\n");
            first_run = 0;
            // wait till there is available data on serial
            pthread_cond_wait(&data_available, &data_check);
        }
        unsigned long long s = get_usec();
        readSensors();
        //time_t now ; time(&now);
        if (omega_index >= 0)
        {
            printf("[%.3f ms][%llu ms] ACS step: %llu | Wx = %f Wy = %f Wz = %f\n", comm_time / 1000.0, (s - t_acs) / 1000, acs_ct++, x_g_W[omega_index], y_g_W[omega_index], z_g_W[omega_index]);
            // Update datavis variables [DO NOT TOUCH]
            global_p.data.step = acs_ct;
            global_p.data.mode = g_acs_mode;
            global_p.data.x_B = x_g_B[mag_index];
            global_p.data.y_B = y_g_B[mag_index];
            global_p.data.z_B = z_g_B[mag_index];
            global_p.data.x_Bt = x_g_Bt[bdot_index];
            global_p.data.y_Bt = y_g_Bt[bdot_index];
            global_p.data.z_Bt = z_g_Bt[bdot_index];
            global_p.data.x_W = x_g_W[omega_index];
            global_p.data.y_W = y_g_W[omega_index];
            global_p.data.z_W = z_g_W[omega_index];
            global_p.data.x_S = x_g_S[sol_index];
            global_p.data.y_S = y_g_S[sol_index];
            global_p.data.z_S = z_g_S[sol_index];
            // wake up datavis thread [DO NOT TOUCH]
            pthread_cond_broadcast(&datavis_drdy);
        }
        //    printf("%s ACS step: %llu | Wx = %f Wy = %f Wz = %f\n", ctime(&now), acs_ct++ , x_g_W[omega_index], y_g_W[omega_index], z_g_W[omega_index]);
        t_acs = s;
        checkTransition(); // check if the system should transition from one state to another
        unsigned long long e = get_usec();
        usleep(MEASURE_TIME - e + s); // sleep for total 20 ms with read
        switch (g_acs_mode)
        {
        case STATE_ACS_DETUMBLE:
            detumbleAction();
            break;

        case STATE_ACS_SUNPOINT:
            sunpointAction();
            break;
        default:
            usleep(DETUMBLE_TIME_STEP - MEASURE_TIME);
            break;
        }
    }
    pthread_exit(NULL);
}

/* Data visualization thread */
#include <sys/socket.h>
#include <arpa/inet.h>

#ifndef PORT
#define PORT 12376
#endif

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
        pthread_cond_wait(&datavis_drdy, &datavis_mutex);
        if ((new_socket = accept(server_fd, (sk_sockaddr *)&address, (socklen_t *)&addrlen)) < 0)
        {
            // perror("accept");
            // cerr << "DataVis: Accept from socket error!" <<endl ;
        }
        ssize_t numsent = send(new_socket, &global_p.buf, PACK_SIZE, 0);
        //cerr << "DataVis: Size of sent data: " << PACK_SIZE << endl ;
        if (numsent > 0 && numsent != PACK_SIZE)
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
/* Data visualization server thread */

int main(void)
{
    // handle sigint
    struct sigaction saction;
    saction.sa_handler = &catch_sigint;
    sigaction(SIGINT, &saction, NULL);

    z_g_W_target = 1;                       // 1 rad s^-1
    MATVECMUL(g_L_target, MOI, g_W_target); // calculate target angular momentum
    calculateBessel(bessel_coeff, SH_BUFFER_SIZE, 3, BESSEL_FREQ_CUTOFF);

    int rc0, rc1, rc2;
    pthread_t thread0, thread1, thread2;
    pthread_attr_t attr;
    void *status;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
    rc0 = pthread_create(&thread0, &attr, acs_detumble, (void *)0);
    if (rc0)
    {
        printf("Main: Error: Unable to create ACS thread %d: Errno %d\n", rc0, errno);
        exit(-1);
    }
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

    rc0 = pthread_join(thread0, &status);
    if (rc0)
    {
        printf("Main: Error: Unable to join ACS thread %d: Errno %d\n", rc0, errno);
    }

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
