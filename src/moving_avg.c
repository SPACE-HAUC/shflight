#include <math.h>
#include <moving_avg.h>
static float moving_weights[MOVING_AVG_SIZE];

void moving_avg_index(float forget_factor)
{
    forget_factor = 1-forget_factor;
    for (int i = 0; i < MOVING_AVG_SIZE; i++)
        moving_weights[i] = exp(-i*forget_factor);
    return;
}

float moving_avg(float arr[], int len, int indx)
{
    float avg = 0; int i;
    for (i = 0; indx - i >= 0; i++)
        avg += moving_weights[i]*arr[indx-i];
    return avg/i;
}