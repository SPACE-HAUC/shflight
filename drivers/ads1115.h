/**
 * @file ads1115.h
 * @author Sunip K. Mukherjee (sunipkmukherjee@gmail.com)
 * @brief ADS1115 I2C Driver function prototypes and data structures
 * @version 0.1
 * @date 2020-03-19
 * 
 * @copyright Copyright (c) 2020
 * 
 */

#ifndef ADS1115_H
#define ADS1115_H
#include <stdint.h>
/**
 * @brief Default I2C Address
 * 
 */
#define ADS1115_S_ADDR 0x48 // Device address for ADS1115
/**
 * @brief Default I2C Bus
 * 
 */
#ifndef I2C_BUS
#define I2C_BUS "/dev/i2c-1" // I2C bus name
#endif // I2C_BUS

/* REGISTERS */
#define CONVERSION_REG 0x00 ///< ADC conversion register
#define CONFIG_REG 0x01     ///< ADC configuration register

/**
 * @brief Configuration register
 * 
 */
typedef union {
    struct
    {
        /**
         * @brief Comparator queue and disable (ADS1114 and ADS1115 only)
         * 
         * These bits perform two functions. When set to 11, the comparator is disabled and
         * the ALERT/RDY pin is set to a high-impedance state. When set to any other
         * value, the ALERT/RDY pin and the comparator function are enabled, and the set
         * value determines the number of successive conversions exceeding the upper or
         * lower threshold required before asserting the ALERT/RDY pin. These bits serve
         * no function on the ADS1113.
         * 00 : Assert after one conversion
         * 01 : Assert after two conversions
         * 10 : Assert after four conversions
         * 11 : Disable comparator and set ALERT/RDY pin to high-impedance (default)
         * 
         */
        uint8_t comp_que : 2;
        /**
 * @brief Latching comparator (ADS1114 and ADS1115 only)
 * 
 * This bit controls whether the ALERT/RDY pin latches after being asserted or
 * clears after conversions are within the margin of the upper and lower threshold
 * values. This bit serves no function on the ADS1113.
 * 0 : Nonlatching comparator . The ALERT/RDY pin does not latch when asserted
 * (default).
 * 1 : Latching comparator. The asserted ALERT/RDY pin remains latched until
 * conversion data are read by the master or an appropriate SMBus alert response
 * is sent by the master. The device responds with its address, and it is the lowest
 * address currently asserting the ALERT/RDY bus line.
 * 
 */
        uint8_t comp_lat : 1;
        /**
         * @brief Comparator polarity (ADS1114 and ADS1115 only)
         * 
         * This bit controls the polarity of the ALERT/RDY pin. This bit serves no function on
         * the ADS1113.
         * 0 : Active low (default)
         * 1 : Active high
         * 
         * 
         */
        uint8_t comp_mode : 1;
        /**
         * @brief Comparator mode (ADS1114 and ADS1115 only)
         *
         * This bit configures the comparator operating mode. This bit serves no function on
         * the ADS1113.
         * 0 : Traditional comparator (default)
        * 1 : Window comparator
         * 
         */
        uint8_t comp_pol : 1;
        /**
         * @brief Data rate: These bits control the data rate setting.
         * 000 : 8 SPS
         * 001 : 16 SPS
         * 010 : 32 SPS
         * 011 : 64 SPS
         * 100 : 128 SPS (default)
         * 101 : 250 SPS
         * 110 : 475 SPS
         * 111 : 860 SPS
         * 
         */
        uint8_t dr : 3;
        /**
         * @brief Device operating mode: This bit controls the operating mode.
         * 0 : Continuous-conversion mode
         * 1 : Single-shot mode or power-down state (default)
         * 
         */
        uint8_t mode : 1;
        /**
         * @brief Programmable gain amplifier configuration
         * These bits set the FSR of the programmable gain amplifier. These bits serve no
         * function on the ADS1113.
         * 000 : FSR = ±6.144 V
         * 001 : FSR = ±4.096 V
         * 010 : FSR = ±2.048 V (default)
         * 011 : FSR = ±1.024 V
         * 100 : FSR = ±0.512 V
         * 101 : FSR = ±0.256 V
         * 110 : FSR = ±0.256 V
         * 111 : FSR = ±0.256 V
         * 
         */
        uint8_t pga : 3;
        /**
         * @brief Input multiplexer configuration (ADS1115 only)
         * These bits configure the input multiplexer. These bits serve no function on the
         * ADS1113 and ADS1114.
         * 000 : AINP = AIN0 and AINN = AIN1 (default)
         * 001 : AINP = AIN0 and AINN = AIN3
         * 010 : AINP = AIN1 and AINN = AIN3
         * 011 : AINP = AIN2 and AINN = AIN3
         * 100 : AINP = AIN0 and AINN = GND
         * 101 : AINP = AIN1 and AINN = GND
         * 110 : AINP = AIN2 and AINN = GND
         * 111 : AINP = AIN3 and AINN = GND
         * 
         */
        uint8_t mux : 3;
        /**
         * @brief Operational status or single-shot conversion start
         * This bit determines the operational status of the device. OS can only be written
         * when in power-down state and has no effect when a conversion is ongoing.
         * When writing:
         * 0 : No effect
         * 1 : Start a single conversion (when in power-down state)
         * When reading:
         * 0 : Device is currently performing a conversion
         * 1 : Device is not currently performing a conversion
         * 
         */
        uint8_t os : 1;
    };
    /**
     * @brief Raw 16 bits corresponding to the config struct
     * 
     */
    uint16_t raw;
} ads1115_config;

/**
 * @brief ads1115 device data structures.
 * 
 */
typedef struct
{
    int fd;         ///< Device file descriptor
    char fname[40]; ///< I2C Bus name
} ads1115;
/**
 * @brief Initializes an ADS1115 device. Opens the I2C device named in ads1115->fname.
 * 
 * @param dev Pointer to ads1115 device struct.
 * @param s_address 7-bit I2C address
 * @return Returns 1 on success, -1 on failure. Sets errno.
 */
int ads1115_init(ads1115 *dev, uint8_t s_address);
/**
 * @brief Configures an ADS1115 device.
 * 
 * @param dev Pointer to ads1115 device struct.
 * @param m_con Configuration to apply
 * @return Returns 1 on success, -1 on failure.
 */
int ads1115_configure(ads1115 *dev, ads1115_config m_con);
/**
 * @brief Reads data from the ADC in single shot.
 * 
 * @param dev Pointer to ads1115 device struct.
 * @param data Pointer to an array of short of length 4 where data is stored
 * @return Returns 1 on success, -1 on failure.
 */
int ads1115_read_data(ads1115 *dev, int16_t *data);
/**
 * @brief Reads data from the ADC in continuous mode.
 * 
 * @param dev Pointer to ads1115 device struct.
 * @param data Pointer to an array of short of length 4 where data is stored
 * @return Returns 1 on success, -1 on failure.
 */
int ads1115_read_cont(ads1115 *dev, int16_t *data);
/**
 * @brief Read current configuration of an ADS1115.
 * 
 * @param dev Pointer to ads1115 device struct.
 * @param data Pointer to unsigned short (ads1115_config->raw)
 * @return Returns 1 on success, -1 on failure.
 */
int ads1115_read_config(ads1115 *dev, uint16_t *data);
/**
 * @brief Powers down ADS1115 device and closes file descriptor.
 * 
 * @param dev Pointer to ads1115 device struct.
 */
void ads1115_destroy(ads1115 *dev);

#endif // ADS1115_H