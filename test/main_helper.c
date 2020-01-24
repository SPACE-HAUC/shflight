#include <main_helper.h>
#ifndef DEBUG_ALWAYS_S0

int bootCount()
{
    int _bootCount = 0;                      // assume 0 boot
    if (access(BOOTCOUNT_FNAME, F_OK) != -1) //file exists
    {
        FILE *fp;
        fp = fopen(BOOTCOUNT_FNAME, "r+");              // open file for reading
        int read_bytes = fscanf(fp, "%d", &_bootCount); // read bootcount
        if (read_bytes < 0)
            perror("File not read");
        fclose(fp);                       // close
        fp = fopen(BOOTCOUNT_FNAME, "w"); // reopen to overwrite
        fprintf(fp, "%d", ++_bootCount);  // write var+1
        fclose(fp);                       // close
        sync();                           // sync file system
    }
    else //file does not exist, create it
    {
        FILE *fp;
        fp = fopen(BOOTCOUNT_FNAME, "w"); // open for writing
        fprintf(fp, "%d", ++_bootCount);  // write 1
        fclose(fp);                       // close
        sync();                           // sync file system
    }
    return --_bootCount; // return 0 on first boot, return 1 on second boot etc
}
#else
int bootCount()
{
    return 0;
}
#endif // DEBUG_ALWAYS_S0

#ifndef MATH_SQRT
float q2isqrt(float x)
{
    float xhalf = x * 0.5f;               // calculate 1/2 x before bit-level changes
    int i = *(int *)&x;                   // convert float to int for bit level operation
    i = 0x5f375a86 - ((*(int *)&x) >> 1); // bit level manipulation to get initial guess (ref: http://www.lomont.org/papers/2003/InvSqrt.pdf)
    x = *(float *)&i;                     // convert back to float
    x = x * (1.5f - xhalf * x * x);       // 1 round of Newton approximation
    x = x * (1.5f - xhalf * x * x);       // 2 round of Newton approximation
    x = x * (1.5f - xhalf * x * x);       // 3 round of Newton approximation
    return x;
}
#else  // MATH_SQRT
float q2isqrt(float x)
{
    return 1.0 / sqrt(x);
};
#endif // MATH_SQRT

// returns time elapsed from 1970-1-1, 00:00:00 UTC to now (UTC) in microseconds.
uint64_t get_usec(void)
{
    struct timespec ts;
    timespec_get(&ts, TIME_UTC);
    return (uint64_t)ts.tv_sec * 1000000L + ((uint64_t)ts.tv_nsec) / 1000;
}