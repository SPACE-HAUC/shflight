/**
 * @file xband.c
 * @author Sunip K. Mukherjee (sunipkmukherjee@gmail.com)
 * @brief X-Band Radio interface code
 * @version 0.1
 * @date 2020-03-19
 * 
 * @copyright Copyright (c) 2020
 * 
 */

/******* PIN ASSIGNMENTS *********/
#define ADAR_SPI_BUS 1
#define ADAR_SPI_CS 1      // Pin 24 for ADAR Chip Select
#define ADAR_CS_INTERNAL 1 // CS is internal for test program

#define ADF_SPI_BUS 1
#define ADF_SPI_CS 2 // Pin 26 NO CONNECT
#define ADF_CS_INTERNAL 0
#define ADF_CS_GPIO 7 // Pin 27 for ADF Chip Select

#define PWR_TGL 4
#define TGL_5V 5
#define PLL_LOCK 16
#define TR_BF 1
#define TR_UDC 6
#define TX_LD 2
#define RX_LD 3
// #define PA_ON 37

#define TMP_0 8
#define TMP_1 9
#define TMP_2 10
#define TMP_3 11
#define TMP_4 12
#define TMP_5 13
#define TMP_6 14

#define PWR_MAIN EPS_LUP_3V2
#define PWR_UDC EPS_LUP_5V2
/********************************/

#include "adar1000/adar1000.h"     // ADAR1000 front end
#include "adf4355/adf4355.h"       // ADF4355 front end
#include "tmp123/tmp123.h"         // XBAND temp sensors
#include "gpiodev/gpiodev.h"       // GPIO control for TR switches
#include "modem/include/txmodem.h" // TX modem
#include "modem/include/rxmodem.h" // RX modem
#include "modem/include/libiio.h"  // PHY controls
#include <eps_extern.h>            // EPS controls

#include <stdint.h>
#include <pthread.h>
#include <stdio.h>
#include <stdbool.h>
#include <signal.h>
#include <macros.h>
#include <string.h>
#include <math.h>
#include <main.h>
#define XBAND_INTERNAL
#define _XBAND_EXTERN_H
#include <xband.h>
#undef XBAND_INTERNAL

#define THIS_MODULE "xband"

static adar1000 adar[1];
static adf4355 adf[1];

static rxmodem rx[1];
static txmodem tx[1];
static adradio_t phy[1];
static tmp123 tsensor[7];

volatile bool xb_sys_rdy = false; // this variable indicates whether the XB system is activated
float TR_MAX_TEMP = 70;           // 70 C by default
float TR_MIN_TEMP = 60;           // 60 C to re-enable after thermal shutdown
int adar_mode = -1;               // uninitialized
int adar_last_mode = -1;          // uninitialized
bool tr_shutdown = false;         // do not assume it is shut down
int16_t MAX_TURNOFF_CTR = 600;   // 600 seconds
uint8_t XBAND_LOOP_TIME = 5;      // loop every 5 seconds

