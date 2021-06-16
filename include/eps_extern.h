/**
 * @file eps_extern.h
 * @author Mit Bailey (mitbailey99@gmail.com)
 * @brief Externally accessible EPS command handling functions.
 * @version 0.3
 * @date 2021-03-17
 *
 * @copyright Copyright (c) 2021
 *
 */

#ifndef EPS_EXTERN_H
#define EPS_EXTERN_H

#include <pthread.h>
#include <stdarg.h>
#include "eps_p31u/p31u.h"

#ifndef EPS_H
extern __thread pthread_cond_t eps_cmd_wait[1];
extern __thread pthread_mutex_t eps_cmd_wait_m[1];
#endif

/**
 * @brief eps_hk_t global struct
 * 
 */
extern eps_hk_t eps_system_hk;
/**
 * @brief sh_hk_t global struct
 * 
 */
extern sh_hk_t sh_system_hk;

/**
 * @brief Pack important EPS info into 36 bytes
 * 
 * @return typedef struct 
 */
typedef struct __attribute__((packed))
{
    uint16_t vboost[3];            //!< Voltage of boost converters [mV] [PV1, PV2, PV3]
    uint16_t vbatt;                //!< Voltage of battery [mV]
    uint16_t cursun;               //!< Current from boost converters [mA]
    uint16_t cursys;               //!< Current out of battery [mA]
    uint16_t curout[2];            //!< Current out (switchable outputs) [mA]
    uint8_t outputs;               //!< Status of outputs**
    uint8_t knife_on_off_delta;    //!< Upper nibble indicates if thermal knife on delta is not 0, lower niblle indicates if thermal knife off delta is not zero
    uint16_t latchup[2];           //!< Number of latch-ups
    uint8_t wdt_i2c_time_min;      //!< I2C time left in minutes
    uint8_t wdt_i2c_time_sec;      //!< I2C time left in seconds
    uint16_t counter_wdt_i2c;      //!< Number of WDT I2C reboots
    uint16_t counter_boot;         //!< Number of EPS reboots
    int8_t temp[6];                //!< Temperatures [degC] [0 = TEMP1, TEMP2, TEMP3, TEMP4, BP4a, BP4b]
    uint8_t bootcause;             //!< Cause of last EPS reset
    uint8_t battnpptmode;          //!< 0x Mode for battery [0 = initial, 1 = undervoltage, 2 = safemode, 3 = nominal, 4=full] | Mode of PPT tracker [1=MPPT, 2=FIXED]
} sh_hk_t;

/**
 * @brief Convert eps_hk_t struct data to sh_hk_t data
 * @param hk_t Pointer to eps_hk_t struct
 * @param hk Pointer to sh_hk_t struct
 */
void convert_hk_to_sh(eps_hk_t *hk_t, sh_hk_t *hk);

/**
 * @brief Get important housekeeping parameters
 * 
 * @param sh_hk pointer to sh_hk_t struct
 * @return int positive on success, negative on error
 */
int eps_get_min_hk(sh_hk_t *sh_hk);

/**
  * @brief Pings the EPS.
  *
  * @return int Positive on success, negative on failure.
  */
int eps_ping();

/**
  * @brief Reboots the EPS.
  *
  * @return int Value for i2c read / write.
  */
int eps_reboot();

/**
  * @brief Gets basic housekeeping data.
  *
  *
  * @param hk Pointer to hkparam_t object.
  * @return int Value for i2c read / write.
  */
int eps_get_hkparam(hkparam_t *hk);

/**
 * @brief 
 * 
 * @param hk 
 * @return int 
 */
int eps_get_hk(eps_hk_t *hk);

/**
  * @brief Gets basic housekeeping data.
  *
  * @param hk_out Pointer to eps_hk_out_t object.
  * @return int Value for i2c read / write.
  */
int eps_get_hk_out(eps_hk_out_t *hk_out);

