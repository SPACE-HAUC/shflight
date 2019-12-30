#include <main.h>
#include <acs.h>

/* void* acs(void* id): ACS action thread. This thread actuates the torque coils depending on the state
 * the system is in. Returns control to data_ack(void*) when the task is complete.
 * 
 * HBRIDGE_ENABLE should take into consideration available power in the system
 */
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
            VECTOR_OP(firingTime, currL, currTorque, /) ; // calculate firing time based on current torque
            VECTOR_MIXED(firingTime, firingTime, 1000000, *); // convert firing time to usec
            DECLARE_VECTOR(firingCmd, int); // integer firing time in usec
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
            HBRIDGE_ENABLE(fire, int); // Turns on the torque coils in the required directions determined by the fire vector
            usleep(firingTime[0]<1?1:firingTime[0]); // sleep until first turnoff
            HBRIDGE_DISABLE(firingOrder[0]) ; // first turn off
            usleep(firingTime[1]<1?1:firingTime[1]); // sleep until second turnoff
            HBRIDGE_DISABLE(firingOrder[1]); // second turnoff
            usleep(firingTime[2]<1?1:firingTime[2]); // sleep until third turnoff
            HBRIDGE_DISABLE(firingOrder[2]); // third turnoff
            usleep(finalWait<1?1:finalWait); // sleep for the remainder of the cycle
            break; // get out of switch and wake up the data acquisition thread

            case SH_ACS_NIGHT: // same as detumble for now. Could implement power vector stuff 
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
            VECTOR_OP(firingTime, currL, currTorque, /) ; // calculate firing time based on current torque
            VECTOR_MIXED(firingTime, firingTime, 1000000, *); // convert firing time to usec
            DECLARE_VECTOR(firingCmd, int); // integer firing time in usec
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

            case SH_ACS_COARSE: // coarse sun pointing
            DECLARE_VECTOR(currB, float) ;
            DECLARE_VECTOR(currBNorm, float) ;
            DECLARE_VECTOR(currL, float) ;
            DECLARE_VECTOR(currLNorm, float) ;
            DECLARE_VECTOR(currS, float) ;
            DECLARE_VECTOR(currSNorm, float) ;
            VECTOR_OP(currB, currB, g_B[mag_index], +) ; // get current magfield
            NORMALIZE(currBNorm, currB); // normalize current magfield
            MATVECMUL(currL, MOI, g_W[omega_index]); // calculate current angular momentum
            VECTOR_OP(currS, currS, g_S[sol_index], +) ; // get current sunvector
            NORMALIZE(currSNorm, currS) ; // normalize sun vector
            // calculate S_B_hat
            DECLARE_VECTOR(SBHat, float);
            float SdotB = DOT_PRODUCT(currSNorm, currBNorm) ;
            VECTOR_MIXED(SBHat, currBNorm, SdotB, *) ;
            VECTOR_OP(SBHat, currSNorm, SBHat, - ) ;
            NORMALIZE(SBHat, SBHat) ;
            // calculate L_B_hat
            DECLARE_VECTOR(LBHat, float) ;
            float LdotB = DOT_PRODUCT(currL, currBNorm) ;
            VECTOR_MIXED(LBHat, currBNorm, LdotB, *) ;
            VECTOR_OP(LBHat, currL, LBHat, -) ;
            NORMALIZE(LBHat, LBHat) ;
            // cross product the two vectors
            DECLARE_VECTOR(SxBxL, float);
            CROSS_PRODUCT(SxBxL, SBHat, LBHat) ;
            NORMALIZE(SxBxL, SxBxL) ;

            int time_on = (int)(DOT_PRODUCT(SxBxL, currBNorm)*SUNPOINT_DUTY_CYCLE) ; // essentially a duty cycle measure
            time_on = time_on > SUNPOINT_DUTY_CYCLE ? SUNPOINT_DUTY_CYCLE : time_on ; // safety measure            
            int time_off = SUNPOINT_DUTY_CYCLE - time_on ;
            int FiringTime = COARSE_TIME_STEP - COARSE_MEASURE_TIME ; // time allowed to fire
            DECLARE_VECTOR(fire, int) ; 
            z_fire = 1 ; // z direction is the only direction of fire
            while(FiringTime > 0){
                HBRIDGE_ENABLE(fire);
                usleep(time_on) ;
                HBRIDGE_DISABLE(3) ; // 3 == executes default, turns off ALL hbridges (safety)
                usleep(time_off) ;
                FiringTime -= SUNPOINT_DUTY_CYCLE ;
            }
            usleep(FiringTime + SUNPOINT_DUTY_CYCLE) ; // sleep for the remainder of the time
            break ;

            case SH_ACS_FINE: // coarse sun pointing
            DECLARE_VECTOR(currB, float) ;
            DECLARE_VECTOR(currBNorm, float) ;
            DECLARE_VECTOR(currL, float) ;
            DECLARE_VECTOR(currLNorm, float) ;
            DECLARE_VECTOR(currS, float) ;
            DECLARE_VECTOR(currSNorm, float) ;
            VECTOR_OP(currB, currB, g_B[mag_index], +) ; // get current magfield
            NORMALIZE(currBNorm, currB); // normalize current magfield
            MATVECMUL(currL, MOI, g_W[omega_index]); // calculate current angular momentum
            VECTOR_OP(currS, currS, g_S[sol_index], +) ; // get current sunvector
            NORMALIZE(currSNorm, currS) ; // normalize sun vector
            // calculate S_B_hat
            DECLARE_VECTOR(SBHat, float);
            float SdotB = DOT_PRODUCT(currSNorm, currBNorm) ;
            VECTOR_MIXED(SBHat, currBNorm, SdotB, *) ;
            VECTOR_OP(SBHat, currSNorm, SBHat, - ) ;
            NORMALIZE(SBHat, SBHat) ;
            // calculate L_B_hat
            DECLARE_VECTOR(LBHat, float) ;
            float LdotB = DOT_PRODUCT(currL, currBNorm) ;
            VECTOR_MIXED(LBHat, currBNorm, LdotB, *) ;
            VECTOR_OP(LBHat, currL, LBHat, -) ;
            NORMALIZE(LBHat, LBHat) ;
            // cross product the two vectors
            DECLARE_VECTOR(SxBxL, float);
            CROSS_PRODUCT(SxBxL, SBHat, LBHat) ;
            NORMALIZE(SxBxL, SxBxL) ;

            int time_on = (int)(DOT_PRODUCT(SxBxL, currBNorm)*SUNPOINT_DUTY_CYCLE) ; // essentially a duty cycle measure
            time_on = time_on > SUNPOINT_DUTY_CYCLE ? SUNPOINT_DUTY_CYCLE : time_on ; // safety measure
            int time_off = SUNPOINT_DUTY_CYCLE - time_on ;
            int FiringTime = FINE_TIME_STEP - FINE_MEASURE_TIME ; // time allowed to fire
            DECLARE_VECTOR(fire, int) ; 
            z_fire = 1 ; // z direction is the only direction of fire
            while(FiringTime > 0){
                HBRIDGE_ENABLE(fire);
                usleep(time_on) ;
                HBRIDGE_DISABLE(3) ; // 3 == executes default, turns off ALL hbridges (safety)
                usleep(time_off) ;
                FiringTime -= SUNPOINT_DUTY_CYCLE ;
            }
            usleep(FiringTime + SUNPOINT_DUTY_CYCLE) ; // sleep for the remainder of the time
            break ;
        }
        pthread_cond_broadcast(&cond_data_ack);
    } while(1) ;
    pthread_exit((void*)NULL) ;
}