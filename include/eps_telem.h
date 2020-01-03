#ifndef EPS_TELEM
#define EPS_TELEM
#include <stdint.h>
#include <time.h>

/*
 * TODO:
 * 1. ~Test endianness~ In all probability we need to implement an inverted memcpy operation to deal with bigendianness of I2C slave transactions.
 * 2. Implement the EPS telemetry thread
 * 3. ~Implement mutex locking on the p31u device so that telemetry thread and data_ack thread (resets WDT) does not
 *    crash together.~
 * 4. ~Figure out how to declare the mutex variables so that multiple definition problems are avoided.~
 * 5. Implement the rest of the functions in eps_telem.h
 */

#define EPS_I2C_ADDR 0x7d        // Temporary designation
#define EPS_I2C_BUS "/dev/i2c-0" // I2C bus

// Enumeration describing the return error codes of an EPS I2C transaction
typedef enum
{
    EPS_I2C_READ_FAILED = -20,
    EPS_I2C_WRITE_FAILED,
    EPS_COMMAND_FAILED,
    EPS_COMMAND_SUCCESS = 1
} eps_xfer_ret_t;

/*
 * Thread that runs the EPS telemetry and is involved in watchdog timing.
 */
void *eps_telem(void *id); // EPS Telemetry Thread

/* Data Structures */
typedef struct
{
    uint16_t pv[3];         // Photo-voltaic input voltage [mV]
    uint16_t pc;            // Total photo current [mA]
    uint16_t bv;            // Battery voltage [mV]
    uint16_t sc;            // Total system current [mA]
    int16_t temp[4];        // Temp. of boost converters (1,2,3) and onboard battery [degC]
    int16_t batt_temp[2];   // External board battery temperatures [degC]
    uint16_t latchup[6];    // Number of latch-ups on each output 5V and +3V3 channel
                            // Order[5V1 5V2 5V3 3.3V1 3.3V2 3.3V3]
                            // Transmit as 5V1 first and 3.3V3 last
    uint8_t reset;          // Cause of last EPS reset
    uint16_t bootcount;     // Number of EPS reboots
    uint16_t sw_errors;     // Number of errors in the eps software
    uint8_t ppt_mode;       // 1 = MPPT, 2 = Fixed SW PPT.
    uint8_t channel_status; // Mask of output channel status, 1=on, 0=off
                            // MSB - [QH QS 3.3V3 3.3V2 3.3V1 5V3 5V2 5V1] - LSB
                            //  QH = Quadbat heater, QS = Quadbat switch

} hkparam_t;

typedef struct __attribute__((packed))
{
    uint16_t vboost[3];            //! Voltage of boost converters [mV] [PV1, PV2, PV3]
    uint16_t vbatt;                //! Voltage of battery [mV]
    uint16_t curin[3];             //! Current in [mA]
    uint16_t cursun;               //! Current from boost converters [mA]
    uint16_t cursys;               //! Current out of battery [mA]
    uint16_t reserved1;            //! Reserved for future use
    uint16_t curout[6];            //! Current out (switchable outputs) [mA]
    uint8_t output[8];             //! Status of outputs**
    uint16_t output_on_delta[8];   //! Time till power on** [s]
    uint16_t output_off_delta[8];  //! Time till power off** [s]
    uint16_t latchup[6];           //! Number of latch-ups
    uint32_t wdt_i2c_time_left;    //! Time left on I2C wdt [s]
    uint32_t wdt_gnd_time_left;    //! Time left on I2C wdt [s]
    uint8_t wdt_csp_pings_left[2]; //! Pings left on CSP wdt
    uint32_t counter_wdt_i2c;      //! Number of WDT I2C reboots
    uint32_t counter_wdt_gnd;      //! Number of WDT GND reboots
    uint32_t counter_wdt_csp[2];   //! Number of WDT CSP reboots
    uint32_t counter_boot;         //! Number of EPS reboots
    int16_t temp[6];               //! Temperatures [degC] [0 = TEMP1, TEMP2, TEMP3, TEMP4, BP4a, BP4b]
    uint8_t bootcause;             //! Cause of last EPS reset
    uint8_t battmode;              //! Mode for battery [0 = initial, 1 = undervoltage, 2 = safemode, 3 = nominal, 4=full]
    uint8_t pptmode;               //! Mode of PPT tracker [1=MPPT, 2=FIXED]
    uint16_t reserved2;
} eps_hk_t;

