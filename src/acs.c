/**
 * @file acs.c
 * @author Sunip K. Mukherjee (sunipkmukherjee@gmail.com)
 * @brief Attitude Control System related functions
 * @version 0.2
 * @date 2020-07-01
 * 
 * @copyright Copyright (c) 2020
 * 
 */
#include <macros.h>           // Macro definitions and functions specific to SHFlight
#include <acs.h>              // prototypes for thread-local functions and variables only
#include <main.h>             // loop control
#include <bessel.h>           // bessel filter prototypes
#include <sitl_comm_extern.h> // Variables shared with serial communication thread
#include <datavis_extern.h>   // variables shared with DataVis thread
#include <ads1115.h>
#include <lsm9ds1.h>
#include <ncv7708.h>
#include <tsl2561.h>
#include <tca9458a.h>
#include <math.h>
#include <stdlib.h>
#include <unistd.h>

/**
 * @brief This is color indicator for printf statements in ACS, for use in debug only."
 */
#define RST "\x1B[0m"  ///< reset to default
#define BLK "\x1B[30m" ///< black
#define RED "\x1B[31m" ///< red
#define GRN "\x1B[32m" ///< green
#define YLW "\x1B[33m" ///< yellow
#define BLU "\x1B[34m" ///< blue
#define MGT "\x1B[35m" ///< magenta
#define CYN "\x1B[36m" ///< cyan
#define LGY "\x1B[37m" ///< light gray
#define DGY "\x1B[90m" ///< dark gray
#define LRD "\x1B[91m" ///< light red
#define LGR "\x1B[92m" ///< light green
#define LYW "\x1B[93m" ///< light yellow
#define LBU "\x1B[94m" ///< light blue
#define LMT "\x1B[95m" ///< light magenta
#define LCY "\x1B[96m" ///< light cyan
#define WHT "\x1B[97m" ///< white

/* Variable allocation for ACS */
/**
 * @brief Condition variable to synchronize ACS and Serial thread in SITL.
 * 
 */
pthread_cond_t data_available;
/**
 * @brief Mutex for locking on data_available.
 */ 
pthread_mutex_t data_check;

/**
 * @brief This variable is unset by the ACS thread at first execution.
 * 
 */
volatile int first_run = 1;

// SITL
/**
 * @brief Declares vector to store magnetic field reading from serial.
 * 
 */
DECLARE_VECTOR(g_readB, unsigned short); // storage to put helmhotz values
/**
 * @brief Fine sun sensor angles read over serial.
 * 
 */
unsigned short g_readFS[2]; // storage to put FS X and Y angles
/**
 * @brief Coarse sun sensor lux values read over serial.
 * 
 */
unsigned short g_readCS[9]; // storage to put CS led brightnesses
/**
 * @brief Magnetorquer command, format: 0b00ZZYYXX, 00 indicates not fired, 01 indicates fire in positive dir, 10 indicates fire in negative dir.
 * 
 */
unsigned char g_Fire; // magnetorquer command

// HITL
/**
 * @brief Magnetometer device struct.
 * 
 */
lsm9ds1 *mag; // magnetometer
/**
 * @brief H-Bridge device struct.
 * 
 */
ncv7708 *hbridge; // h-bridge
/**
 * @brief I2C Mux device struct.
 * 
 */
tca9458a *mux; // I2C mux
/**
 * @brief Array of coarse sun sensor device struct.
 * 
 */
tsl2561 **css; // coarse sun sensors
/**
 * @brief I2C ADC struct for fine sun sensor.
 * 
 */
ads1115 *adc; // analog to digital converters
              // SITL
/**
 * @brief Creates buffer for \f$\vec{\omega}\f$.
 * 
 */
DECLARE_BUFFER(g_W, float); // omega global circular buffer
/**
 * @brief Creates buffer for \f$\vec{B}\f$.
 * 
 */
DECLARE_BUFFER(g_B, double); // magnetic field global circular buffer
/**
 * @brief Creates buffer for \f$\vec{\dot{B}}\f$.
 * 
 */
DECLARE_BUFFER(g_Bt, double); // Bdot global circular buffer1
/**
 * @brief Creates vector for target angular momentum.
 * 
 */
DECLARE_VECTOR(g_L_target, float); // angular momentum target vector
/**
 * @brief Creates vector for target angular speed.
 * 
 */
DECLARE_VECTOR(g_W_target, float); // angular velocity target vector
/**
 * @brief Creates buffer for sun vector.
 * 
 */
DECLARE_BUFFER(g_S, float); // sun vector
/**
 * @brief Storage for current coarse sun sensor lux measurements.
 * 
 */
