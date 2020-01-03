#ifndef ACS_H
#define ACS_H

#include <main.h>

void *acs(void *id);

#define DIPOLE_MOMENT 0.21 // A m^2

#define MAX_DETUMBLE_FIRING_TIME DETUMBLE_TIME_STEP - MAG_MEASURE_TIME

#define MIN_DETUMBLE_FIRING_TIME 10000 // 10 ms

// 3 element array sort, first array is guide that sorts both arrays
void insertionSort(int a1[], int a2[])
{
  for (int step = 1; step < 3; step++)
  {
    int key1 = a1[step];
    int key2 = a2[step];
    int j = step - 1;
    while (key1 < a1[j] && j >= 0)
    {
      // For descending order, change key<array[j] to key>array[j].
      a1[j + 1] = a1[j];
      a2[j + 1] = a2[j];
      --j;
    }
    a1[j + 1] = key1;
    a2[j + 1] = key2;
  }
}

#define HBRIDGE_ENABLE(name) \
  hbridge_enable(x_##name, y_##name, z_##name);

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

#endif // ACS_H