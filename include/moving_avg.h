/**
 * @file moving_avg.h
 * @author Sunip K. Mukherjee (sunipkmukherjee@gmail.com)
 * @brief Moving average using exponential weights
 * @version 0.1
 * @date 2020-03-19
 * 
 * @copyright Copyright (c) 2020
 * 
 */

#define MOVING_AVG_SIZE 128*4

float moving_avg(float arr[], int len, int indx);
void moving_avg_index(float forget_factor);