float g_CSS[9]; // current CSS lux values, in HITL this will be populated by TSL2561 code
/**
 * @brief Storage for current fine sun sensor angle measurements.
 * 
 */
float g_FSS[2]; // current FSS angles, in rad; in HITL this will be populated by NANOSSOC A60 driver
/**
 * @brief Current index of the \f$\vec{B}\f$ circular buffer.
 * 
 */
int mag_index = -1;
/**
 * @brief Current index of the \f$\vec{\omega}\f$ circular buffer.
 * 
 */
int omega_index = -1;
/**
 * @brief Current index of the \f$\vec{\dot{B}}\f$ circular buffer.
 * 
 */
int bdot_index = -1;
/**
 * @brief Current index of the sun vector circular buffer.
 * 
 */
int sol_index = -1; // circular buffer indices, -1 indicates uninitiated buffer
/**
 * @brief Indicates if the \f$\vec{B}\f$ circular buffer is full.
 * 
 */
int B_full = 0;
/**
 * @brief Indicates if the \f$\vec{\dot{B}}\f$ circular buffer is full.
 * 
 */
int Bdot_full = 0;
/**
 * @brief Indicates if the \f$\vec{\omega}\f$ circular buffer is full.
 * 
 */
int W_full = 0;
/**
 * @brief Indicates if the sun vector circular buffer is full.
 * 
 */
int S_full = 0; // required to deal with the circular buffer problem
/**
 * @brief This variable is set by checkTransition() if the satellite does not detect the sun.
 * 
 */
uint8_t g_night = 0; // night mode?
/**
 * @brief This variable contains the current state of the flight system.
 * 
 */
uint8_t g_acs_mode = 0; // Detumble by default
/**
 * @brief This variable is unset when the system is detumbled for the first time after a power cycle.
 * 
 */
uint8_t g_first_detumble = 1; // first time detumble by default even at night
/**
 * @brief Counts the number of cycles on the ACS thread.
 * 
 */
unsigned long long acs_ct = 0; // counts the number of ACS steps
/**
 * @brief Moment of inertia of the satellite (SI).
 * 
 */
float MOI[3][3] = {{0.06467720404, 0, 0},
                   {0, 0.06474406267, 0},
                   {0, 0, 0.07921836177}};
/**
 * @brief Inverse of the moment of inertia of the satellite (SI).
 * 
 */
float IMOI[3][3] = {{15.461398105297564, 0, 0},
                    {0, 15.461398105297564, 0},
                    {0, 0, 12.623336025344317}};
/**
 * @brief Current timestamp after readSensors() in ACS thread, used to keep track of time taken by ACS loop.
 * 
 */
unsigned long long g_t_acs;

#ifdef ACS_DATALOG
/**
 * @brief ACS Datalog file pointer.
 * 
 */
FILE *acs_datalog;
#endif
/*******************************/

/**
 * @brief This function executes the detumble algorithm.
 * 
 * The detumble algorithm calculates the direction and time
 * for which the magnetorquers fire. The direction is determined
 * by first calculating the vector \f$\hat{B}\times\hat{L_0 - L}\f$,
 * which is a unit vector, and then checking which of the components have
 * a magnitude greater than 0.01. A component with magnitude greater than
 * 0.01 indicates that torquer can be fired, in the direction indicated by
 * the sign of the component. Further, the torque that is generated by
 * the firing decision is estimated for the current value of the magnetic
 * field by calculating \f$\vec{\tau}=\vec{\mu}\times\vec{B}\f$, where
 * \f$\vec{mu}\f$ is calculated by multiplying the firing direction vector
 * with the dipole moment of the magnetorquers (0.21 A\f$\cdot\f$m\f$^2\f$).
 * Then for each direction, the firing time is estimated by
 * \f$ t_i = \frac{\Delta L_i}{\tau_i}\f$. The torquer in any direction is fired
 * only if the firing time is greater than 5 ms, and any torquer is fired for
 * at most the allowed firing time. At the end of the action, all torquers
 * are turned off for the next magnetic field measurement.
 * 
 * 
 */
static inline void detumbleAction();

/**
 * @brief This function executes the sunpointing algorithm.
 * 
 * The sunpointing algoritm calculates the duty cycle of the 
 * Z-magnetorquer firing. The duty cycle is determined by calculating
 * the vector \f$(\hat{S}(\hat{S}\cdot\hat{B}))\times((\hat{L}(\hat{L}\cdot\hat{B}))\f$.
 * The Z component of this vector upon normalization specifies the duty
 * cycle. However, due to lowering of efficiency as the spacecraft aligns 
 * with the sun, the gain is increased.
 */
