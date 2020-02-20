#include <main.h>

#ifndef SERIAL_COMM
int hbridge_enable(int x, int y, int z)
{
    // Set up X
    hbridge->pack->hbcnf1 = x > 0 ? 1 : 0;
    hbridge->pack->hbcnf2 = x < 0 ? 1 : 0;
    // Set up Y
    hbridge->pack->hbcnf3 = y > 0 ? 1 : 0;
    hbridge->pack->hbcnf4 = y < 0 ? 1 : 0;
    // Set up Z
    hbridge->pack->hbcnf5 = z > 0 ? 1 : 0;
    hbridge->pack->hbcnf6 = z < 0 ? 1 : 0;
    return ncv7708_xfer(hbridge);
}

int HBRIDGE_DISABLE(int num)
{
    switch (num)
    {
    case 0: // X axis
        hbridge->pack->hbcnf1 = 0;
        hbridge->pack->hbcnf2 = 0;
        break;

    case 1: // Y axis
        hbridge->pack->hbcnf3 = 0;
        hbridge->pack->hbcnf4 = 0;
        break;

    case 2: // Z axis
        hbridge->pack->hbcnf5 = 0;
        hbridge->pack->hbcnf6 = 0;
        break;

    default: // disable all
        hbridge->pack->hbcnf1 = 0;
        hbridge->pack->hbcnf2 = 0;
        hbridge->pack->hbcnf3 = 0;
        hbridge->pack->hbcnf4 = 0;
        hbridge->pack->hbcnf5 = 0;
        hbridge->pack->hbcnf6 = 0;
        break;
    }
    return ncv7708_xfer(hbridge);
}
#else
int hbridge_enable(int x, int y, int z)
{
    uint8_t val = 0x00;
    // Set up Z
    val |= z > 0 ? 0x01 : (z < 0 ? 0x02 : 0x00);
    val <<= 2;
    // printf("HBEnable: Z: 0b");
    // fflush(stdout);
    // print_bits(val);
    // printf(" ");
    // fflush(stdout);
    // Set up Y
    val |= y > 0 ? 0x01 : (y < 0 ? 0x02 : 0x00);
    val <<= 2;
    // printf("HBEnable: Y: 0b");
    // fflush(stdout);
    // print_bits(val);
    // printf(" ");
    // fflush(stdout);
    // Set up X
    val |= x > 0 ? 0x01 : (x < 0 ? 0x02 : 0x00);
    // printf("HBEnable: X: 0b");
    // fflush(stdout);
    // print_bits(val);
    // printf("\n");
    // fflush(stdout);
    pthread_mutex_lock(&serial_write);
    g_Fire = val;
    pthread_mutex_unlock(&serial_write);
    // printf("HBEnable: %d %d %d: 0x%x\n", x, y, z, g_Fire);
    return val;
}
// disable selected channel on the hbridge
int HBRIDGE_DISABLE(int i)
{
    int tmp = 0xff;
    tmp ^= 0x03 << 2 * i;
    // printf("HBDisable: tmp: 0b");
    // fflush(stdout);
    // print_bits(tmp);
    // fflush(stdout);
    pthread_mutex_lock(&serial_write);
    g_Fire &= tmp;
    pthread_mutex_unlock(&serial_write);
    // printf("HBDisable: 0b");
    // fflush(stdout);
    // print_bits(g_Fire);
    // printf("\n");
    // fflush(stdout);
    return tmp;
}
#endif // SERIAL_COMM