int xband_init(void)
{
    int status;
    status = eps_ping();
    if (status < 0)
    {
        eprintf("Failed EPS ping");
        return -9;
    }
    gpioSetMode(PWR_TGL, GPIO_OUT);
    gpioSetMode(TGL_5V, GPIO_OUT);
    gpioSetMode(PLL_LOCK, GPIO_IN);
    gpioSetMode(TR_BF, GPIO_OUT);
    gpioSetMode(TR_UDC, GPIO_OUT);
    gpioSetMode(TX_LD, GPIO_OUT);
    gpioSetMode(RX_LD, GPIO_OUT);

    adf->spi_bus = ADF_SPI_BUS;
    adf->spi_cs = ADF_SPI_CS;
    adf->spi_cs_internal = CS_EXTERNAL;
    adf->cs_gpio = ADF_CS_GPIO;
    adf->muxval = 6; // PLL LOCK
    adf->single = true;

    adar->initd = false;

    adar->num_devs = 4; // specify that there are 4 chips

    status = adf4355_init(adf);
    if (status < 0)
    {
        fprintf(stderr, "Error initializing ADF4355\n");
        return -1;
    }
    status = tmp123_init(&(tsensor[0]), ADF_SPI_BUS, ADF_SPI_CS, TMP_0);
    if (status < 0)
    {
        fprintf(stderr, "Error initializing TMP123\n");
        return -2;
    }
    status = tmp123_init(&(tsensor[1]), ADF_SPI_BUS, ADF_SPI_CS, TMP_1);
    if (status < 0)
    {
        fprintf(stderr, "Error initializing TMP123\n");
        return -3;
    }
    status = tmp123_init(&(tsensor[2]), ADF_SPI_BUS, ADF_SPI_CS, TMP_2);
    if (status < 0)
    {
        fprintf(stderr, "Error initializing TMP123\n");
        return -4;
    }
    status = tmp123_init(&(tsensor[3]), ADF_SPI_BUS, ADF_SPI_CS, TMP_3);
    if (status < 0)
    {
        fprintf(stderr, "Error initializing TMP123\n");
        return -5;
    }
    status = tmp123_init(&tsensor[4], ADF_SPI_BUS, ADF_SPI_CS, TMP_4);
    if (status < 0)
    {
        fprintf(stderr, "Error initializing TMP123\n");
        return -6;
    }
    status = tmp123_init(&tsensor[5], ADF_SPI_BUS, ADF_SPI_CS, TMP_5);
    if (status < 0)
    {
        fprintf(stderr, "Error initializing TMP123\n");
        return -7;
    }
    status = tmp123_init(&tsensor[6], ADF_SPI_BUS, ADF_SPI_CS, TMP_6);
    if (status < 0)
    {
        fprintf(stderr, "Error initializing TMP123\n");
        return -8;
    }
    status = txmodem_init(tx, uio_get_id("tx_ipcore"), uio_get_id("tx_dma"));
    if (status < 0)
    {
        eprintf("Error initializing TX modem, fatal error");
        // reboot?
        eps_hardreset();
    }
    status = rxmodem_init(rx, uio_get_id("rx_ipcore"), uio_get_id("rx_dma"));
    if (status < 0)
    {
        eprintf("Error initializing RX modem, fatal error");
        // reboot?
        eps_hardreset();
    }
    // do not initialize ADAR1000 until TX/RX mode has been chosen

    xb_sys_rdy = false;

    return 0;
}

enum ensm_mode adradio_read_ensm_mode(adradio_t *dev)
{
    char buf[10];
    if (adradio_get_ensm_mode(dev, buf, sizeof(buf) < 0))
        return -1;
    if (strncmp(buf, "SLEEP", sizeof("SLEEP") - 1) == 0)
        return SLEEP;
    else if (strncmp(buf, "FDD", sizeof("FDD") - 1) == 0)
        return FDD;
    else if (strncmp(buf, "TDD", sizeof("TDD") - 1) == 0)
        return TDD;
    return -1;
}