static inline void sunpointAction();

#ifndef SITL
int hbridge_enable(int x, int y, int z)
{
    // Set up X
    hbridge->pack->hbcnf1 = x > 0 ? 1 : 0;
    hbridge->pack->hbcnf2 = x < 0 ? 1 : 0;
    // Set up Y
    hbridge->pack->hbcnf3 = y > 0 ? 1 : 0;
    hbridge->pack->hbcnf4 = y < 0 ? 1 : 0;
    // Set up Z
    hbridge->pack->hbcnf5 = z > 0 ? 1 : 0;
    hbridge->pack->hbcnf6 = z < 0 ? 1 : 0;
    return ncv7708_xfer(hbridge);
}

int HBRIDGE_DISABLE(int num)
{
    switch (num)
    {
    case 0: // X axis
        hbridge->pack->hbcnf1 = 0;
        hbridge->pack->hbcnf2 = 0;
        break;

    case 1: // Y axis
        hbridge->pack->hbcnf3 = 0;
        hbridge->pack->hbcnf4 = 0;
        break;

    case 2: // Z axis
        hbridge->pack->hbcnf5 = 0;
        hbridge->pack->hbcnf6 = 0;
        break;

    default: // disable all
        hbridge->pack->hbcnf1 = 0;
        hbridge->pack->hbcnf2 = 0;
        hbridge->pack->hbcnf3 = 0;
        hbridge->pack->hbcnf4 = 0;
        hbridge->pack->hbcnf5 = 0;
        hbridge->pack->hbcnf6 = 0;
        break;
    }
    return ncv7708_xfer(hbridge);
}
#else
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
#endif // SITL

