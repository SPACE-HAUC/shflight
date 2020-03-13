#ifndef __DATAVIS_H
#define __DATAVIS_H

#include <main_helper.h>
#include <shflight_consts.h>

#ifndef PORT
#define PORT 12376
#endif

typedef struct sockaddr sk_sockaddr;

typedef struct
{
    uint8_t mode;               // ACS Mode
    uint64_t step;              // ACS Step
    DECLARE_VECTOR2(B, float);  // magnetic field
    DECLARE_VECTOR2(Bt, float); // B dot
    DECLARE_VECTOR2(W, float);  // Omega
    DECLARE_VECTOR2(S, float);  // Sun vector
} datavis_p;

#define PACK_SIZE sizeof(datavis_p)

typedef union {
    datavis_p data;
    unsigned char buf[sizeof(datavis_p)];
} data_packet;

// datavis thread, sets up a server that listens on port PORT
void *datavis_thread(void *t);

#endif // __DATAVIS_H