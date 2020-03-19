[![Build Status](https://travis-ci.org/SPACE-HAUC/shflight.svg?branch=master)](https://travis-ci.org/SPACE-HAUC/shflight) [![codecov](https://codecov.io/gh/SPACE-HAUC/shflight/branch/master/graph/badge.svg)](https://codecov.io/gh/SPACE-HAUC/shflight) [![License: AGPL v3](https://img.shields.io/badge/License-AGPL%20v3-blue.svg?style=flat)](https://github.com/SPACE-HAUC/shflight/blob/master/LICENSE)

# Test code for SPACE HAUC ACS algorithms
## Last stable tested commit: `release` branch
## Current status
The following major features have been implemented:
1. Threaded code (split into different files)
1. `make` code generation system (TODO: Transition to `cmake`)
1. ACS detumble and sunpointing algorithms
1. Serial communication for SITL (Software In The Loop) testing
1. ACS devices have been added for HITL (Hardware In The Loop) testing
1. External data visualization over TCP using a Python frontend
1. External data visualization for Simulink data over Serial + TCP using Python frontend

## `make` Options:

1. `make`: Invokes `all` which is the default compilation option. Does not pass any arguments to the compiler, hence genrates dynamically linked code that runs is compatible with HITL without any sun sensor code.
2. `make sim_server`: Creates the server code that can read Simulink display output over serial port and publish it over TCP for `geode.py` visualization service.
3. `make clean`: Delete all the object files and the built code.
4. `make spotless`: Remove every object file, build directory etc.

## Program Options:

Program options are still scattered throughout the program. These options can be passed through the `CFLAGS` variable to `make` (e.g. `make CFLAGS="-DCSS_READY"` will enable coarse sun sensor support in the code). Here is a list of different compile switches that turns on/off different features:

1. `SITL`: Turns on the `sitl_comm` interface for a Software In The Loop test.
2. `PORT`: Requires an input of the form of an integer, assigns port for the DataVis thread.
3. `CSS_READY`: Turns on coarse sun sensor related code in the software for HITL/production.
4. `FSS_READY`: Turns on fine sun sensor related code in the software for HITL/production (partial support).
5. `I2C_BUS`: Requires an input of the form of a string pointing to the absolute path of the I2C device file.
6. `SPIDEV_ACS`: Requires an input of the form of a string pointing to the absolute path of the SPI device file.
7. `ACS_DATALOG`: Writes ACS data to a file.



There is a hidden option in `drivers/tsl2561.c` that enables the true low-gain operation of the coarse sun sensors. The true low-gain operation is currently disabled to support the calibration that was last performed on the coarse sun sensors.

## Quirks (and TO-DOs)
The following quirks are present in the code as of now:
### Serial Communication
1. ~~The Simulink simulator is not a real time system yet (investigating Real-time execution where `Serial` blocks raise errors; using `Packet` blocks may help.)~~
   Simulink is running in real time mode using `Packet` output blocks.

1. ~~The lack of true real time implies that the serial data needs to be synchronized to the simulation itself to guarantee a functional data stream without any errors.~~
   No synchronization necessary.
1. The baud rate being low (230400 bps == ~1.7 ms for 40 bytes of data) could be a possible reason for the apparent lack of synchronization. In this case, the `sitl_comm` thread should also time (and synchronize itself) to the simulation. Look into such possibilities.
1. Currently due to the synchronization problems the `acs_detumble` thread waits on wakeup from the `sitl_comm` thread to guarantee a basic form of synchronization with the Simulation.
1. For HITL, no such synchronization is necessary and the flight code can operate outside of the realm of Simulink.

### ACS Detumble Algorithm
1. Magnetic field is represented in milliGauss to enhance math precision.
1. Omega measurement does not include the second order correction term that uses the MOI and past measurement. This corrected value of omega should be passed through a Bessel filter.
1. Investigate if every sensor reading should be filtered using a low pass filter. Discuss the cutoff frequency for such a filter.
1. Investigate implementation of a Kalman filter instead of a Bessel function.
1. In HITL, due to the noise Bessel filtering is used on B, dB/dt and ω which leads to a bias on ω·z. This throws off the detumble determination. Find a better filter/criterion.
1. Investigate the effect of $\omega_z < 0$ at initialization.

### ACS Sunpointing Algorithm



1. Both FSS and CSS are read. If FSS reading is valid, it is used to determine sun vector.
2. If FSS reading is invalid, CSS readings are used to determine sun vector essentially by subtracting the flux on the negative direction from the positive direction, doing this for all three faces, and then normalizing the resultant vector.
3. Investigate the gain factor in the sunpointing algorithm.