typedef struct __attribute__((packed))
{
    uint16_t vboost[3]; //! Voltage of boost converters [mV] [PV1, PV2, PV3]
    uint16_t vbatt;     //! Voltage of battery [mV]
    uint16_t curin[3];  //! Current in [mA]
    uint16_t cursun;    //! Current from boost converters [mA]
    uint16_t cursys;    //! Current out of battery [mA]
    uint16_t reserved1; //! Reserved for future use
} eps_hk_vi_t;

typedef struct __attribute__((packed))
{
    uint16_t curout[6];           //! Current out (switchable outputs) [mA]
    uint8_t output[8];            //! Status of outputs**
    uint16_t output_on_delta[8];  //! Time till power on** [s]
    uint16_t output_off_delta[8]; //! Time till power off** [s]
    uint16_t latchup[6];          //! Number of latch-ups
} eps_hk_out_t;
// BP4a and BP4b are temperatures on quat battery BP4
// [6] is BP4 heater, [7] is BP4 switch
typedef struct __attribute__((packed))
{
    uint32_t wdt_i2c_time_left;    //! Time left on I2C wdt [s]
    uint32_t wdt_gnd_time_left;    //! Time left on I2C wdt [s]
    uint8_t wdt_csp_pings_left[2]; //! Pings left on CSP wdt
    uint32_t counter_wdt_i2c;      //! Number of WDT I2C reboots
    uint32_t counter_wdt_gnd;      //! Number of WDT GND reboots
    uint32_t counter_wdt_csp[2];   //! Number of WDT CSP reboots
} eps_hk_wdt_t;

typedef struct __attribute__((packed))
{
    uint32_t counter_boot; //! Number of EPS reboots
    int16_t temp[6];       //! Temperatures [degC] [0 = TEMP1, TEMP2, TEMP3, TEMP4, BATT0, BATT1]
    uint8_t bootcause;     //! Cause of last EPS reset
    uint8_t battmode;      //! Mode for battery [0 = initial, 1 = undervoltage, 2 = safemode, 3 = nominal, 4=full]
    uint8_t pptmode;       //! Mode of PPT tracker [1=MPPT, 2=FIXED]
    uint16_t reserved2;
} eps_hk_basic_t;

/* End Data Structures */

/* 
 * I2C Slave Mode:
 * 
 *           <start><addr + slave wr (1)><cmd (1)><req data (n)><start><addr + slave r (1)>|<cmd (1)><err (1)><data (m)><stop>
 * 
 * Byte order: Big endian
 * 10 ms till reply is ready
 */

typedef enum
{
    PING = 1,            // 1 byte, 0 data in, 1 byte out, 0 data out
    REBOOT = 4,          // 1 byte, 0x80 0x07 0x80 0x07 in, 0 out (reboot!)
    GET_HK = 8,          // request empty, reply struct hkparam_t
                         // request 0x00, reply eps_hk_t
                         // request 0x01, reply eps_hk_vi_t
                         // request 0x02, reply eps_hk_out_t
                         // request 0x03, reply eps_hk_wdt_t
                         // request 0x04, reply eps_hk_basic_t
    SET_OUTPUT,          // request 0x[XX], reply none
                         // Set output switch states by a bitmask where 1 means channel is on, 0 means channel is off
                         // LSB is channel 1, next bit is channel 2 etc
                         // MSB -- NC NC 3V3_3 3V3_2 3V3_1 5V_3 5V_2 5V_1 --LSB
    SET_SINGLE_OUTPUT,   // request: byte channel, byte value, int16 delay. Reply: None
                         // set output %channel% to value %value% with delay,
                         // channel (0-5), BP4 heater (6), BP4 switch (7)
                         // Value 0 = off, 1 = on; delay in seconds
    SET_PV_VOLT,         // request: uint16 voltage1, voltage2, voltage3, reply: none
                         // set the voltage on the photovoltaic inputs V1, V2, V3 in mV
                         // takes effect in MODE=2 (see SET_PV_AUTO)
                         // transmit voltage1 first and voltage3 last.
    SET_PV_AUTO,         // request: uint8 MODE, reply: none
                         // Sets the solar cell power tracking mode.
                         // MODE = 0: Hardware default power point
                         // MODE = 1: Maximum power point tracking
                         // MODE = 2: Fixed software power point, value set with SET_PV_VOLT, default 4V
    SET_HEATER,          // request: uint8 cmd, heater, mode; reply: uint8 bp4_heater, uint8 onboard_heater
                         // cmd = 0: set heater on/off; Heater: 0 = BP4, 1 = Onboard, 2 = Both; Mode: 0 = OFF, 1 = ON
                         // Replies with heater modes. 0=Off, 1 = On. To do only query, send an empty message
    RESET_COUNTERS = 15, // request: 0x42. reply: none
                         // Send this command to reset boot counters and WDT counters
    RESET_WDT,           // request: 0x78, reply: None.
                         // Use this command to reset (kick) the dedicated WDT.
    CONFIG_CMD,          // request: uint8 cmd; reply: none
                         //  cmd = 1: restore default config
    CONFIG_GET,          // request: None, reply: struct eps_config_t
    CONFIG_SET,          // request: struct eps_config_t, reply: none
    HARD_RESET,          // request: none, reply: none
                         // Hard reset of the P31 cycles permament 5V, 3V3 and battery outputs.
    CONFIG2_CMD,         // request: uint8 cmd, reply: none
                         // Controls the config 2 system.
                         // cmd = 1: restore default
                         // cmd = 2: confirm current
    CONFIG2_GET,         // request: none, reply: struct eps_config2_t
    CONFIG2_SET,         // request: struct eps_config2_t, reply: none
                         // REQUIRES CONFIRMATION
    CONFIG3 = 25         // cmd = 1: Restore default config3,
                         // cmd = 2: confirm current config3,
                         // cmd = 3: get config3,
                         // cmd = 4: set config3
} eps_commands;

