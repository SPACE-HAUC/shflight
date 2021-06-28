/**
 * @file eps.c
 * @author Mit Bailey (mitbailey99@gmail.com)
 * @brief Implementation of EPS command handling functions.
 * @version 0.3
 * @date 2021-03-17
 *
 * @copyright Copyright (c) 2021
 *
 */

#define EPS_P31U_PRIVATE
#include "eps_p31u/p31u.h"
#undef EPS_P31U_PRIVATE
#include <eps.h>
#include <main.h>
#include <stdint.h>
#include <stdbool.h>
#include <pthread.h>
#include <unistd.h>
#include "datalogger_extern.h"
#ifdef DATAVIS
#include <datavis_extern.h>
#endif

#define THIS_MODULE "eps"

/* Variable allocation for EPS */

/**
  * @brief The EPS object pointer.
  *
  */
static p31u eps[1];

int eps_ping()
{
    if (eps == NULL)
    {
        return -1;
    }

    return eps_p31u_ping(eps);
}

int eps_reboot()
{
    if (eps == NULL)
    {
        return -1;
    }

    return eps_p31u_reboot(eps);
}

int eps_get_hkparam(hkparam_t *hk)
{
    if (eps == NULL)
    {
        return -1;
    }

    return eps_p31u_get_hkparam(eps, hk);
}

int eps_get_hk(eps_hk_t *hk)
{
    if (eps == NULL)
        return -1;
    return eps_p31u_get_hk(eps, hk);
}

int eps_get_hk_out(eps_hk_out_t *hk_out)
{
    if (eps == NULL)
    {
        return -1;
    }

    return eps_p31u_get_hk_out(eps, hk_out);
}

int eps_tgl_lup(eps_lup_idx lup)
{
    if (eps == NULL)
    {
        return -1;
    }

    return eps_p31u_tgl_lup(eps, lup);
}

int eps_lup_set(eps_lup_idx lup, int pw)
{
    if (eps == NULL)
    {
        return -1;
    }

    return eps_p31u_lup_set(eps, lup, (int)pw);
}

int eps_battheater_set(uint64_t tout_ms)
{
    if (eps == NULL)
        return -1;
    return eps_p31u_battheater_set(eps, tout_ms);
}

int eps_ks_set(uint64_t tout_ms)
{
    if (eps == NULL)
        return -1;
    return eps_p31u_ks_set(eps, tout_ms);
}

int eps_hardreset()
{
    if (eps == NULL)
    {
        return -1;
    }

    return eps_p31u_hardreset(eps);
}

int eps_get_conf(eps_config_t *conf)
{
    return eps_p31u_get_conf(eps, conf);
}

int eps_set_conf(eps_config_t *conf)
{
    return eps_p31u_set_conf(eps, conf);
}

int eps_get_conf2(eps_config2_t *conf)
{
    if (eps == NULL)
        return -1;
    return eps_p31u_get_conf2(eps, conf);
}

int eps_set_conf2(eps_config2_t *conf)
{
    if (eps == NULL)
        return -1;
    return eps_p31u_set_conf2(eps, conf);
}

int eps_reset_counters()
{
    if (eps == NULL)
        return -1;
    return eps_p31u_reset_counters(eps);
}

int eps_set_heater(unsigned char *reply, uint8_t cmd, uint8_t heater, uint8_t mode)
{
    if (eps == NULL)
        return -1;
    return eps_p31u_set_heater(eps, reply, cmd, heater, mode);
}

int eps_set_pv_auto(uint8_t mode)
{
    if (eps == NULL)
        return -1;
    return eps_p31u_set_pv_auto(eps, mode);
}

int eps_set_pv_volt(uint16_t V1, uint16_t V2, uint16_t V3)
{
    if (eps == NULL)
        return -1;
    return eps_p31u_set_pv_volt(eps, V1, V2, V3);
}

int eps_get_hk_2_vi(eps_hk_vi_t *hk)
{
    if (eps == NULL)
        return -1;
    return eps_p31u_get_hk_2_vi(eps, hk);
}

int eps_get_hk_wdt(eps_hk_wdt_t *hk)
{
    if (eps == NULL)
        return -1;
    return eps_p31u_get_hk_wdt(eps, hk);
}

int eps_get_hk_2_basic(eps_hk_basic_t *hk)
{
    if (eps == NULL)
        return -1;
    return eps_p31u_get_hk_2_basic(eps, hk);
}

