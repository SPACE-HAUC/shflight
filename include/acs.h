#ifndef ACS_H
#define ACS_H

#define DIPOLE_MOMENT 0.21 // A m^2

#define MAX_DETUMBLE_FIRING_TIME DETUMBLE_TIME_STEP-MAG_MEASURE_TIME

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
      a2[j + 1] = a2[j] ;
      --j;
    }
    a1[j + 1] = key1;
    a2[j + 1] = key2;
  }
}


#endif // ACS_H