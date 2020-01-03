#include <spacehauc_devices.h>
int hbridge_init(ncv7708 *dev)
{
    int status = ncv7708_init(dev);
    if (status < 0)
        return status;
    dev->pack->hben1 = 1;
    dev->pack->hben2 = 1;
    dev->pack->hben3 = 1;
    dev->pack->hben4 = 1;
    dev->pack->hben5 = 1;
    dev->pack->hben6 = 1;
    return ncv7708_xfer(dev); // enable all the half-bridges
}