int eps_get_sys_curr()
{
    eps_hk_vi_t vi[1];
    if (eps_get_hk_2_vi(vi) < 0)
        return -1;
    return vi->cursys;
}

int eps_get_vbatt()
{
    eps_hk_vi_t vi[1];
    if (eps_get_hk_2_vi(vi) < 0)
        return -1;
    return vi->vbatt;
}

int eps_get_outpower()
{
    eps_hk_vi_t vi[1];
    if (eps_get_hk_2_vi(vi) < 0)
        return -1;
    return vi->vbatt * vi->cursys / 1000; // micro Watt to milli Watt
}

int eps_get_vsun()
{
    int vsun = 0;
    eps_hk_vi_t vi[1];
    if (eps_get_hk_2_vi(vi) < 0)
        return -1;
    for (int i = 0; i < 3; i++)
        vsun += vi->vboost[i];
    return vsun / 3;
}

int eps_get_vsun_all(uint16_t *vsun)
{
    if (vsun == NULL)
        return -2;
    eps_hk_vi_t vi[1];
    if (eps_get_hk_2_vi(vi) < 0)
        return -1;
    for (int i = 0; i < 3; i++)
        vsun[i] = vi->vboost[i];
    return 1;
}

int eps_get_isun()
{
    eps_hk_vi_t vi[1];
    if (eps_get_hk_2_vi(vi) < 0)
        return -1;
    return vi->cursun;
}

void convert_hk_to_sh(eps_hk_t *hk_t, sh_hk_t *hk)
{
    for (int i = 0; i < 3; i++)
        hk->vboost[i] = hk_t->vboost[i];
    hk->vbatt = hk_t->vbatt;
    hk->cursun = hk_t->cursun;
    hk->cursys = hk_t->cursys;
    hk->curout[0] = hk_t->curout[0]; // OBC
    hk->curout[1] = hk_t->curout[1]; // UDC
    hk->outputs = 0;
    for (int i = 0; i < 8; i++)
        hk->outputs |= (hk_t->output[i] > 0) << i;
    hk->knife_on_off_delta = (hk_t->output_off_delta[7] > 0);
    hk->knife_on_off_delta |= (hk_t->output_on_delta[7] > 0) << 4;
    for (int i = 0; i < 2; i++)
        hk->latchup[i] = hk_t->latchup[i];
    hk->wdt_i2c_time_min = hk_t->wdt_i2c_time_left / 60;
    hk->wdt_i2c_time_sec = hk_t->wdt_i2c_time_left % 60;
    hk->counter_wdt_i2c = hk_t->counter_wdt_i2c;
    hk->counter_boot = hk_t->counter_boot;
    for (int i = 0; i < 6; i++)
        if (abs(hk_t->temp[i] > 0xff))
            hk->temp[i] = hk_t->temp[i] / 100;
        else
            hk->temp[i] = hk_t->temp[i];
    hk->bootcause = hk_t->bootcause;
    hk->battnpptmode = hk_t->pptmode;
    hk->battnpptmode |= hk_t->battmode << 4;
}

int eps_get_min_hk(sh_hk_t *hk)
{
    if (hk == NULL)
        return -2;
    eps_hk_t hk_t[1];
    if (eps_get_hk(hk_t) < 0)
        return -1;
    convert_hk_to_sh(hk_t, hk);
    return 1;
}

int eps_get_loop_timer()
{
    return EPS_LOOP_TIMER;
}

int eps_set_loop_timer(int t)
{
    if (t < 1)
        t = 1; // minimum 1 s
    if (t > 3600)
        t = 600; // 10 minutes in case we want to loop an hour, that is ridiculuous
    EPS_LOOP_TIMER = t;
    return EPS_LOOP_TIMER;
}

eps_hk_t eps_system_hk;
sh_hk_t sh_system_hk;
uint16_t eps_vbatt;
uint16_t eps_mvboost;
uint16_t eps_cursun;
uint16_t eps_cursys;
uint8_t eps_battmode;

