#ifndef SPACEHAUC_DEVICES_H
#define SPACEHAUC_DEVICES_H
#include "ncv7708/ncv7708.h"
#define HBRIDGE_DEV_NAME "/dev/spidev1.0" // temporary device
extern ncv7708 *hbridge;
int hbridge_init(ncv7708 *dev);
#endif // SPACEHAUC_DEVICES_H