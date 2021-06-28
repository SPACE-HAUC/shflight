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
#include <finesunsensor/a60sensor.h>
#include <lsm9ds1/lsm9ds1.h>
#include <ncv7718/ncv7718.h>
#include <tsl2561/tsl2561.h>
#include <tca9458a/tca9458a.h>
#include <math.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <datalogger_extern.h>

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

#define THIS_MODULE "acs"

/* Variable allocation for ACS */

/**
 * @brief This variable is unset by the ACS thread at first execution.
 * 
 */
volatile int first_run = 1;

// HITL
/**
 * @brief Magnetometer device struct.
 * 
 */
lsm9ds1 mag[1]; // magnetometer
/**
 * @brief H-Bridge device struct.
 * 
 */
ncv7718 hbridge[1]; // h-bridge
/**
 * @brief I2C Mux device struct.
 * 
 */
tca9458a mux[1]; // I2C mux
/**
 * @brief Array of coarse sun sensor device struct.
 * 
 */
tsl2561 css[7]; // coarse sun sensors
/**
 * @brief I2C ADC struct for fine sun sensor.
 * 
 */
a60sensor fss[1]; // fine sun sensor
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
float g_CSS[7]; // current CSS lux values, in HITL this will be populated by TSL2561 code
/**
 * @brief Indicate if Mux channel has error
 * 
 */
bool mux_err_channel[3] = {false, false, false};
/**
 * @brief Storage for current fine sun sensor angle measurements.
 * 
 */
float g_FSS[2]; // current FSS angles, in rad; in HITL this will be populated by NANOSSOC A60 driver
/**
 * @brief Stores the return value of FSS algorithm
 * 
 */
int g_FSS_RET; // return value of FSS
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
float MOI[3][3] = {{0.0821, 0, 0},
                   {0, 0.0752, 0},
                   {0, 0, 0.0874}};
/**
 * @brief Inverse of the moment of inertia of the satellite (SI).
 * 
 */
float IMOI[3][3] = {{12.1733, 0, 0},
                    {0, 13.2941, 0},
                    {0, 0, 11.4661}};
/**
 * @brief Current timestamp after readSensors() in ACS thread, used to keep track of time taken by ACS loop.
 * 
 */
unsigned long long g_t_acs;

/**
 * @brief Dipole moment of the magnetorquer rods
 * 
 */
static float DIPOLE_MOMENT = 0.22; // A m^-2
/**
 * @brief ACS loop time period
 * 
 */
static uint32_t DETUMBLE_TIME_STEP = 100000; // 100 ms for full loop
/**
 * @brief ACS readSensors() max execute time per cycle
 * 
 */
static uint32_t MEASURE_TIME = 30000; // 30 ms to measure
/**
 * @brief ACS max actuation time per cycle
 * 
 */
#define MAX_DETUMBLE_FIRING_TIME (DETUMBLE_TIME_STEP - MEASURE_TIME) // Max allowed detumble fire time
/**
 * @brief Minimum magnetorquer firing time
 * 
 */
static uint32_t MIN_DETUMBLE_FIRING_TIME = 10000; // 10 ms
/**
 * @brief Sunpointing magnetorquer PWM duty cycle
 * 
 */
static uint32_t SUNPOINT_DUTY_CYCLE = 20000; // 20 msec, in usec
/**
 * @brief Course sun sensing mode loop time for ACS
 * 
 */
#define COARSE_TIME_STEP DETUMBLE_TIME_STEP // 100 ms, in usec
/**
 * @brief Coarse sun sensor minimum lux threshold for valid measurement
 * 
 */
static float CSS_MIN_LUX_THRESHOLD = 40000 * 0.5; // 5000 lux is max sun, half of that is our threshold (subject to change)
/**
 * @brief Leeway factor for value of omega_z in percentage
 * 
 */
static float LEEWAY_FACTOR = 0.1;
/**
 * @brief Acceptable leeway of the angular speed target
 * 
 */