void getOmega(void)
{
    if (mag_index < 2 && B_full == 0) // not enough measurements
        return;
    // once we have measurements, we declare that we proceed
    if (omega_index == SH_BUFFER_SIZE - 1) // hit max, buffer full
        W_full = 1;
    omega_index = (1 + omega_index) % SH_BUFFER_SIZE;                             // calculate new index in the circular buffer
    int8_t m0, m1;                                                                // temporary addresses
    m1 = bdot_index;                                                              // current address
    m0 = (bdot_index - 1) < 0 ? SH_BUFFER_SIZE - bdot_index - 1 : bdot_index - 1; // previous address, wrapped around the circular buffer
    float freq;
    freq = 1e6 / DETUMBLE_TIME_STEP;                     // time units!
    CROSS_PRODUCT(g_W[omega_index], g_Bt[m1], g_Bt[m0]); // apply cross product
    float norm2 = NORM2(g_Bt[m0]);
    VECTOR_MIXED(g_W[omega_index], g_W[omega_index], freq / norm2, *); // omega = (B_t dot x B_t-dt dot)*freq/Norm2(B_t dot)
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

void getSVec(void)
{
    if (sol_index == SH_BUFFER_SIZE - 1) // hit max, buffer full
        S_full = 1;
    sol_index = (sol_index + 1) % SH_BUFFER_SIZE;
#ifdef SITL
    // SITL expects radians input
    float fsx = 180 / M_PI * g_FSS[0];
    float fsy = 180 / M_PI * g_FSS[1];
#else
    // hardware reads degrees
    float fsx = g_FSS[0];
    float fsy = g_FSS[1];
#endif // SITL
#ifndef M_PI
/**
 * @brief Approximate definition of Pi in case M_PI is not included from math.h
 */ 
#define M_PI 3.1415
#endif
    // check if FSS results are acceptable
    // if they are, use that to calculate the sun vector
    // printf("[FSS] %.3f %.3f\n", fsx * 180. / M_PI, fsy * 180. / M_PI);
    if (fabsf(fsx) <= 60 && fabsf(fsy) <= 60) // angle inside FOV (FOV -> 60°, half angle 30°)
    {
#ifdef ACS_PRINT
        printf("[" GRN "FSS" RST "]");
#endif                                            // ACS_PRINT
        x_g_S[sol_index] = tan(fsx * M_PI / 180); // Consult https://www.cubesatshop.com/wp-content/uploads/2016/06/nanoSSOC-A60-Technical-Specifications.pdf, section 4
        y_g_S[sol_index] = tan(fsy * M_PI / 180);
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
#ifdef ACS_PRINT
        printf("[" RED "FSS" RST "]");
#endif // ACS_PRINT
    }
    else
    {
        g_night = 0;
        NORMALIZE(g_S[sol_index], g_S[sol_index]); // return normalized sun vector
#ifdef ACS_PRINT
        printf("[" YLW "FSS" RST "]");
#endif // ACS_PRINT
    }
    // printf("[sunvec %d] %0.3f %0.3f | %0.3f %0.3f %0.3f\n", sol_index, fsx, fsy, x_g_S[sol_index], y_g_S[sol_index], z_g_S[sol_index]);
    return;
}

int readSensors(void)
{
    // read magfield, CSS, FSS
    // printf("In readSensors()...\n");
    int status = 1;
    if (mag_index == SH_BUFFER_SIZE - 1) // hit max, buffer full
        B_full = 1;
    mag_index = (mag_index + 1) % SH_BUFFER_SIZE;
    VECTOR_CLEAR(g_B[mag_index]); // clear the current B
#ifdef SITL
    pthread_mutex_lock(&serial_read);
    VECTOR_OP(g_B[mag_index], g_B[mag_index], g_readB, +); // load B - equivalent reading from sensor
    for (int i = 0; i < 9; i++)                            // load CSS
        g_CSS[i] = (g_readCS[i] * 5000.0) / 0x0fff;
    g_FSS[0] = ((g_readFS[0] * M_PI) / 65535.0) - (M_PI / 2); // load FSS angle 0
    g_FSS[1] = ((g_readFS[1] * M_PI) / 65535.0) - (M_PI / 2); // load FSS angle 1
    // printf("[read]%04x %04x %04x %04x %04x %04x %04x %04x %04x %04x %04x\n", g_readFS[0], g_readFS[1], g_readCS[0], g_readCS[1], g_readCS[2], g_readCS[3], g_readCS[4], g_readCS[5], g_readCS[6], g_readCS[7], g_readCS[8]);
    pthread_mutex_unlock(&serial_read);
#define B_RANGE 32767
    VECTOR_MIXED(g_B[mag_index], g_B[mag_index], B_RANGE, -);
    VECTOR_MIXED(g_B[mag_index], g_B[mag_index], 4e-4 * 1e7 / B_RANGE, *); // in milliGauss to have precision
#else                                                                      // HITL
    short mag_measure[3];
    status = lsm9ds1_read_mag(mag, mag_measure);
    if (status < 0) // failure
        return status;
    x_g_B[mag_index] = mag_measure[0] / 6.842; // scaled to milliGauss
    y_g_B[mag_index] = mag_measure[1] / 6.842;
    z_g_B[mag_index] = mag_measure[2] / 6.842;
    APPLY_DBESSEL(g_B, mag_index); // bessel filter
#ifdef CSS_READY
    for (int i = 0; i < 3; i++)
    {
        tca9458a_set(mux, i); // activate channel
        for (int j = 0; j < 3; j++)
        {
            uint32_t measure;
            errno = 0;                                 // unset errno
            tsl2561_measure(css[i * 3 + j], &measure); // make measurement
            if (errno)
            {
                perror("CSS measure");
                return -1;
            }
            g_CSS[i * 3 + j] = tsl2561_get_lux(measure);
        }
    }
#else
    for (int i = 0; i < 9; i++)
        g_CSS[i] = 0;
#endif // CSS_READY
#ifdef FSS_READY
    // TODO: Read FSS
#else
    g_FSS[0] = -90;
    g_FSS[1] = -90;
#endif // FSS_READY
#endif // SITL

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
    APPLY_DBESSEL(g_Bt, bdot_index); // bessel filter
    // APPLY_FBESSEL(g_Bt, bdot_index); // bessel filter
    // printf("readSensors: m0: %d m1: %d Btx: %f Bty: %f Btz: %f\n", m0, m1, x_g_Bt[bdot_index], y_g_Bt[bdot_index], z_g_Bt[bdot_index]);
    getOmega();
    getSVec();
    // check if any of the values are NaN. If so, return -1
    // the NaN may stem from Bdot = 0, which may stem from the fact that during sunpointing
    // B may align itself with Z/ω
    if (isnan(x_g_B[mag_index]))
        return -1;
    if (isnan(y_g_B[mag_index]))
        return -1;
    if (isnan(z_g_B[mag_index]))
        return -1;

    if (isnan(x_g_W[omega_index]))
        return -1;
    if (isnan(y_g_W[omega_index]))
        return -1;
    if (isnan(z_g_W[omega_index]))
        return -1;

    if (isnan(x_g_S[sol_index]))
        return -1;
    if (isnan(y_g_S[sol_index]))
        return -1;
    if (isnan(z_g_S[sol_index]))
        return -1;
    return status;
}

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

    DECLARE_VECTOR(body, float);                     // Body frame vector oriented along Z axis
    z_body = 1;                                      // Body frame vector is Z
    float W_target_diff = z_g_W_target - z_avgOmega; // difference of omega_z
    NORMALIZE(avgOmega, avgOmega);                   // Normalize avg omega to get omega hat
    float w_ang = (DOT_PRODUCT(avgOmega, body));
    w_ang = w_ang >= 1 ? 1 : w_ang;
    float z_w_ang = 180. * acos(w_ang) / M_PI;                     // average omega angle in degrees
    float z_S_ang = 180. * acos(DOT_PRODUCT(avgSun, body)) / M_PI; // Sun angle in degrees
    // printf("[state %d] dW = %.3f, Ang = %.3f, DP = %.3f, |SUN| = %.3f\n", g_acs_mode, fabs(W_target_diff), fabs(z_w_ang), DOT_PRODUCT(avgOmega, body), NORM(avgSun));
    uint8_t next_mode = g_acs_mode;
    if (g_acs_mode == STATE_ACS_DETUMBLE)
    {
        // printf("[CASE %d] %d\n", g_acs_mode , MIN_DETUMBLE_ANGLE - z_w_ang);
        // If detumble criterion is met, go to Sunpointing mode
        if (fabsf(z_w_ang) < MIN_DETUMBLE_ANGLE && fabsf(W_target_diff) < OMEGA_TARGET_LEEWAY)
        {
            //  printf("[DETUMBLE]\n");
            //  fflush(stdout);
            next_mode = STATE_ACS_NIGHT;
            g_first_detumble = 0; // when system detumbles for the first time, unsets this variable
        }
        if (!g_first_detumble) // if this var is unset, the system does not do anything at night
        {
            if (NORM(avgSun) < 0.8f)
            {
                // printf("Here!");
                next_mode = STATE_ACS_NIGHT;
            }
        }
    }

    else if (g_acs_mode == STATE_ACS_SUNPOINT)
    {
        // If detumble criterion is not held, fall back to detumbling
        if (fabsf(z_w_ang) > MIN_DETUMBLE_ANGLE || fabsf(W_target_diff) > OMEGA_TARGET_LEEWAY * 3) // extra leeway for exact value of w_z
        {
            next_mode = STATE_ACS_DETUMBLE;
        }
        // if it is night, fall back to night mode. Should take SH_BUFFER_SIZE * DETUMBLE_TIME_STEP seconds for the actual state change to occur
        if (NORM(avgSun) < 0.8f)
        {
            next_mode = STATE_ACS_NIGHT;
        }
        // if the satellite is detumbled, it is not night and the sun angle is less than 4 deg, declare ACS is ready
        if (fabsf(z_S_ang) < MIN_SOL_ANGLE)
        {
            next_mode = STATE_ACS_READY;
        }
    }

    else if (g_acs_mode == STATE_ACS_NIGHT)
    {
        // printf("[NIGHT] %.3f\n", NORM(avgSun));
        if (NORM(avgSun) > 0.8f)
        {
            // printf("[NIGHT] %.3f\n", NORM(avgSun));
            if (fabsf(z_w_ang) > MIN_DETUMBLE_ANGLE || fabsf(W_target_diff) > OMEGA_TARGET_LEEWAY)
            {
                next_mode = STATE_ACS_DETUMBLE;
            }
            if (fabsf(z_S_ang) < MIN_SOL_ANGLE)
            {
                next_mode = STATE_ACS_READY;
            }
            else
                next_mode = STATE_ACS_SUNPOINT;
        }
    }

    else if (g_acs_mode == STATE_ACS_READY)
    {
        if (NORM(avgSun) < 0.8f) // transition to night
            next_mode = STATE_ACS_NIGHT;
        else
        {
            if (fabsf(z_w_ang) > MIN_DETUMBLE_ANGLE || fabsf(W_target_diff) > OMEGA_TARGET_LEEWAY) // Detumble required
            {
                next_mode = STATE_ACS_DETUMBLE;
            }
            if (fabsf(z_S_ang) > MIN_SOL_ANGLE) // sunpointing required
            {
                next_mode = STATE_ACS_SUNPOINT;
            }
            else // everything good
            {
                next_mode = STATE_ACS_READY;
            }
        }
    }
    g_acs_mode = next_mode; // update the global state
}

