#include <stdio.h>
#include <unistd.h>

#define BOOTCOUNT_FNAME "bootcount_fname.txt"
int bootCount(void);

#define FLUSH_BUFFER(name)                                      \
for(uint8_t counter = SH_BUFFER_SIZE; counter > 0; )            \
{                                                               \                
    counter--;                                                  \
    name##x[counter] = 0;                                       \
    name##y[counter] = 0;                                       \
    name##z[counter] = 0;                                       \
}        

#define CROSS_PRODUCT(dest,s1,s2,loc)                           \
dest##x[loc]=s1##y[loc]*s2##z[loc] - s1##z[loc]*s2##y[loc];     \
dest##y[loc]=s1##z[loc]*s2##x[loc] - s1##x[loc]*s2##z[loc];     \
dest##z[loc]=s1##x[loc]*s2##y[loc] - s1##y[loc]*s2##z[loc];     \