/* Configuration Data Structures */
typedef struct __attribute__((packed))
{
    uint8_t ppd_mode;                     //! Mode for PPT [1 = AUTO, 2 = FIXED]
    uint8_t battheater_mode;              //! Mode for battheater [0 = MANUAL, 1 = AUTO]
    int8_t battheater_low;                //! Turn heater on at [degC]
    int8_t battheater_high;               //! Turn off heater at [degC]
    uint8_t output_normal_value[8];       //! Nominal mode output value
    uint8_t output_safe_value[8];         //! Safe mode output value
    uint16_t output_initial_on_delay[8];  //! Output switches: init with these on delays [s]
    uint16_t output_initial_off_delay[8]; //! Output switches: init with these off delays [s]
    uint16_t vboost[3];                   //! Fixed PPT point for boost converters [mV]
} eps_config_t;
typedef struct __attribute__((packed))
{
    uint16_t batt_maxvoltage;
    uint16_t batt_safevoltage;
    uint16_t batt_criticalvoltage;
    uint16_t batt_normalvoltage;
    uint32_t reserved1[2];
    uint8_t reserved2[4];
} eps_config2_t;
typedef struct __attribute__((packed))
{
    uint8_t version;
    uint8_t cmd; // cmd = 1: Restore default config3,
                 // cmd = 2: confirm current config3,
                 // cmd = 3: get config3,
                 // cmd = 4: set config3
    uint8_t length;
    uint8_t flags;
    uint16_t cur_lim[8];
    uint8_t cur_ema_gain;
    uint8_t cspwdt_channel[2];
    uint8_t cspwdt_address[2];
} eps_config3_t;
/* Configuration Data Structures */

/* Output channels */
typedef union {
    struct
    {
        // MSB - [QH QS 3.3V3 3.3V2 3.3V1 5V3 5V2 5V1] - LSB
        uint8_t V5_1 : 1;
        uint8_t V5_2 : 1;
        uint8_t V5_3 : 1;

        uint8_t V3_1 : 1;
        uint8_t V3_2 : 1;
        uint8_t V3_3 : 1;

        uint8_t qs : 1;
        uint8_t qh : 1;
    };
    uint8_t reg;
} channel_t;

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <linux/i2c-dev.h>
#include <i2c/smbus.h>
#include <errno.h>

typedef struct
{
    int file;                    //! I2C File Descriptor
    char fname[40];              //! I2C File Name
    uint8_t addr;                //! Device Address
    hkparam_t hkparam;           //! hkparam_t structure memory
    eps_hk_t full_hk;            //! Full housekeeping data
    eps_hk_vi_t battpower_hk;    //! battery voltage and current data
    eps_hk_out_t outstats_hk;    //! Output status and current data
    eps_hk_wdt_t wdtstats_hk;    //! Watchdog status data
    eps_hk_basic_t basicstas_hk; //! bootcount, powerup, battery status etc
} p31u;

