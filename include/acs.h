#ifndef ACS_H
#define ACS_H

#include <main.h>

void *acs(void *id);

#define DIPOLE_MOMENT 0.21 // A m^2

#define MAX_DETUMBLE_FIRING_TIME DETUMBLE_TIME_STEP - MAG_MEASURE_TIME

#define MIN_DETUMBLE_FIRING_TIME 10000 // 10 ms

// 3 element array sort, first array is guide that sorts both arrays
void insertionSort(int a1[], int a2[]);

#define HBRIDGE_ENABLE(name) \
    hbridge_enable(x_##name, y_##name, z_##name);
int hbridge_enable(int x, int y, int z);
int HBRIDGE_DISABLE(int num);
// get omega using magnetic field measurements (Bdot)
void getOmega(void);
// get sun vector using lux measurements
void getSVec(void);
// read all sensors
int readSensors(void);
// check if the program should transition from one state to another
void checkTransition(void);
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
        HBRIDGE_DISABLE(0);
        HBRIDGE_DISABLE(1);
        HBRIDGE_DISABLE(2);
    }
}
// This function executes the sunpointing action
inline void sunpointAction(void)
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
        printf("[SUNPOINT] %d", time_on);
        int dir = time_on > 0 ? 1 : -1;
        time_on = time_on > 0 ? time_on : -time_on;
        time_on = time_on > SUNPOINT_DUTY_CYCLE ? SUNPOINT_DUTY_CYCLE : time_on; // safety measure
        if (time_on < 5000 && time_on > 2499)
            time_on = 5000;
        time_on = 10000 * round(time_on / 10000.0f); // added rounding to increase gain
        time_on /= 5000;
        time_on *= 5000; // granularity of 5 ms, essentially 5 bit precision
        printf("[SUNPOINT] %d\n", time_on);
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
#endif // ACS_H