#define OMEGA_TARGET_LEEWAY z_g_W_target *LEEWAY_FACTOR // 10% leeway in the value of omega_z
/**
 * @brief Sunpointing angle target (in degrees)
 * 
 */
static float MIN_SOL_ANGLE = 20; // minimum solar angle for sunpointing to be a success
/**
 * @brief Detumble angle target (in degrees)
 * 
 */
static float MIN_DETUMBLE_ANGLE = 10; // minimum angle for detumble to be a success

int acs_get_moi(float *moi)
{
    if (moi == NULL)
        return -1;
    for (int i = 0; i < 3; i++)
        for (int j = 0; j < 3; j++)
            moi[3 * i + j] = MOI[i][j];
    return 1;
}

int acs_set_moi(float *moi)
{
    if (moi == NULL)
        return -1;
    for (int i = 0; i < 3; i++)
        for (int j = 0; j < 3; j++)
            MOI[i][j] = moi[3 * i + j];
    return 1;
}

int acs_get_imoi(float *imoi)
{
    if (imoi == NULL)
        return -1;
    for (int i = 0; i < 3; i++)
        for (int j = 0; j < 3; j++)
            imoi[3 * i + j] = IMOI[i][j];
    return 1;
}

int acs_set_imoi(float *imoi)
{
    if (imoi == NULL)
        return -1;
    for (int i = 0; i < 3; i++)
        for (int j = 0; j < 3; j++)
            IMOI[i][j] = imoi[3 * i + j];
    return 1;
}

float acs_get_dipole(void)
{
    return DIPOLE_MOMENT;
}

float acs_set_dipole(float d)
{
    if (d <= 0)
        d = 0.22;
    DIPOLE_MOMENT = d;
    return DIPOLE_MOMENT;
}

uint32_t acs_get_tstep(void)
{
    return DETUMBLE_TIME_STEP;
}

uint32_t acs_set_tstep(uint8_t t)
{
    if (t < 100)
        t = 100;
    t = (t / 10) * 10;             // in increments of 10 ms
    DETUMBLE_TIME_STEP = t * 1000; // ms to us
    return DETUMBLE_TIME_STEP;
}

uint32_t acs_get_measure_time(void)
{
    return MEASURE_TIME;
}

uint32_t acs_set_measure_time(uint8_t t)
{
    if (t < 20)
        t = 20;
    else if (t > 50)
        t = 50;
    t = (t / 10) * 10;
    MEASURE_TIME = t * 1000; // ms to us
    return MEASURE_TIME;
}

uint8_t acs_get_leeway(void)
{
    return 1 / LEEWAY_FACTOR;
}

uint8_t acs_set_leeway(uint8_t leeway)
{
    if (leeway < 5)
        leeway = 5;
    else if (leeway > 50)
        leeway = 50;
    LEEWAY_FACTOR = 1.0 / leeway;
    return acs_get_leeway();
}

float acs_get_wtarget(void)
{
    return z_g_W_target;
}

float acs_set_wtarget(float t)
{
    int sgn = t < 0 ? -1 : 1;
    if (fabs(t) < 0.1)
        t = 0.1 * sgn;
    else if (fabs(t) > 2)
        t = 2 * sgn;
    z_g_W_target = t;
    return z_g_W_target;
}

uint8_t acs_get_detumble_ang()
{
    return MIN_DETUMBLE_ANGLE;
}

uint8_t acs_set_detumble_ang(uint8_t ang)
{
    if (ang > 45)
        ang = 20; // conservative
    MIN_DETUMBLE_ANGLE = ang;
    return MIN_DETUMBLE_ANGLE;
}

uint8_t acs_get_sun_angle()
{
    return MIN_SOL_ANGLE;
}

uint8_t acs_set_sun_angle(uint8_t ang)
{
    if (ang > 45)
        ang = 20; // conservative
    MIN_SOL_ANGLE = ang;
    return MIN_SOL_ANGLE;
}

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

