[![Build Status](https://travis-ci.org/SPACE-HAUC/shflight.svg?branch=master)](https://travis-ci.org/SPACE-HAUC/shflight) [![License: AGPL v3](https://img.shields.io/badge/license-GPLv3-green)](https://github.com/SPACE-HAUC/shflight/blob/master/LICENSE)

# SPACE HAUC Flight Code
Software to control the SPACE HAUC satellite. It is implemented as a Linux userspace program, and ideally provides the design guidelines for a multi-module system. The modules are to be implemented as POSIX threads, with complete control over variable and function scoping -- so that only relevant modules can communicate, reducing programming complications and increasing maintainability of the individual modules. Modules can be registered into the main program through the `modules.h` file and a `module_iface.h` header corresponding to the module. It is encouraged that the modules be written following C99 or C11 standards.
## Last stable tested commit: `release` branch
## Current status
The following major features have been implemented:
1. Threaded code (split into different files)
2. `make` code generation system (TODO: Transition to `cmake`)
3. ACS detumble and sunpointing algorithms
4. Serial communication for SITL (Software In The Loop) testing
5. ACS devices have been added for HITL (Hardware In The Loop) testing
6. External data visualization over TCP using a Python frontend
7. External data visualization for Simulink data over Serial + TCP using Python frontend
8. Complete Doxygen documentation and travis build checker support.
9. The code has been redesigned to make adding modules easier, with variable and function scoping support within the realm of C99, with different modules being aware of only the variables and functions that need to be used. Refer to the design document (`shflight-design.pdf`) for more information. `refman.pdf` includes the latest documentation for the project.

## `make` Options:

1. `make`: Invokes `all` which is the default compilation option. Does not pass any arguments to the compiler, hence genrates dynamically linked code that runs is compatible with HITL without any sun sensor code.
2. `make sim_server`: Creates the server code that can read Simulink display output over serial port and publish it over TCP for `geode.py` visualization service.
3. `make clean`: Delete all the object files and the built code.
4. `make spotless`: Remove every object file, build directory etc.
5. `make doc`: Create doxygen documentation.
6. `make pdf`: Make PDF documentation (requires TeXLive 2019 or earlier).

## Program Options:

Program options are still scattered throughout the program. These options can be passed through the `CFLAGS` variable to `make` (e.g. `make CFLAGS="-DCSS_READY"` will enable coarse sun sensor support in the code). Here is a list of different compile switches that turns on/off different features:

1. `SITL`: Turns on the `sitl_comm` interface for a Software In The Loop test.
2. `DATAVIS`: Turns on the `datavis` service to display system performance externally.
3. `PORT`: Requires an input of the form of an integer, assigns port for the DataVis thread.
4. `CSS_READY`: Turns on coarse sun sensor related code in the software for HITL/production.
5. `FSS_READY`: Turns on fine sun sensor related code in the software for HITL/production (partial support).
6. `I2C_BUS`: Requires an input of the form of a string pointing to the absolute path of the I2C device file.
7. `SPIDEV_ACS`: Requires an input of the form of a string pointing to the absolute path of the SPI device file.
8. `ACS_DATALOG`: Writes ACS data to a file.
9. `ACS_PRINT`: Prints ACS status to `stdout`.



There is a hidden option in `drivers/tsl2561.c` that enables the true low-gain operation of the coarse sun sensors. The true low-gain operation is currently disabled to support the calibration that was last performed on the coarse sun sensors.

## Quirks (and TO-DOs)
The following quirks are present in the code as of now:
### Serial Communication
1. Simulink is running in real time mode using `Packet` output blocks.
2. The baud rate being low (230400 bps == ~1.7 ms for 40 bytes of data) could be a possible reason for the apparent lack of synchronization. In this case, the `sitl_comm` thread should also time (and synchronize itself) to the simulation. Look into such possibilities.
3. Currently due to the synchronization problems the `acs_detumble` thread waits on wakeup from the `sitl_comm` thread to guarantee a basic form of synchronization with the Simulation.
4. For HITL, no such synchronization is necessary and the flight code can operate outside of the realm of Simulink.

### ACS Detumble Algorithm
1. Magnetic field is represented in milliGauss to enhance math precision.
2. Omega measurement does not include the second order correction term that uses the MOI and past measurement. This corrected value of omega should be passed through a Bessel filter.
3. Investigate if every sensor reading should be filtered using a low pass filter. Discuss the cutoff frequency for such a filter.
4. Investigate implementation of a Kalman filter instead of a Bessel function.
5. In HITL, due to the noise Bessel filtering is used on B, dB/dt and $\omega$ which leads to a bias on $\omega \cdot z$. This throws off the detumble determination. Find a better filter/criterion.
6. Investigate the effect of $\omega_z < 0$ at initialization.

### ACS Sunpointing Algorithm



1. Both FSS and CSS are read. If FSS reading is valid, it is used to determine sun vector.
2. If FSS reading is invalid, CSS readings are used to determine sun vector essentially by subtracting the flux on the negative direction from the positive direction, doing this for all three faces, and then normalizing the resultant vector.
3. Investigate the gain factor in the sunpointing algorithm.