void *acs_thread(void *id)
{
    while (!done)
    {
        // first run indication
        if (first_run)
        {
            printf("ACS: Waiting for release...\n");
            first_run = 0;
#ifdef SITL
            // wait till there is available data on serial
            pthread_cond_wait(&data_available, &data_check);
#endif // SITL
        }
        unsigned long long s = get_usec();
        /* TODO: Soft- and hard- errors: All errors do not require a buffer reset, e.g. a CSS read error */
        if (readSensors() < 0) // error in readSensors
        {
            // Flush all buffers
            FLUSH_BUFFER(g_B);
            mag_index = -1;
            B_full = 0;

            FLUSH_BUFFER(g_Bt);
            bdot_index = -1;

            FLUSH_BUFFER(g_W);
            omega_index = -1;
            W_full = 0;

            FLUSH_BUFFER(g_S);
            sol_index = -1;
            S_full = 0;
            /*
             * Fall back into night mode which is the safe mode
             * NOTE: Since the buffers are empty at this point, 
             * checkTransition() will not be called. Hence, 
             * after flushing the buffer the system will spend
             * 64 cycles reading the sensors.
             * 
             * TODO: Add more checks and do a system reboot if
             * such a state persists.
             */
            g_acs_mode = STATE_ACS_NIGHT;
        }
        //time_t now ; time(&now);
        if (omega_index >= 0)
        {
#ifdef ACS_PRINT
#ifdef SITL
            printf("[%.3f ms][%.3f ms][%llu][%d] | Wx = %.3e Wy = %.3e Wz = %.3e\n", comm_time / 1000.0, (s - g_t_acs) / 1000.0, acs_ct++, g_acs_mode, x_g_W[omega_index], y_g_W[omega_index], z_g_W[omega_index]);
#else
            printf("[%.3f ms][%llu][%d] | Wx = %.3e Wy = %.3e Wz = %.3e\n", (s - g_t_acs) / 1000.0, acs_ct++, g_acs_mode, x_g_W[omega_index], y_g_W[omega_index], z_g_W[omega_index]);
#endif // SITL
#endif // ACS_PRINT
#ifdef DATAVIS
            // Update datavis variables [DO NOT TOUCH]
            g_datavis_st.data.step = acs_ct;
            g_datavis_st.data.mode = g_acs_mode;
            g_datavis_st.data.x_B = x_g_B[mag_index];
            g_datavis_st.data.y_B = y_g_B[mag_index];
            g_datavis_st.data.z_B = z_g_B[mag_index];
            g_datavis_st.data.x_Bt = x_g_Bt[bdot_index];
            g_datavis_st.data.y_Bt = y_g_Bt[bdot_index];
            g_datavis_st.data.z_Bt = z_g_Bt[bdot_index];
            g_datavis_st.data.x_W = x_g_W[omega_index];
            g_datavis_st.data.y_W = y_g_W[omega_index];
            g_datavis_st.data.z_W = z_g_W[omega_index];
            g_datavis_st.data.x_S = x_g_S[sol_index];
            g_datavis_st.data.y_S = y_g_S[sol_index];
            g_datavis_st.data.z_S = z_g_S[sol_index];
            // wake up datavis thread [DO NOT TOUCH]
            pthread_cond_broadcast(&datavis_drdy);
#endif
        }
#ifdef ACS_DATALOG
        fprintf(acs_datalog, "%llu %d %e %e %e %e %e %e %e %e %e\n", acs_ct, g_acs_mode, x_g_B[mag_index], y_g_B[mag_index], z_g_B[mag_index], x_g_W[omega_index], y_g_W[omega_index], z_g_W[omega_index], x_g_S[sol_index], y_g_S[sol_index], z_g_S[sol_index]);
#endif
        //    printf("%s ACS step: %llu | Wx = %f Wy = %f Wz = %f\n", ctime(&now), acs_ct++ , x_g_W[omega_index], y_g_W[omega_index], z_g_W[omega_index]);
        g_t_acs = s;
        checkTransition(); // check if the system should transition from one state to another
        unsigned long long e = get_usec();
        /* TODO: In case a read takes longer, reduce ACS action time in order to conserve loop time */
        int sleep_time = MEASURE_TIME - e + s;
        sleep_time = sleep_time > 0 ? sleep_time : 0;
        usleep(sleep_time); // sleep for total 20 ms with read
        if (g_acs_mode == STATE_ACS_DETUMBLE)
            detumbleAction();
        else if (g_acs_mode == STATE_ACS_SUNPOINT)
            sunpointAction();
        else
            usleep(DETUMBLE_TIME_STEP - MEASURE_TIME);
    }
    pthread_exit(NULL);
}

