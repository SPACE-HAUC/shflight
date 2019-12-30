#include <main.h>
#include <acs.h>

// void* acs(void* id): ACS action thread. This thread actuates the torque coils depending on the state
// the system is in. Returns control to data_ack(void*) when the task is complete.
void * acs(void * id)
{
    do {
        pthread_cond_wait(&cond_acs, &mutex_acs);
        // do ACS things
        switch(g_program_state){
            case SH_ACS_DETUMBLE:
            DECLARE_VECTOR(currL, float); // vector for current angular momentum
            MATVECMUL(currL, MOI, g_W[omega_index]); // calculate current angular momentum
            VECTOR_OP(currL, g_L_target, currL, -); // calculate angular momentum error
            DECLARE_VECTOR(currLNorm, float);
            NORMALIZE(currLNorm, currL); // normalize the angular momentum error vector
            DECLARE_VECTOR(currB, float); // current normalized magnetic field TMP
            NORMALIZE(currB, g_B[mag_index]); // normalize B
            DECLARE_VECTOR(firingDir, float); // firing direction vector
            CROSS_PRODUCT(firingDir, currB, currLNorm); // calculate firing direction
            int8_t x_fire = (x_firingDir < 0 ? -1 : 1) * (abs(x_firingDir) > 0.01 ? 1 : 0) ; // if > 0.01, then fire in the direction of input
            int8_t y_fire = (y_firingDir < 0 ? -1 : 1) * (abs(y_firingDir) > 0.01 ? 1 : 0) ; // if > 0.01, then fire in the direction of input
            int8_t z_fire = (z_firingDir < 0 ? -1 : 1) * (abs(z_firingDir) > 0.01 ? 1 : 0) ; // if > 0.01, then fire in the direction of input
            DECLARE_VECTOR(currDipole, float);
            VECTOR_MIXED(currDipole,fire,DIPOLE_MOMENT,*); // calculate dipole moment
            DECLARE_VECTOR(currTorque, float);
            CROSS_PRODUCT(currTorque, currDipole, g_B[mag_index]); // calculate current torque
            DECLARE_VECTOR(firingTime, float); // initially gives firing time in seconds
            VECTOR_MIXED(firingTime, currL, currTorque, /) ; // calculate firing time based on current torque
            VECTOR_MIXED(firingTime, firingTime, 1000000, *); // convert firing time to usec
            VECTOR_MIXED(firingCmd, int); // integer firing time in usec
            x_firingCmd = x_firingTime > MAX_DETUMBLE_FIRING_TIME ? MAX_DETUMBLE_FIRING_TIME : ( x_firingTime < MIN_DETUMBLE_FIRING_TIME ? 0 : (int) x_firingTime ) ;
            y_firingCmd = y_firingTime > MAX_DETUMBLE_FIRING_TIME ? MAX_DETUMBLE_FIRING_TIME : ( y_firingTime < MIN_DETUMBLE_FIRING_TIME ? 0 : (int) y_firingTime ) ;
            z_firingCmd = z_firingTime > MAX_DETUMBLE_FIRING_TIME ? MAX_DETUMBLE_FIRING_TIME : ( z_firingTime < MIN_DETUMBLE_FIRING_TIME ? 0 : (int) z_firingTime ) ;
            int firingOrder[3] = {0,1,2}, firingTime[3] ; // 0 == x, 1 == y, 2 == z
            firingTime[0] = x_firingCmd ;
            firingTime[1] = y_firingCmd ;
            firingTime[2] = z_firingCmd ;
            insertionSort(firingTime, firingOrder) ; // sort firing order based on firing time
            int finalWait = MAX_DETUMBLE_FIRING_TIME - firingTime[2] ;
            firingTime[2] -= firingTime[1] ; // time after second one turns off
            firingTime[1] -= firingTime[0] ; // time after first one turns off
            HBRIDGE_ENABLE(fire); // Turns on the torque coils in the required directions determined by the fire vector
            usleep(firingTime[0]<1?1:firingTime[0]); // sleep until first turnoff
            HBRIDGE_DISABLE(firingOrder[0]) ; // first turn off
            usleep(firingTime[1]<1?1:firingTime[1]); // sleep until second turnoff
            HBRIDGE_DISABLE(firingOrder[1]); // second turnoff
            usleep(firingTime[2]<1?1:firingTime[2]); // sleep until third turnoff
            HBRIDGE_DISABLE(firingOrder[2]); // third turnoff
            usleep(finalWait<1?1:finalWait); // sleep for the remainder of the cycle
            break; // get out of switch and wake up the data acquisition thread
        }
        pthread_cond_broadcast(&cond_data_ack);
    } while(1) ;
    pthread_exit((void*)NULL) ;
}