// Initializes the EPS and ping-tests it.
int eps_init()
{
    // Check if malloc was successful.
    if (eps == NULL)
    {
        return -1;
    }

    // Initializes the EPS component while checking if successful.
    if (eps_p31u_init(eps, 0, 0x1b) <= 0)
    {
        return -1;
    }
    shprintf("EPS initializing\n");
    // If we can't successfully ping the EPS then something has gone wrong.
    if (eps_p31u_ping(eps) < 0)
    {
        return -2;
    }
    if (sys_boot_count < 2) // first boot, another go at thermal knife to make life easier
    {
        if (eps_p31u_ks_set(eps, 1000 * 30) < 0)
        {
            shprintf("Could not run thermal knife at boot %d\n", sys_boot_count);
        }
    }
    // at this point, get config
    eps_config_t conf[1];
    if (eps_p31u_get_conf(eps, conf) < 0)
    {
        shprintf("Could not get config file\n");
        return -3;
    }
    conf->ppt_mode = 1;                    // MPPT tracking
    conf->vboost[0] = 16700;               // 16.7 V boost max (for FIXED)
    conf->vboost[1] = 16700;               // 16.7 V boost max (for FIXED)
    conf->vboost[2] = 16700;               // 16.7 V boost max (for FIXED)
    conf->battheater_mode = 1;             // auto mode for battery heater
    conf->battheater_high = 40;            // turn off at 40 C
    conf->battheater_low = 5;              // turn on at 5 C
    conf->output_initial_off_delay[0] = 0; // at this point, OBC should not have off delay
    conf->output_initial_on_delay[0] = 15; // at this point, OBC has short on delay
    conf->output_initial_off_delay[1] = 0; // make sure PLL 5V does not have on/off delay
    conf->output_initial_on_delay[1] = 0;  // make sure PLL 5V does not have on/off delay
    conf->output_initial_off_delay[3] = 0; // at this point, UHF should not have off delay
    conf->output_initial_on_delay[3] = 10; // at this point, UHF has short on delay
    conf->output_initial_off_delay[7] = 0; // at this point, thermal knife should not turn on
    conf->output_initial_on_delay[7] = 0;  // at this point, thermal knife should not turn on
    conf->output_normal_value[0] = 0;      // at this point, OBC is on in normal startup
    conf->output_normal_value[1] = 0;      // make sure PLL 5V does not turn on
    conf->output_normal_value[3] = 0;      // at this point, UHF is on in safe startup
    conf->output_normal_value[7] = 0;      // ensure thermal knife is not on
    conf->output_safe_value[0] = 1;        // OBC on in safe
    conf->output_safe_value[1] = 0;        // make sure PLL 5V does not turn on
    conf->output_safe_value[3] = 1;        // UHF on in safe
    conf->output_safe_value[7] = 0;        // Thermal knife OFF in safe
    if (eps_p31u_set_conf(eps, conf) < 0)
    {
        shprintf("Could not set config file\n");
        return -4;
    }
    return 1;
}

void *eps_thread(void *tid)
{
#ifdef DLGR_EPS
    DLGR_REGISTER(eps_system_hk, sizeof(eps_hk_t));
    DLGR_REGISTER(sh_system_hk, sizeof(sh_hk_t));
#endif
    while (!done)
    {
        // Reset the watch-dog timer.
        eps_reset_wdt(eps);
        // get full housekeeping data
        if (eps_get_hk(&eps_system_hk) < 0)
        {
            eprintf("Error getting EPS housekeeping data");
        }
        convert_hk_to_sh(&eps_system_hk, &sh_system_hk);
        // Log housekeeping data.
#ifdef DLGR_EPS
        DLGR_WRITE(eps_system_hk);
        DLGR_WRITE(sh_system_hk);
#endif
        eps_vbatt = eps_system_hk.vbatt;
        for (int i = 0, eps_mvboost = 0; i < 3; i++)
            if (eps_mvboost < eps_system_hk.vboost[i])
                eps_mvboost = eps_system_hk.vboost[i];
        eps_cursun = eps_system_hk.cursun;
        eps_cursys = eps_system_hk.cursys;
        eps_battmode = eps_system_hk.battmode;
#ifdef DATAVIS
        pthread_mutex_lock(&(datavis_mutex));
        g_datavis_st.data.eps_vbatt = eps_vbatt;
        g_datavis_st.data.eps_mvboost = eps_mvboost;
        g_datavis_st.data.eps_cursun = eps_cursun;
        g_datavis_st.data.eps_cursys = eps_cursys;
        g_datavis_st.data.eps_battmode = eps_battmode;
        pthread_mutex_unlock(&(datavis_mutex));
#endif
        sleep(EPS_LOOP_TIMER);
    }
    pthread_exit(NULL);
}

// Frees eps memory and destroys the EPS object.
void eps_destroy()
{
    // Destroy / free the eps.
    eps_p31u_destroy(eps);
}
