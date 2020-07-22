/**
 * @file xband.c
 * @author Sunip K. Mukherjee (sunipkmukherjee@gmail.com)
 * @brief X-Band Radio interface code
 * @version 0.1
 * @date 2020-03-19
 * 
 * @copyright Copyright (c) 2020
 * 
 */
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <moving_avg.h>
#include <adar1000.h>
#include <xband.h>

int beam_spiral_search(adar1000 devs[], float phase_out[], float *rssi_last)
{
    float rssi_arr[128*128], phases[16];
    if (sizeof(phase_out) < 16 * sizeof(float))
    {
        printf("beam_spiral_search: Memory error on output\n");
        return -1;
    }
    float max_rssi = -120; // start at the noise floor
    int max_loop = SPIRAL_MAX_LOOP;
    do
    {
        int indx = -1;
        for (int i = 0; i < 128; i++)
        {
            for (int j = 0; j < 128; j++)
            {
                float px, py;
                px = 360 * PATCH_DIST * sin(i*PHASE_STEP) / LAM;
                py = 360 * PATCH_DIST * sin(j*PHASE_STEP) / LAM;

                phases[0] = 0 + 90; // 1,1
                phases[1] = px; // 1,2
                phases[2] = 2*px + 90; // 1,3
                phases[3] = 3*px; // 1,4

                phases[4] = py; // 2,1
                phases[5] = px + py + 90; // 2,2
                phases[6] = 2*px + py; // 2,3
                phases[7] = 3*px + py + 90; // 2,4

                phases[8] = 2*py + 90; // 3,1
                phases[9] = 2*py + px; // 3,2
                phases[10] = 2*py + 2*px + 90; // 3,3
                phases[11] = 2*py + 3*px; // 3,4

                phases[12] = 3*py; // 4,1
                phases[13] = 3*py + px + 90; // 4,2
                phases[14] = 3*py + 2*px; // 4,3
                phases[15] = 3*py + 3*px + 90; // 4,4

                for (int k = 5; k > 0; k--)
                {
                    float tmp = phases[k];
                    tmp = tmp > 360 ? tmp - 360 : tmp;
                    phases[k] = tmp;
                }
                while(adar1000_load_multiple_chns(devs, phases) < 0); // load the phases
                usleep(10000); // sleep 10 ms in the new phase
                rssi_arr[indx++] = get_rssi();
                rssi_arr[indx] = moving_avg(rssi_arr, 128*128, indx);
                *rssi_last = rssi_arr[indx];
                if (rssi_arr[indx] > max_rssi)
                {
                    max_rssi = rssi_arr[indx];
                    memcpy(phase_out, phases, 16*sizeof(float));
                }
            }
        }
    } while(max_rssi <= ABS_MAX_RSSI && max_loop-- > 0);
    return max_loop; // if 0 then the loop broke
}