int xband_set_tx(float LO, float bw, uint16_t samp, uint8_t phy_gain, const char *ftr_name, float *phase, uint8_t adar_gain) // set up for TX
{
    if (eps_battmode < 3)
        return -10;
    // step 1: Turn on power
    int status = eps_lup_set(PWR_MAIN, 1);
    usleep(1000 * 100);
    if (status < 0)
    {
        eprintf("Could not turn on 3V3 LUP");
        return -1;
    }
    else
    {
        gpioWrite(PWR_TGL, GPIO_HIGH); // turn on main power, at this stage indicate system is ready for temperature sensor readouts
    }
    adar_mode = ADAR_MODE_TX;
    // turn on power to PLL
    if (eps_lup_set(PWR_UDC, 1) < 0)
    {
        eprintf("Could not turn on UDC LUP");
        return -7;
    }
    // initialize PLL
    gpioWrite(TR_UDC, GPIO_HIGH);
    adf4355_set_tx(adf);
    usleep(10000); // wait 10 ms for PLL to stabilize
    // check for lock
    bool pll_lock = true;
    for (int i = 10; i > 0; i--)
    {
        pll_lock &= (gpioRead(PLL_LOCK) == GPIO_HIGH);
        usleep(100);
    }
    if (pll_lock)
    {
        eprintf("PLL locked for TX");
    }
    else
    {
        eprintf("PLL lock failed");
        xband_disable();
        return -2;
    }
    adar1000_initialize(adar, adar_mode, ADAR_SPI_BUS, ADAR_SPI_CS, ADAR_CS_GPIO, TR_BF, TX_LD, RX_LD); // initialize ADAR1000
    adar->initd = true;
    uint8_t s_gain[16];
    for (int i = 0; i < 16; i++)
        s_gain[i] = adar_gain;
    adar1000_all_beam(adar, phase, s_gain); // load phases and gain
    // turn on phy
    for (int i = 10; (i > 0) && (adradio_read_ensm_mode(phy) != FDD); i--)
    {
        if (adradio_set_ensm_mode(phy, FDD) == EXIT_FAILURE)
        {
            xband_disable();
            return -3;
        }
    }
    // set TX LO
    long long freq = 0;
    int i = 0;
    for (i = 10; i > 0; i--)
    {
        freq = LO * 1e6; // MHz to Hz
        if (freq < 0)
        {
            eprintf("LO frequency negative, fatal error");
            xband_disable();
            return -4;
        }
        if (adradio_set_tx_lo(phy, freq) == EXIT_FAILURE)
        {
            eprintf("Error setting TX LO");
            xband_disable();
            return -5;
        }
        if (adradio_set_rx_lo(phy, freq + 10e6) == EXIT_FAILURE)
        {
            eprintf("Error setting RX LO"); // set RX frequency above TX frequency
        }
        long long rd_freq = 0;
        if (adradio_get_tx_lo(phy, &rd_freq) == EXIT_FAILURE)
        {
            eprintf("Error getting TX LO");
        }
        int new_freq = floor(rd_freq * 1e-6); // freq in MHz
        if (new_freq == floor(LO))            // compare the freqs in MHz
        {
            eprintf("Frequencies match!");
            break;
        }
    }
    // one last check
    if (i == 0)
    {
        if (adradio_get_tx_lo(phy, &freq) == EXIT_FAILURE)
        {
            eprintf("Error getting TX LO");
        }
        int new_freq = floor(freq * 1e-6); // freq in MHz
        if (new_freq != floor(LO))         // compare the freqs in MHz
        {
            eprintf("Frequencies do not match!");
            xband_disable();
            return -6;
        }
    }
    // set TX gain
    double gain = -phy_gain * 0.25; // quarter stepping
    if (gain < -5)
    {
        eprintf("PHY gain %lf set too low. Proceed with caution.", gain);
    }
    adradio_set_tx_hardwaregain(phy, gain);
    // load filter
    if (ftr_name != NULL)
    {
        if (adradio_load_fir(phy, ftr_name) == EXIT_FAILURE)
        {
            eprintf("Could not load filter %s", ftr_name);
        }
        else
        {
            if (adradio_enable_fir(phy, true) == EXIT_FAILURE)
            {
                eprintf("Could not enable FIR filter");
            }
        }
    }
    else
    {
        adradio_set_tx_bw(phy, bw * 1e6);
        if (samp > 2e3)
            adradio_set_samp(phy, samp * 1e3);
        else
            adradio_set_samp(phy, 5e6);
    }
    xb_sys_rdy = true;
    return 1;
}