/*
 * Initializes the EPS device file descriptor for communication over the I2C bus.
 */
int p31u_init(p31u *);
/*
 * Close the EPS I2C file descriptor and free memory for the EPS device.
 */
void p31u_destroy(p31u *);
/* 
 * Write input buffer of size insize and read reply of expected size outsize in output buffer.
 * This is the fundamental I2C transfer function that is used to communicate with the EPS.
 * insize < 1 indicates that no reply is required and the function returns immediately after write.
 */
int p31u_xfer(p31u *, char *, ssize_t, char *, ssize_t);
/*
 * Ping the EPS for response
 * TODO: Ensure if this is equivalent to a watchdog kick
 */
int eps_ping(p31u *);
/*
 * Reboot the EPS
 */
int eps_reboot(p31u *);
/*
 * Get housekeeping data in mode 0x[xx]
 */
int eps_get_hk(p31u *, uint8_t);
/*
 * Get housekeeping data specified in hkparams_t
 */
int eps_hk(p31u *);
/*
 * Toggle outputs on all channels
 */
int eps_set_output(p31u *, channel_t);
/*
 * Toggle output on a single channel.
 * Channel: (0--5) for 3V3 and 5V channels, BP4 heater (6), BP4 switch (7)
 * Value: 1 -- On, 0 -- Off
 * Delay in seconds
 * 
 * TODO: Endianness confirmation. This function can be especially helpful!
 */
int eps_set_single(p31u *, uint8_t, uint8_t, int16_t);
/*
 * Set the voltage on the photovoltaic inputs V1, V2, V3 in mV. Takes effect when MODE=2.
 * Transmits V1 first and V3 last.
 */
int eps_set_pv_volt(p31u *, uint16_t, uint16_t, uint16_t);
/*
 * Sets the solar cell power tracking mode:
 * MODE = 0 : Hardware default power point
 * MODE = 1 : Maximum power point tracking
 * MODE = 2 : Fixed software power point, value set with SET_PV_VOLT, default 4V
 */
int eps_set_pv_mode(p31u *, uint8_t);
/*
 * Sets the heaters specified.
 * Cmd = 0 : Set heater on/off
 * Heater: 0 = BP4; 1 = Onboard; 2 = Both
 * Mode: 0 = OFF, 1 = ON 
 * 
 * Returns uint8 bp4_heater and uint8 onboard_heater packed in uint16, lower byte is bp4 and higher byte is onboard.
 * 
 * TODO: Clarify what cmd and mode are. They seem similar.
 */
int eps_set_heater(p31u *, uint8_t cmd, uint8_t heater, uint8_t mode, uint16_t *output);
/*
 * Reset boot and WDT counter
 */
int eps_reset_counters(p31u *);
/*
 * Reset (kick) WDT
 */
int eps_reset_wdt(p31u *);
/*
 * Config command. cmd = 1 implies restore default.
 * 
 * TODO: Clarify if we need to set cmd = 0 to activate a config sent using CONFIG_SET
 */
int eps_config_cmd(p31u *, uint8_t);
/*
 * Get current config. Stores the reply struct in the p31u structure.
 */
int eps_config_get(p31u *);
/*
 * Set a configuration (eps_config_t) passed into the function.
 */
int eps_config_set(p31u *, eps_config_t);
/*
 * Hard reset the P31u. This power cycles every output including the battery.
 */
int eps_hard_reset(p31u *);
/*
 * Config command. cmd = 1 implies restore default.
 * 
 * TODO: Clarify if we need to set cmd = 0 to activate a config sent using CONFIG_SET
 */
int eps_config2_cmd(p31u *, uint8_t);
/*
 * Get current config. Stores the reply struct in the p31u structure.
 */
int eps_config2_get(p31u *);
/*
 * Set a configuration (eps_config2_t) passed into the function.
 */
int eps_config2_set(p31u *, eps_config2_t);
/*
 * Controls the config3 system.
 */
int eps_config3(p31u *, eps_config3_t);
/*
 * Global EPS interface
 */
extern p31u *g_eps;
/*
 * 
 */
#endif