static inline void detumbleAction(void)
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
        HBRIDGE_DISABLE(0);
        HBRIDGE_DISABLE(1);
        HBRIDGE_DISABLE(2);
    }
}

static inline void sunpointAction(void)
{
    // printf("[Sunpoint Action]\n");
    // fflush(stdout);
    if (sol_index < 0)
    {
        // printf("[Sunpoint Action Invalid, sleep]\n");
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
        // printf("[Sunpoint Action] %d\n", __LINE__);
        VECTOR_OP(currB, currB, g_B[mag_index], +); // get current magfield
        NORMALIZE(currBNorm, currB);                // normalize current magfield
        MATVECMUL(currL, MOI, g_W[omega_index]);    // calculate current angular momentum
        VECTOR_OP(currS, currS, g_S[sol_index], +); // get current sunvector
        NORMALIZE(currSNorm, currS);                // normalize sun vector
        // printf("[Sunpoint Action] %d\n", __LINE__);
        // calculate S_B_hat
        DECLARE_VECTOR(SBHat, float);
        float SdotB = DOT_PRODUCT(currSNorm, currBNorm);
        VECTOR_MIXED(SBHat, currBNorm, SdotB, *);
        VECTOR_OP(SBHat, currSNorm, SBHat, +);
        NORMALIZE(SBHat, SBHat);
        // printf("[Sunpoint Action] %d\n", __LINE__);
        // calculate L_B_hat
        DECLARE_VECTOR(LBHat, float);
        float LdotB = DOT_PRODUCT(currL, currBNorm);
        VECTOR_MIXED(LBHat, currBNorm, LdotB, *);
        VECTOR_OP(LBHat, currL, LBHat, +);
        NORMALIZE(LBHat, LBHat);
        // printf("[Sunpoint Action] %d\n", __LINE__);
        // cross product the two vectors
        DECLARE_VECTOR(SxBxL, float);
        CROSS_PRODUCT(SxBxL, SBHat, LBHat);
        NORMALIZE(SxBxL, SxBxL);
        // printf("[Sunpoint Action] %d\n", __LINE__);
        float sun_ang = fabs(z_g_S[sol_index]);
        uint8_t gain = round(sun_ang * 32);
        gain = gain < 1 ? 1 : gain;                                                      // do not allow gain to be lower than one
        int time_on = (int)(DOT_PRODUCT(SxBxL, currBNorm) * SUNPOINT_DUTY_CYCLE * gain); // essentially a duty cycle measure
#ifdef SUNPOINT_DEBUG
        printf("[SUNPOINT] %d", time_on);
#endif // SUNPOINT_DEBUG
        int dir = time_on > 0 ? 1 : -1;
        time_on = time_on > 0 ? time_on : -time_on;
        time_on = time_on > SUNPOINT_DUTY_CYCLE ? SUNPOINT_DUTY_CYCLE : time_on; // safety measure
        if (time_on < 5000 && time_on > 2499)
            time_on = 5000;
        time_on = 10000 * round(time_on / 10000.0f); // added rounding to increase gain
        time_on /= 5000;
        time_on *= 5000; // granularity of 5 ms, essentially 5 bit precision
#ifdef SUNPOINT_DEBUG
        printf("[SUNPOINT] %d\n", time_on);
#endif // SUNPOINT_DEBUG
        int time_off = SUNPOINT_DUTY_CYCLE - time_on;
        int FiringTime = COARSE_TIME_STEP - MEASURE_TIME; // time allowed to fire
        DECLARE_VECTOR(fire, int);
        z_fire = dir; // z direction is the only direction of fire
        // printf("[Sunpoint Action] %d %d\n", __LINE__, FiringTime);
        while (FiringTime > 0)
        {
            // printf("[Sunpoint Action] %d %d %d %d\n", __LINE__, FiringTime, time_on, time_off);
            HBRIDGE_ENABLE(fire);
            usleep(time_on);
            if (time_off > 0)
            {
                HBRIDGE_DISABLE(2); // 3 == executes default, turns off ALL hbridges (safety)
                usleep(time_off);
            }
            FiringTime -= SUNPOINT_DUTY_CYCLE;
            // printf("[Sunpoint Action] %d %d\n", __LINE__, FiringTime);
        }
        // usleep(FiringTime + SUNPOINT_DUTY_CYCLE); // sleep for the remainder of the time
        HBRIDGE_DISABLE(2); // 3 == executes default, turns off ALL hbridges (safety)
    }
}

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