int xband_set_rx(float LO, float bw, uint16_t samp, uint8_t phy_gain, const char *ftr_name, float *phase, uint8_t adar_gain) // set up for TX
{
    if (eps_battmode < 3)
        return -10;
    // step 1: Turn on power
    int status = eps_lup_set(PWR_MAIN, 1);
    usleep(1000 * 100);
    if (status < 0)
    {
        eprintf("Could not turn on 3V3 LUP");
        return -1;
    }
    else
    {
        gpioWrite(PWR_TGL, GPIO_HIGH); // turn on main power, at this stage indicate system is ready for temperature sensor readouts
    }
    adar_mode = ADAR_MODE_RX;
    // turn on power to PLL
    if (eps_lup_set(PWR_UDC, 1) < 0)
    {
        eprintf("Could not turn on UDC LUP");
        return -7;
    }
    // initialize PLL
    gpioWrite(TR_UDC, GPIO_HIGH);
    adf4355_set_rx(adf);
    usleep(10000); // wait 10 ms for PLL to stabilize
    // check for lock
    bool pll_lock = true;
    for (int i = 10; i > 0; i--)
    {
        pll_lock &= (gpioRead(PLL_LOCK) == GPIO_HIGH);
        usleep(100);
    }
    if (pll_lock)
    {
        eprintf("PLL locked for RX");
    }
    else
    {
        eprintf("PLL lock failed");
        xband_disable();
        return -2;
    }
    adar1000_initialize(adar, adar_mode, ADAR_SPI_BUS, ADAR_SPI_CS, ADAR_CS_GPIO, TR_BF, TX_LD, RX_LD); // initialize ADAR1000
    adar->initd = true;
    uint8_t s_gain[16];
    for (int i = 0; i < 16; i++)
        s_gain[i] = adar_gain;
    adar1000_all_beam(adar, phase, s_gain); // load phases and gain
    // turn on phy
    for (int i = 10; (i > 0) && (adradio_read_ensm_mode(phy) != FDD); i--)
    {
        if (adradio_set_ensm_mode(phy, FDD) == EXIT_FAILURE)
        {
            xband_disable();
            return -3;
        }
    }
    // set TX LO
    long long freq = 0;
    int i = 0;
    for (i = 10; i > 0; i--)
    {
        freq = LO * 1e6; // MHz to Hz
        if (freq < 0)
        {
            eprintf("LO frequency negative, fatal error");
            xband_disable();
            return -4;
        }
        if (adradio_set_rx_lo(phy, freq) == EXIT_FAILURE)
        {
            eprintf("Error setting RX LO");
            xband_disable();
            return -5;
        }
        if (adradio_set_tx_lo(phy, freq + 10e6) == EXIT_FAILURE)
        {
            eprintf("Error setting TX LO"); // set TX frequency above TX frequency
        }
        long long rd_freq = 0;
        if (adradio_get_rx_lo(phy, &rd_freq) == EXIT_FAILURE)
        {
            eprintf("Error getting TX LO");
        }
        int new_freq = floor(rd_freq * 1e-6); // freq in MHz
        if (new_freq == floor(LO))            // compare the freqs in MHz
        {
            eprintf("Frequencies match!");
            break;
        }
    }
    // one last check
    if (i == 0)
    {
        if (adradio_get_rx_lo(phy, &freq) == EXIT_FAILURE)
        {
            eprintf("Error getting TX LO");
        }
        int new_freq = floor(freq * 1e-6); // freq in MHz
        if (new_freq != floor(LO))         // compare the freqs in MHz
        {
            eprintf("Frequencies do not match!");
            xband_disable();
            return -6;
        }
    }
    // load filter
    if (ftr_name != NULL)
    {
        if (adradio_load_fir(phy, ftr_name) == EXIT_FAILURE)
        {
            eprintf("Could not load filter %s", ftr_name);
        }
        else
        {
            if (adradio_enable_fir(phy, true) == EXIT_FAILURE)
            {
                eprintf("Could not enable FIR filter");
            }
        }
    }
    else
    {
        adradio_set_rx_bw(phy, bw * 1e6);
        if (samp > 2e3)
            adradio_set_samp(phy, samp * 1e3);
        else
            adradio_set_samp(phy, 5e6);
    }
    xb_sys_rdy = true;
    return 1;
}

int xband_disable(void)
{
    if (xb_sys_rdy == false)
        return 0;
    xb_sys_rdy = false;
    // set phy to sleep
    for (int i = 10; (i > 0) && (adradio_read_ensm_mode(phy) != SLEEP); i--)
    {
        if (adradio_set_ensm_mode(phy, SLEEP) == EXIT_FAILURE)
        {
            return -3;
        }
    }
    // disable ADAR1000
    gpioWrite(TGL_5V, GPIO_LOW); // ensure it is low
    adar1000_destroy(adar);
    adar_mode = -1;
    adar->initd = false;
    // disable PLL
    adf4355_pw_down(adf);
    usleep(1000); // wait 1 ms
    adf4355_destroy(adf);
    while (eps_lup_set(PWR_UDC, 0) < 0)
    {
        eprintf("Could not turn off power to UDC, can not turn off main power");
        sleep(1);
    }
    // disable power
    gpioWrite(PWR_TGL, GPIO_LOW); // turn off main power
    eps_lup_set(PWR_MAIN, 0);
    return 0;
}

