#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "lsm9ds1.h"

int main()
{
    lsm9ds1 *mag = (lsm9ds1 *)malloc(sizeof(lsm9ds1));
    snprintf(mag->fname, 40, "/dev/i2c-1");
    lsm9ds1_init(mag, 0x6b, 0x1e);
    while(1)
    {
        short B[3];
        lsm9ds1_read_mag(mag, B);
        printf("%d %d %d\n", B[0], B[1], B[2]);
        usleep(100000);
    }
    return 0;
}