/**
  * @brief Toggle EPS latch up.
  *
  * @param lup ID of latchup. 
  * @return int Variable tracking latchup status on success, value for i2c read / write on failure.
  */
int eps_tgl_lup(eps_lup_idx lup);

/**
  * @brief Set EPS latch up.
  *
  * @param lup ID of latchup.
  * @param pw Desired state; 1 for on, 0 for off.
  * @return int Value for eps_p31u_cmd
  */
int eps_lup_set(eps_lup_idx lup, int pw);

/**
 * @brief 
 * 
 * @param tout_ms 
 * @return int 
 */
int eps_battheater_set(uint64_t tout_ms);

/**
 * @brief 
 * 
 * @param tout_ms 
 * @return int 
 */
int eps_ks_set(uint64_t tout_ms);

/**
  * @brief Power cycles all power lines including battery rails.
  *
  * @return int Value for i2c read / write.
  */
int eps_hardreset();

/**
 * @brief Gets the EPS configuration.
 * 
 * @param conf Pointer to eps_config_t object for output.
 * @return int 
 */
int eps_get_conf(eps_config_t *conf);

/**
 * @brief Sets the EPS configuration.
 * 
 * @param conf Pointer to eps_config_t object for input.
 * @return int 
 */
int eps_set_conf(eps_config_t *conf);

/**
 * @brief 
 * 
 * @param conf 
 * @return int 
 */
int eps_get_conf2(eps_config2_t *conf);

/**
 * @brief 
 * 
 * @param conf 
 * @return int 
 */
int eps_set_conf2(eps_config2_t *conf);

/**
 * @brief 
 * 
 * @return int 
 */
int eps_reset_counters();

/**
 * @brief 
 * 
 * @param reply 
 * @param cmd 
 * @param heater 
 * @param mode 
 * @return int 
 */
int eps_set_heater(unsigned char *reply, uint8_t cmd, uint8_t heater, uint8_t mode);

/**
 * @brief 
 * 
 * @param mode 
 * @return int 
 */
int eps_set_pv_auto(uint8_t mode);

/**
 * @brief 
 * 
 * @param V1 
 * @param V2 
 * @param V3 
 * @return int 
 */
int eps_set_pv_volt(uint16_t V1, uint16_t V2, uint16_t V3);

/**
 * @brief 
 * 
 * @param hk 
 * @return int 
 */
int eps_get_hk_2_vi(eps_hk_vi_t *hk);

/**
 * @brief 
 * 
 * @param hk 
 * @return int 
 */
int eps_get_hk_wdt(eps_hk_wdt_t *hk);

/**
 * @brief 
 * 
 * @param hk 
 * @return int 
 */
int eps_get_hk_2_basic(eps_hk_basic_t *hk);

/**
 * @brief Return current power consumption in mW
 * 
 * @return int 
 */
int eps_get_outpower();
/**
 * @brief Return current battery voltage in mV
 * 
 * @return int voltage in mV
 */
int eps_get_vbatt();
/**
 * @brief Return total system current consumption
 * 
 * @return int current in mA
 */
int eps_get_sys_curr();
/**
 * @brief Get average solar panel voltage
 * 
 * @return int voltage in mV
 */
int eps_get_vsun();
/**
 * @brief Get all solar panel (boost converter) voltages in mV
 * 
 * @param vsun 3-element array of uint16_t
 * @return int 1 on success, -1 on failure, -2 on null output array ptr
 */
int eps_get_vsun_all(uint16_t *vsun);
/**
 * @brief Get total input solar current
 * 
 * @return int current in mA
 */
int eps_get_isun();
/**
 * @brief Get EPS loop timeout
 * 
 * @return int timeout in second
 */
int eps_get_loop_timer();
/**
 * @brief Set EPS loop timeout
 * 
 * @param t timeout in second (minimum 1 s, maximum 3600 s)
 * @return int set timeout in second
 */
int eps_set_loop_timer(int t);

#endif // EPS_EXTERN_H