int hbridge_enable(int x, int y, int z)
{
    ncv7718_set_output(hbridge, 1, x);
    ncv7718_set_output(hbridge, 0, y);
    ncv7718_set_output(hbridge, 2, z);
    return ncv7718_exec_output(hbridge);
}

int HBRIDGE_DISABLE(int num)
{
    ncv7718_set_output(hbridge, 1, 0);
    ncv7718_set_output(hbridge, 0, 0);
    ncv7718_set_output(hbridge, 2, 0);
    return ncv7718_exec_output(hbridge);
}

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
    // hardware reads degrees, and angles are reversed in hardware
    float fsx = -g_FSS[0];
    float fsy = -g_FSS[1];
#ifndef M_PI
/**
 * @brief Approximate definition of Pi in case M_PI is not included from math.h
 */
#define M_PI 3.1415
#endif
    // check if FSS results are acceptable
    // if they are, use that to calculate the sun vector
    // printf("[FSS] %.3f %.3f\n", fsx * 180. / M_PI, fsy * 180. / M_PI);
    if (!(g_FSS_RET & (E_INDEX_MIN | E_INDEX_MAX | E_ANGLEX | E_ANGLEY | E_DIV_ZERO))) // angle inside FOV (FOV -> 60°, half angle 30°)
    {
#ifdef ACS_PRINT
        printf("[" GRN "FSS" RST "]");
#endif                                            // ACS_PRINT
        x_g_S[sol_index] = tan(fsx * M_PI / 180); // Consult https://www.cubesatshop.com/wp-content/uploads/2016/06/nanoSSOC-A60-Technical-Specifications.pdf, section 4
        y_g_S[sol_index] = tan(fsy * M_PI / 180);
        z_g_S[sol_index] = 1;
        NORMALIZE(g_S[sol_index], g_S[sol_index]);
        g_night = 0;
        return;
    }

    // get average -Z luminosity from 2 sensors
    float znavg = 0;
    for (int i = 5; i < 7; i++)
        znavg += g_CSS[i];
    znavg *= 0.5f;

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

DECLARE_VECTOR(mag_mes, float);

uint32_t css_0, css_1, css_2, css_3, css_4, css_5, css_6, css_7;

float fss_0, fss_1;

bool mux_err_0 = false, mux_err_1 = false, mux_err_2 = false;

