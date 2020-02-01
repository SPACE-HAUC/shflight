# Test code for SPACE HAUC ACS algorithms
## Last stable tested commit: `2f8837f`
## Current status
The following major features have been implemented:
1. Threaded code
1. ACS detumble algorithm only
1. Serial communication for SITL (Software In The Loop) testing
1. External data visualization over TCP using a Python frontend

## Quirks (and TO-DOs)
The following quirks are present in the code as of now:
### Serial Communication
1. The Simulink simulator is not a real time system yet (investigating Real-time execution where `Serial` blocks raise errors; using `Packet` blocks may help.)
1. The lack of true real time implies that the serial data needs to be synchronized to the simulation itself to guarantee a functional data stream without any errors.
1. The baud rate being low (230400 bps == ~1.7 ms for 40 bytes of data) could be a possible reason for the apparent lack of synchronization. In this case, the `sitl_comm` thread should also time (and synchronize itself) to the simulation. Look into such possibilities.
1. Currently due to the synchronization problems the `acs_detumble` thread waits on wakeup from the `sitl_comm` thread to guarantee a basic form of synchronization with the Simulation.

### ACS Detumble Algorithm
1. Magnetic field is represented in milliGauss to enhance math precision.
1. Omega measurement does not include the second order correction term that uses the MOI and past measurement. This corrected value of omega should be passed through a Bessel filter.
1. Investigate if every sensor reading should be filtered using a low pass filter. Discuss the cutoff frequency for such a filter.
1. Investigate implementation of a Kalman filter instead of a Bessel function.