void *xband_thread(void *in)
{
    float tmpC[7];
    short ctr = 0;
    (void *)in;
    while (!done)
    {
        if (xb_sys_rdy)
        {
            if (ctr <= 0)
                ctr = MAX_TURNOFF_CTR;
            // read temperature sensors and log the data, enable thermal shutdown
            for (int i = 0; i < 7; i++)
                tmpC[i] = tmp123_read(&tsensor[i]) / 100.0;
            // establish the need for a shutdown
            if (!tr_shutdown)
            {
                bool shutdown_now = false;
                for (int i = 3; i < 7; i++)
                    shutdown_now |= tmpC[i] > TR_MAX_TEMP;
                if (shutdown_now)
                {
                    gpioWrite(TGL_5V, GPIO_LOW); // ensure this is off
                    if (adar->initd)
                    {
                        adar_last_mode = adar_mode;
                        adar_mode = ADAR_MODE_IDLE;
                        adar1000_set_mode(adar, adar_mode); // set ADAR1000 to IDLE mode
                    }
                    tr_shutdown = true; // indicate shutdown
                }
            }
            else if (xb_sys_rdy) // in case we have a shutdown AND system is still ready
            {
                bool shutdown_now = false;
                for (int i = 3; i < 7; i++)
                    shutdown_now |= tmpC[i] < TR_MIN_TEMP;
                if (shutdown_now)
                {
                    adar_mode = adar_last_mode;
                    adar1000_set_mode(adar, adar_mode);
                    tr_shutdown = false; // 5V will be turned on during TX only
                }
            }
        }
        ctr -= XBAND_LOOP_TIME;
        if (ctr < XBAND_LOOP_TIME)
            xband_disable();
        sleep(XBAND_LOOP_TIME); // do that every second
    }
    return NULL;
}

int xband_do_tx(uint8_t *data, ssize_t len, int mtu)
{
    if (eps_battmode < 3)
    {
        xband_disable();
        return -10;
    }
    // check if system in TX mode
    if (!xb_sys_rdy)
    {
        eprintf("XBand system not activated");
        return -1;
    }
    if (tr_shutdown)
    {
        eprintf("XBand system overheated");
        return -2;
    }
    if (adar_mode != ADAR_MODE_TX)
    {
        eprintf("XBand system not in TX mode, mode %d", adar_mode);
        return -(0x100 | adar_mode);
    }
    // turn on 5V
    gpioWrite(TGL_5V, GPIO_HIGH);
    usleep(500000); // half second wait for LO to settle
    if (mtu == 0)
        tx->mtu = 128;
    int retval = txmodem_write(tx, data, len);
    usleep(500000); // wait another half second
    gpioWrite(TGL_5V, GPIO_LOW);
    if (retval < 0)
        retval = -retval;
    else
        return 1;
    return -(0x200 | retval);
}

int xband_do_rx(uint8_t *data, ssize_t len)
{
    return -1; // not implemented
}

void xband_destroy(void)
{
    xband_disable();
    txmodem_destroy(tx);
    rxmodem_destroy(rx);
    for (int i = 0; i < 7; i++)
        tmp123_destroy(&tsensor[i]);
    adf4355_destroy(adf);
}

int8_t xband_set_max_on(int8_t t)
{
    if (t < 1)
        t = 1;
    if (t > 20)
        t = 20;
    MAX_TURNOFF_CTR = t * 60;
    return MAX_TURNOFF_CTR / 60;
}

int8_t xband_get_max_on(void)
{
    return MAX_TURNOFF_CTR / 60;
}

int8_t xband_set_tmp_shutdown(int8_t tmp)
{
    if (tmp < 50)
        tmp = 50;
    if (tmp > 95)
        tmp = 95;
    TR_MAX_TEMP = tmp;
    return (int8_t)TR_MAX_TEMP;
}

int8_t xband_get_tmp_shutdown(void)
{
    return (int8_t)TR_MAX_TEMP;
}

int8_t xband_set_tmp_op(int8_t tmp)
{
    if (tmp > TR_MAX_TEMP)
        tmp = TR_MAX_TEMP - 10;
    if (tmp > 95)
        tmp = 75;
    TR_MIN_TEMP = tmp;
    return (int8_t)TR_MIN_TEMP;
}

int8_t xband_get_tmp_op(void)
{
    return (int8_t)TR_MIN_TEMP;
}

uint8_t xband_get_loop_time(void)
{
    return XBAND_LOOP_TIME;
}

uint8_t xband_set_loop_time(uint8_t t)
{
    if (t > (MAX_TURNOFF_CTR / 5))
        t = MAX_TURNOFF_CTR / 5; // we require at least five measurements during the max time the system is on
    if (t < 1) // at least every second
        t = 1;
    XBAND_LOOP_TIME = t;
    return XBAND_LOOP_TIME;
}