int acs_init(void)
{
    /* Set up data logging */
#ifdef ACS_DATALOG
    int bc = sys_boot_count;
    char fname[40] = {0}; // Holds file name where log file is saved
    sprintf(fname, "logfile%d.txt", bc);

    acs_datalog = fopen(fname, "w");
#endif // ACS_DATALOG
    /* End setup datalogging */

    // init for bessel coefficients
    calculateBessel(bessel_coeff, SH_BUFFER_SIZE, 3, BESSEL_FREQ_CUTOFF);

    // initialize target omega
    z_g_W_target = 1; // 1 rad s^-1
    MATVECMUL(g_L_target, MOI, g_W_target); // calculate target angular momentum

#ifndef SITL // Prepare devices for HITL
    hbridge = (ncv7708 *)malloc(sizeof(ncv7708));
    if (hbridge == NULL)
        return ERROR_MALLOC;
    snprintf(hbridge->fname, 40, SPIDEV_ACS);
#ifdef FSS_READY
    ads1115 *adc = (ads1115 *)malloc(sizeof(ads1115));
    if (adc == NULL)
        return ERROR_MALLOC;
#endif
    css = (tsl2561 **)malloc(9 * sizeof(tsl2561 *));
    for (int i = 0; i < 9; i++)
    {
        css[i] = (tsl2561 *)malloc(sizeof(tsl2561));
        if (css[i] == NULL)
            return ERROR_MALLOC;
    }
    mux = (tca9458a *)malloc(sizeof(tca9458a));
    if (mux == NULL)
        return ERROR_MALLOC;
    snprintf(mux->fname, 40, I2C_BUS);
    int init_stat = 0;
    init_stat = ncv7708_init(hbridge); // Initialize hbridge
    if (init_stat < 0)
        return ERROR_HBRIDGE_INIT;
#ifdef CSS_READY
    // Initialize MUX
    if ((init_stat = tca9458a_init(mux, 0x70)) < 0)
    {
        perror("Mux init failed");
        return ERROR_MUX_INIT;
    }
    // Initialize CSSs
    for (int i = 0; i < 3; i++)
    {
        uint8_t css_addr = TSL2561_ADDR_LOW;
        tca9458a_set(mux, i);
        for (int j = 0; j < 3; j++)
        {
            if ((init_stat = tsl2561_init(css[3 * i + j], css_addr)) < 0)
            {
                perror("CSS init failed");
                printf("CSS Init failed at channel %d addr 0x%02x\n", i, css_addr);
                fflush(stdout);
                return ERROR_CSS_INIT;
            }
            css_addr += 0x10;
        }
    }
    tca9458a_set(mux, 8); // disables mux
#endif                    // CSS_READY
    // Initialize magnetometer
    mag = (lsm9ds1 *)malloc(sizeof(lsm9ds1));
    snprintf(mag->fname, 40, I2C_BUS);
    if ((init_stat = lsm9ds1_init(mag, 0x6b, 0x1e)) < 0)
    {
        perror("Magnetometer init failed");
        return ERROR_MAG_INIT;
    }
    // Initialize adc
#ifdef FSS_READY
    init_stat = ads1115_init(adc, ADS1115_S_ADDR);
    if (init_stat < 0)
    {
        perror("ADC init failed");
        return ERROR_FSS_INIT;
    }
    ads1115_config adc_conf;

    adc_conf.raw = 0x0000;

    adc_conf.os = 0;
    adc_conf.mux = 0;
    adc_conf.pga = 1;
    adc_conf.mode = 0;
    adc_conf.dr = 5;
    adc_conf.comp_mode = 0;
    adc_conf.comp_pol = 0;
    adc_conf.comp_lat = 0;
    adc_conf.comp_que = 3;

    init_stat = ads1115_configure(adc, adc_conf);
    if (init_stat < 0)
    {
        perror("ADC config failed");
        return ERROR_FSS_CONFIG;
    }
#endif               // FSS_READY
    usleep(1000000); // sleep 1 second
#endif               // ifndef SITL
    return 1;
}

void acs_destroy(void)
{
#ifndef SITL // if SITL, no need to disable any devices
#ifdef CSS_READY
    // Destroy CSSs
    for (int i = 0; i < 3; i++)
    {
        uint8_t css_addr = TSL2561_ADDR_LOW;
        tca9458a_set(mux, i);
        for (int j = 0; j < 3; j++)
        {
            tsl2561_destroy(css[3 * i + j]);
            css_addr += 0x10;
        }
    }
    tca9458a_set(mux, 8); // disables mux
#endif                    // CSS_READY
    tca9458a_destroy(mux);
    lsm9ds1_destroy(mag);
    ncv7708_destroy(hbridge);
#ifdef FSS_READY
    // destroy FSS ADC
    ads1115_destroy(adc);
#endif // FSS_READY
#endif // SITL
}