int readSensors(void)
{
    // read magfield, CSS, FSS
    // printf("In readSensors()...\n");
    int status = 1;
    if (mag_index == SH_BUFFER_SIZE - 1) // hit max, buffer full
        B_full = 1;
    mag_index = (mag_index + 1) % SH_BUFFER_SIZE;
    VECTOR_CLEAR(g_B[mag_index]); // clear the current B                                                  /
    // HITL
    short mag_measure[3];
    // if ((status = i2cbus_lock(fss)))
    // {
    //     shprintf("Error %d locking I2C bus for ACS readouts\n", status);
    // }
    // read magnetic sensor
    status = lsm9ds1_read_mag(mag, mag_measure);
    if (status < 0) // failure
    {
        shprintf("Error reading magsensor\n");
    }
    // read coarse sun sensors
    uint32_t mes;
    // activate mux channel 0
    if (mux_err_channel[0])
    {
        shprintf("Mux channel 0 has device error, skipping\n");
        goto read_mux_1;
    }
    if (tca9458a_set(mux, 0) < 0)
    {
        shprintf("Could not set mux channel 0\n");
        goto read_mux_1;
    }
    { // read dev on chn 0
        bool mux_err = true;
        if (tsl2561_measure(&(css[0]), &mes) > 0) // all good
        {
            g_CSS[0] = css_0 = mes & 0xffff;
            mux_err = false;
        }
        if (tsl2561_measure(&(css[1]), &mes) > 0) // all good
        {
            g_CSS[1] = css_1 = mes & 0xffff;
            mux_err = false;
        }
        if (tsl2561_measure(&(css[2]), &mes) > 0) // all good
        {
            g_CSS[2] = css_2 = mes & 0xffff;
            mux_err = false;
        }
        mux_err_channel[0] = mux_err_0 = mux_err;
    }
// activate mux channel 1
read_mux_1:
    if (mux_err_channel[1])
    {
        shprintf("Mux channel 1 has device error, skipping\n");
        goto read_mux_2;
    }
    if (tca9458a_set(mux, 1) < 0)
    {
        shprintf("Could not set mux channel 0\n");
        goto read_mux_2;
    }
    { // read dev on chn 0
        bool mux_err = true;
        if (tsl2561_measure(&(css[3]), &mes) > 0) // all good
        {
            g_CSS[3] = css_3 = mes & 0xffff;
            mux_err = false;
        }
        if (tsl2561_measure(&(css[4]), &mes) > 0) // all good
        {
            g_CSS[4] = css_4 = mes & 0xffff;
            mux_err = false;
        }
        if (tsl2561_measure(&(css[5]), &mes) > 0) // all good
        {
            g_CSS[5] = css_5 = mes & 0xffff;
            mux_err = false;
        }
        mux_err_channel[1] = mux_err_1 = mux_err;
    }
read_mux_2:
    if (mux_err_channel[2])
    {
        shprintf("Mux channel 2 has device error, skipping\n");
        goto read_css;
    }
    // activate mux channel 0
    if (tca9458a_set(mux, 2) < 0)
    {
        shprintf("Could not set mux channel 0\n");
    }
    { // read dev on chn 0
        bool mux_err = true;
        if (tsl2561_measure(&(css[6]), &mes) > 0) // all good
        {
            g_CSS[6] = css_6 = mes & 0xffff;
            mux_err = false;
        }
        else if (tsl2561_measure(&(css[6]), &mes) > 0) // all good
        {
            g_CSS[6] = css_6 = mes & 0xffff;
            mux_err = false;
        }
        mux_err_channel[2] = mux_err_2 = mux_err;
    }
read_css:
    if (tca9458a_set(mux, 8) != 1)
    {
        shprintf("Error disabling mux\n");
    }
    if ((g_FSS_RET = a60sensor_read(fss, &(g_FSS[0]), &(g_FSS[1]))) < 0)
    {
        shprintf("Error getting data from fine sun sensor\n");
    }
    // if ((status = i2cbus_unlock(fss)))
    // {
    //     shprintf("Error %d unlocking I2C bus for ACS readouts\n", status);
    // }
    x_mag_mes = -mag_measure[1] / 6.842;
    y_mag_mes = mag_measure[0] / 6.842;
    z_mag_mes = mag_measure[2] / 6.842;
    x_g_B[mag_index] = x_mag_mes; // scaled to milliGauss
    y_g_B[mag_index] = y_mag_mes;
    z_g_B[mag_index] = z_mag_mes;
    fss_0 = g_FSS[0];
    fss_1 = g_FSS[1];
    APPLY_DBESSEL(g_B, mag_index); // bessel filter

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
    // log data
#ifdef ACS_FILE_DATALOG
    DLGR_WRITE(x_mag_mes);
    DLGR_WRITE(y_mag_mes);
    DLGR_WRITE(z_mag_mes);
    DLGR_WRITE(css_0);
    DLGR_WRITE(css_1);
    DLGR_WRITE(css_2);
    DLGR_WRITE(css_3);
    DLGR_WRITE(css_4);
    DLGR_WRITE(css_5);
    DLGR_WRITE(css_6);
    DLGR_WRITE(g_FSS_RET);
    DLGR_WRITE(fss_0);
    DLGR_WRITE(fss_1);
#endif
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

acs_uhf_packet acs_upd;

void *acs_thread(void *id)
{
    uint64_t acs_thread_start = get_usec();
    pthread_mutex_lock(&datavis_mutex);
    g_datavis_st.data.tstart = acs_thread_start;
    pthread_mutex_unlock(&datavis_mutex);
    acs_upd.ct = 0;
    while (!done)
    {
        // first run indication
        if (first_run)
        {
            printf("ACS: Waiting for release...\n");
            first_run = 0;
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
            pthread_mutex_lock(&datavis_mutex);
            g_datavis_st.data.tnow = s;
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
            pthread_mutex_unlock(&datavis_mutex);
#endif
            acs_upd.mode = g_acs_mode;
            acs_upd.bx = x_g_B[mag_index];
            acs_upd.by = y_g_B[mag_index];
            acs_upd.bz = z_g_B[mag_index];
            acs_upd.wx = x_g_W[mag_index];
            acs_upd.wy = y_g_W[mag_index];
            acs_upd.wz = z_g_W[mag_index];
            acs_upd.sx = x_g_S[mag_index];
            acs_upd.sy = y_g_S[mag_index];
            acs_upd.sz = z_g_S[mag_index];
            acs_upd.ct++;
        }
#ifdef ACS_DATALOG
        fprintf(acs_datalog, "%llu %d %e %e %e %e %e %e %e %e %e\n", acs_ct, g_acs_mode, x_g_B[mag_index], y_g_B[mag_index], z_g_B[mag_index], x_g_W[omega_index], y_g_W[omega_index], z_g_W[omega_index], x_g_S[sol_index], y_g_S[sol_index], z_g_S[sol_index]);
#endif
        //    printf("%s ACS step: %llu | Wx = %f Wy = %f Wz = %f\n", ctime(&now), acs_ct++ , x_g_W[omega_index], y_g_W[omega_index], z_g_W[omega_index]);
        g_t_acs = s;
        checkTransition(); // check if the system should transition from one state to another
        unsigned long long e = get_usec();
        /* In case a read takes longer, reset to keep loop jitter minimal */
        int sleep_time = MEASURE_TIME - e + s;
        if (sleep_time < 0)
        {
            sleep_time = DETUMBLE_TIME_STEP - e + s;
            sleep_time > 0 ? sleep_time : 0;
            usleep(sleep_time);
            continue;
        }
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

#define ACS_I2C_CTX -1

int acs_init(void)
{
    /* Set up data logging */
#ifdef ACS_DATALOG
    int bc = sys_boot_count;
    char fname[40] = {0}; // Holds file name where log file is saved
    sprintf(fname, "logfile%d.txt", bc);

    acs_datalog = fopen(fname, "w");
#endif // ACS_DATALOG
#ifdef ACS_FILE_DATALOG
    DLGR_REGISTER_SINGLE(x_mag_mes);
    DLGR_REGISTER_SINGLE(y_mag_mes);
    DLGR_REGISTER_SINGLE(z_mag_mes);
    DLGR_REGISTER_SINGLE(css_0);
    DLGR_REGISTER_SINGLE(css_1);
    DLGR_REGISTER_SINGLE(css_2);
    DLGR_REGISTER_SINGLE(css_3);
    DLGR_REGISTER_SINGLE(css_4);
    DLGR_REGISTER_SINGLE(css_5);
    DLGR_REGISTER_SINGLE(css_6);
    DLGR_REGISTER_SINGLE(g_FSS_RET);
    DLGR_REGISTER_SINGLE(fss_0);
    DLGR_REGISTER_SINGLE(fss_1);
#endif
    /* End setup datalogging */

    // init for bessel coefficients
    calculateBessel(bessel_coeff, SH_BUFFER_SIZE, 3, BESSEL_FREQ_CUTOFF);

    // initialize target omega
    z_g_W_target = 1;                       // 1 rad s^-1
    MATVECMUL(g_L_target, MOI, g_W_target); // calculate target angular momentum

    int init_stat = 0;
    init_stat = ncv7718_init(hbridge, 0, 1, -1); // Initialize hbridge
    if (init_stat < 0)
        return ERROR_HBRIDGE_INIT;
    // Initialize MUX
    if (tca9458a_init(mux, 0, 0x70, ACS_I2C_CTX) < 0)
    {
        shprintf("Could not initialize mux\n");
        return ERROR_MUX_INIT;
    }
    // activate mux channel 0
    if (tca9458a_set(mux, 0) < 0)
    {
        printf("Could not set mux channel 0\n");
        goto set_mux_1;
    }
    {
        bool mux_err = true;
        // activate dev on channel 0
        if (tsl2561_init(&(css[0]), 0, 0x29, ACS_I2C_CTX) > 0)
        {
            mux_err &= false;
        }
        else
        {
            shprintf("Could not open 0x29 chn 0\n");
        }
        if (tsl2561_init(&(css[1]), 0, 0x39, ACS_I2C_CTX) > 0)
        {
            mux_err &= false;
        }
        else
        {
            shprintf("Could not open 0x39 chn 0\n");
        }
        if (tsl2561_init(&(css[2]), 0, 0x49, ACS_I2C_CTX) > 0)
        {
            mux_err &= false;
        }
        else
        {
            shprintf("Could not open 0x49 chn 0\n");
        }
        mux_err_channel[0] = mux_err;
    }
set_mux_1:
    // activate mux channel 1
    if (tca9458a_set(mux, 1) < 0)
    {
        printf("Could not set mux channel 1\n");
        goto set_mux_2;
    }
    {
        bool mux_err = true;
        // activate dev on channel 0
        if (tsl2561_init(&(css[3]), 0, 0x39, ACS_I2C_CTX) > 0)
        {
            mux_err &= false;
        }
        else
        {
            shprintf("Could not open 0x39 chn 1\n");
        }
        if (tsl2561_init(&(css[4]), 0, 0x49, ACS_I2C_CTX) > 0)
        {
            mux_err &= false;
        }
        else
        {
            shprintf("Could not open 0x49 chn 1\n");
        }
        if (tsl2561_init(&(css[5]), 0, 0x29, ACS_I2C_CTX) > 0)
        {
            mux_err &= false;
        }
        else
        {
            shprintf("Could not open 0x29 chn 1\n");
        }
        mux_err_channel[1] = mux_err;
    }
set_mux_2:
    // activate mux channel 2
    if (tca9458a_set(mux, 2) < 0)
    {
        printf("Could not set mux channel 2\n");
        goto init_fss;
    }
    {
        bool mux_err = true;
        // activate dev on channel 0
        if (tsl2561_init(&(css[6]), 0, 0x39, ACS_I2C_CTX) > 0)
        {
            mux_err &= false;
        }
        else
        {
            shprintf("Could not open 0x39 chn 2\n");
        }
        mux_err_channel[2] = mux_err;
    }
init_fss:
    tca9458a_set(mux, 8); // disables mux
    if (a60sensor_init(fss, 0, 0x4a, ACS_I2C_CTX) < 0)
    {
        shprintf("Could not initialize fine sun sensor\n");
    }
    // Initialize magnetometer
    if ((init_stat = lsm9ds1_init(mag, 0, 0x6b, 0x1e, ACS_I2C_CTX)) < 0)
    {
        shprintf("Magnetometer init failed\n");
        return ERROR_MAG_INIT;
    }
    shprintf("ACS init\n");
    sleep(1); // sleep 1 second
    return 1;
}

void acs_destroy(void)
{
    // FSS
    a60sendor_destroy(fss);
    // CSS
    if (!mux_err_channel[2])
    {
        tca9458a_set(mux, 2);
        tsl2561_destroy(&(css[7]));
    }
    if (!mux_err_channel[1])
    {
        tca9458a_set(mux, 1);
        tsl2561_destroy(&(css[6]));
        tsl2561_destroy(&(css[5]));
        tsl2561_destroy(&(css[4]));
    }
    if (!mux_err_channel[0])
    {
        tca9458a_set(mux, 0);
        tsl2561_destroy(&(css[3]));
        tsl2561_destroy(&(css[2]));
        tsl2561_destroy(&(css[1]));
    }
    tca9458a_set(mux, 8); // disable mux
    tca9458a_destroy(mux);
    lsm9ds1_destroy(mag);
    ncv7718_destroy(hbridge);
}
