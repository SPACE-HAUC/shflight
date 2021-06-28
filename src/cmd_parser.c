/**
 * @file parser.c
 * @author Mit Bailey (mitbailey99@gmail.com)
 * @brief 
 * @version 0.1
 * @date 2021.06.18
 * 
 * @copyright Copyright (c) 2021
 * 
 */

#include "acs_extern.h"
#include "eps_extern.h"
#include "xband_extern.h"
#include "datalogger_extern.h"
#include "uhf_modem/uhf_modem.h"
#define MAIN_PRIVATE
#include "main.h"
#undef MAIN_PRIVATE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include "sw_update_sh.h"

#define THIS_MODULE "comm"

enum MODULE_ID
{
    INVALID_ID = 0x0,
    ACS_ID = 0x1,
    EPS_ID = 0x2,
    XBAND_ID = 0x3,
    SW_UPD_ID = 0xf,
    ACS_UPD_ID = 0xe,
    SYS_VER_MAGIC = 0xd,
    SYS_RESTART_PROG = 0xff,
    SYS_REBOOT = 0xfe,
    SYS_CLEAN_SHBYTES = 0xfd
};

#define SYS_RESTART_FUNC_MAGIC 0x3c

uint64_t SYS_RESTART_FUNC_VAL = 0x2fa45d2002d54af2;

#define SYS_REBOOT_FUNC_MAGIC 0x9d

uint64_t SYS_REBOOT_FUNC_VAL = 0x36a45d2002d54af0;

enum SW_UPD_FUNC_ID
{
    SW_UPD_FUNC_MAGIC = 0x87,
};

#define SW_UPD_VALID_MAGIC 0x2489f750228d2e4fL

enum ACS_FUNC_ID
{
    ACS_INVALID_ID = 0x0,
    ACS_GET_MOI_ID,
    ACS_SET_MOI_ID,
    ACS_GET_IMOI_ID,
    ACS_SET_IMOI_ID,
    ACS_GET_DIPOLE,
    ACS_SET_DIPOLE,
    ACS_GET_TSTEP,
    ACS_SET_TSTEP,
    ACS_GET_MEASURE_TIME,
    ACS_SET_MEASURE_TIME,
    ACS_GET_LEEWAY,
    ACS_SET_LEEWAY,
    ACS_GET_WTARGET,
    ACS_SET_WTARGET,
    ACS_GET_DETUMBLE_ANG,
    ACS_SET_DETUMBLE_ANG,
    ACS_GET_SUN_ANGLE,
    ACS_SET_SUN_ANGLE
};

enum EPS_FUNC_ID
{
    EPS_INVALID_ID = 0x0,
    EPS_GET_MIN_HK = 0x1,
    EPS_GET_VBATT,
    EPS_GET_SYS_CURR,
    EPS_GET_OUTPOWER,
    EPS_GET_VSUN,
    EPS_GET_VSUN_ALL,
    EPS_GET_ISUN,
    EPS_GET_LOOP_TIMER,
    EPS_SET_LOOP_TIMER,
};

enum XBAND_FUNC_ID
{
    XBAND_INVALID_ID = 0x0,
    XBAND_SET_TX,
    XBAND_SET_RX,
    XBAND_DO_TX,
    XBAND_DO_RX,
    XBAND_DISABLE,
    XBAND_SET_MAX_ON,
    XBAND_GET_MAX_ON,
    XBAND_SET_TMP_SHDN,
    XBAND_GET_TMP_SHDN,
    XBAND_SET_TMP_OP,
    XBAND_GET_TMP_OP,
    XBAND_GET_LOOP_TIME,
    XBAND_SET_LOOP_TIME,
};

typedef struct __attribute__((packed))
{
    float LO;
    float bw;
    uint16_t samp;
    uint8_t phy_gain;
    uint8_t adar_gain;
    uint8_t ftr;
    short phase[16];
} xband_set_data;

#define XBAND_MAX_FID 10

typedef struct __attribute__((packed))
{
    uint8_t type; // 0 -> text, 1 -> image
    int f_id;     // -1 -> random, 0 -> max
    int mtu;      // 0 -> 128
} xband_tx_data;

typedef struct __attribute__((packed))
{
    uint8_t mod;
    uint8_t cmd;
    int unused;
    int data_size;
    unsigned char data[46];
} cmd_input_t;

typedef struct __attribute__((packed))
{
    uint8_t mod;            // 1
    uint8_t cmd;            // 1
    int retval;             // 4
    int data_size;          // 4
    unsigned char data[46]; // 46
} cmd_output_t;

char *XBAND_FTR_NAMES[] =
    {
        "m_6144.ftr",
        "m_3072.ftr",
        "m_1000.ftr",
        "m_lte5.ftr",
        "m_lte1.ftr"};

int prsr_wait_on_command(uhf_modem_t uhf_dev, cmd_input_t *in)
{
    return uhf_read(uhf_dev, (char *)in, sizeof(cmd_input_t));
}

int prsr_write_reply(uhf_modem_t uhf_dev, cmd_output_t *out)
{
    return uhf_write(uhf_dev, (char *)out, sizeof(cmd_output_t));
}

ssize_t replace_file(const char *dest, const char *src)
{
    if (access(src, F_OK))
    {
        shprintf("Could not access file %s\n", src);
        return -1;
    }
    int df, sf;
    df = open(dest, O_CREAT | O_WRONLY | O_TRUNC, 0755); // open destination
    if (df < 0)
    {
        shprintf("Could not open destination FID\n");
        return df;
    }
    sf = open(src, O_RDONLY); // open source
    if (sf < 0)
    {
        shprintf("Could not open source FID");
        return sf;
    }
    char buf[8192];
    ssize_t rd_bytes = 0, wr_bytes = 0;
    ssize_t rd_buf = 0;
    while ((rd_buf = read(sf, buf, sizeof(buf))) > 0)
    {
        rd_bytes += rd_buf; // update read size
        ssize_t wr_buf = 0;
        do
        {
            wr_buf += write(df, buf + wr_buf, rd_buf - wr_buf); // write to file and update write size
        } while (wr_buf < rd_buf);                              // continue while
        wr_bytes += wr_buf;
    }
    shprintf("Read %d bytes, wrote %d bytes\n", rd_bytes, wr_bytes);
    sync();
    return (rd_bytes - wr_bytes); // 0 on success, non-zero on error
}

int replace_cp(const char *dest, const char *src)
{
    char cmd[512];
    snprintf(cmd, 512, "\\cp %s %s", src, dest);
    return system(cmd);
}

void prsr_parse_command(uhf_modem_t uhf_fd, cmd_input_t *input, cmd_output_t *output)
{
    memset(output->data, 0x0, 46);
    int module_id = input->mod;
    int function_id = input->cmd;
    output->mod = input->mod;
    output->cmd = input->cmd;
    output->data_size = 0;
    output->retval = -50;
    shprintf("Module: %x, Command: %x\n", module_id, function_id);
    if (module_id == SYS_CLEAN_SHBYTES)
    {
        if (function_id == SYS_CLEAN_SHBYTES)
        {
            system("/bin/rm -f *.shbytes");
            output->retval = 1;
        }
    }
    else if (module_id == SW_UPD_ID)
    {
        if (function_id == SW_UPD_FUNC_MAGIC)
        {
            uint64_t sw_upd_magic = *((uint64_t *)input->data);
            if (sw_upd_magic == SW_UPD_VALID_MAGIC)
            {
                bool rcv = false;
                output->retval = sw_sh_receive_file(uhf_fd, &rcv);
            }
            shprintf("sw_sh_receive retval: %d\n", output->retval);
            if (output->retval == 1) // update binary
            {
                // check if it is our binary
                if (!access("flight_upd_bin", F_OK))
                {
                    shprintf("file exists!\n");
                    while (!access("/var/tmp/fsan.running", F_OK))
                    {
                        shprintf("fsan running!\n");
                        sleep(1);
                    }
                    // lock fsan.running
                    int fd = open("/var/tmp/fsan.running", O_CREAT | O_RDWR, 0666);
                    while (write(fd, "1", 1) < 1)
                        ;
                    close(fd);
                    sync(); // sync FS
                    // Update binary
                    char bin_name[256];
                    int tot_updated = 0;
                    for (int i = 1; i < 10; i++)
                    {
                        int ctr = 10, upd_ret = 0;
                        snprintf(bin_name, sizeof(bin_name), "%s%d", sys_bin_name, i);
                        shprintf("Replacing %s\n", bin_name);
                        while ((upd_ret = replace_file(bin_name, "flight_upd_bin")) && (--ctr))
                        {
                            shprintf("Error updating binary %s\n", bin_name);
                        }
                        if (!upd_ret)
                            tot_updated++;
                    }
                    if (tot_updated > 5)
                    {
                        if (replace_cp(sys_bin_name, "flight_upd_bin"))
                        {
                            shprintf("Error updating main binary %s\n", sys_bin_name);
                        }
                    }
                    // unlock fsan.running
                    unlink("/var/tmp/fsan.running");
                    // unlink the update file
                    unlink("flight_upd_bin");
                    sync();     // write changes to disk
                    sys_exit(); // exit the current program to load into the new binary
                }
            }
        }
        else
        {
            output->retval = -50;
        }
    }
    else if (module_id == SYS_VER_MAGIC)
    {
        output->retval = vermagic;
    }
    else if (module_id == SYS_RESTART_PROG)
    {
        if (function_id == SYS_RESTART_FUNC_MAGIC)
        {
            uint64_t val = *((uint64_t *)(input->data));
            if (val == SYS_RESTART_FUNC_VAL)
            {
                catch_sigint(SIGCONT);
            }
        }
    }
    else if (module_id == SYS_REBOOT)
    {
        if (function_id == SYS_REBOOT_FUNC_MAGIC)
        {
            uint64_t val = *((uint64_t *)(input->data));
            if (val == SYS_REBOOT_FUNC_VAL)
            {
                execl("/bin/shutdown", "-r", "now", (char *)0);
            }
        }
    }
    else if (module_id == ACS_UPD_ID)
    {
        acs_upd.vbatt = eps_vbatt;
        acs_upd.vboost = eps_mvboost;
        acs_upd.cursun = eps_cursun;
        acs_upd.cursys = eps_cursys;
        output->retval = 1;
        output->data_size = sizeof(acs_uhf_packet);
        memcpy(output->data, &acs_upd, output->data_size);
    }
    else if (module_id == ACS_ID)
    {
        switch (function_id)
        {
        // TODO: Collect these into one
        case ACS_GET_MOI_ID:
        {
            output->data_size = 9 * sizeof(float);
            memset(output->data, 0x0, sizeof(float));

            output->retval = acs_get_moi((float *)output->data);
            break; // ACS_GET_MOI_ID
        }

        case ACS_SET_MOI_ID:
        {
            if (input->data_size != 9 * sizeof(float))
            {
                output->retval = -50;
                break;
            }
            float *inval = (float *)input->data;
            output->retval = acs_set_moi(inval);
            break; // ACS_SET_MOI_ID
        }

        case ACS_GET_IMOI_ID:
        {
            output->data_size = 9 * sizeof(float);
            memset(output->data, 0x0, sizeof(float));

            output->retval = acs_get_imoi((float *)output->data);
            break; // ACS_GET_IMOI_ID}
        }

        case ACS_SET_IMOI_ID:
        {
            if (input->data_size != 9 * sizeof(float))
            {
                output->retval = -50;
                break;
            }
            float *inval = (float *)input->data;
            output->retval = acs_set_imoi(inval);
            break; // ACS_SET_IMOI_ID
        }
            // end TODO

        case ACS_GET_DIPOLE:
        {
            float retval = acs_get_dipole();
            memcpy(&(output->retval), &retval, sizeof(float));
            break; // ACS_GET_DIPOLE
        }

        case ACS_SET_DIPOLE:
        {
            if (input->data_size != sizeof(float))
            {
                output->retval = -50;
                break;
            }
            float *inval = (float *)input->data;
            float retval = acs_set_dipole(*inval);
            memcpy(&(output->retval), &retval, sizeof(float));
            break; // ACS_SET_DIPOLE
        }

        case ACS_GET_TSTEP:
        {
            uint32_t retval = acs_get_tstep();
            memcpy(&(output->retval), &retval, sizeof(uint32_t));
            break; // ACS_GET_TSTEP
        }

        case ACS_SET_TSTEP:
        {
            if (input->data_size != sizeof(uint8_t))
            {
                output->retval = -50;
                break;
            }

            uint8_t inval = input->data[0];
            *(&output->retval) = acs_set_tstep(inval);
            break; // ACS_SET_TSTEP
        }

        case ACS_GET_MEASURE_TIME:
        {
            *(&output->retval) = acs_get_measure_time();
            break; // ACS_GET_MEASURE_TIME
        }

        case ACS_SET_MEASURE_TIME:
        {
            if (input->data_size != sizeof(uint8_t))
            {
                output->retval = -50;
                break;
            }
            uint8_t *inval = input->data;
            *(&output->retval) = acs_set_measure_time(*inval);
            break; // ACS_SET_MEASURE_TIME
        }

        case ACS_GET_LEEWAY:
        {
            *((uint8_t *)&(output->retval)) = acs_get_leeway();
            break; // ACS_GET_LEEWAY
        }

        case ACS_SET_LEEWAY:
        {
            if (input->data_size != sizeof(uint8_t))
            {
                output->retval = -50;
                break;
            }
            uint8_t *inval = input->data;
            *((uint8_t *)&(output->retval)) = acs_set_leeway(*inval);
            break; // ACS_SET_LEEWAY
        }

        case ACS_GET_WTARGET:
        {
            *((float *)&output->retval) = acs_get_wtarget();
            break; // ACS_GET_WTARGET
        }

        case ACS_SET_WTARGET:
        {
            if (input->data_size != sizeof(float))
            {
                output->retval = -50;
                break;
            }
            float *inval = (float *)input->data;
            *((float *)&output->retval) = acs_set_wtarget(*inval);
            break; // ACS_SET_WTARGET
        }

        case ACS_GET_DETUMBLE_ANG:
        {
            *((uint8_t *)&output->retval) = acs_get_detumble_ang();
            break; // ACS_GET_DETUMBLE_ANG
        }

        case ACS_SET_DETUMBLE_ANG:
        {
            if (input->data_size != sizeof(uint8_t))
            {
                output->retval = -50;
                break;
            }
            uint8_t inval = input->data[0];
            *((uint8_t *)&output->retval) = acs_set_detumble_ang(inval);
            break; // ACS_SET_DETUMBLE_ANG
        }

        case ACS_GET_SUN_ANGLE:
        {
            *((uint8_t *)&output->retval) = acs_get_sun_angle();
            break; // ACS_GET_SUN_ANGLE
        }

        case ACS_SET_SUN_ANGLE:
        {
            if (input->data_size != sizeof(uint8_t))
            {
                output->retval = -50;
                break;
            }
            uint8_t inval = input->data[0];
            *((uint8_t *)&output->retval) = acs_set_sun_angle(inval);
            break; // ACS_SET_SUN_ANGLE
        }

        default:
        case ACS_INVALID_ID:
        {
            output->data_size = 0;

            output->retval = -1;
            break; // DEFAULT, ACS_INVALID_ID
        }
        }
    }
    else if (module_id == EPS_ID)
    {
        switch (function_id)
        {
        case EPS_GET_MIN_HK:
        {
            sh_hk_t sh_hk[1];
            output->retval = eps_get_min_hk(sh_hk);
            memset(output->data, 0x0, sizeof(sh_hk));
            memcpy(output->data, sh_hk, sizeof(sh_hk));
            output->data_size = sizeof(sh_hk);
            break; // EPS_GET_MIN_HK
        }

        case EPS_GET_VBATT:
        {
            output->retval = eps_get_vbatt();
            output->data_size = 0;
            break; // EPS_GET_VBATT
        }

        case EPS_GET_SYS_CURR:
        {
            output->retval = eps_get_sys_curr();
            output->data_size = 0;
            break; // EPS_GET_SYS_CURR
        }

        case EPS_GET_OUTPOWER:
        {
            output->retval = eps_get_outpower();
            output->data_size = 0;
            break;
        }

        case EPS_GET_VSUN:
        {
            output->retval = eps_get_vsun();
            output->data_size = 0;
            break;
        }

        case EPS_GET_VSUN_ALL:
        {
            output->retval = eps_get_vsun(output->data);
            output->data_size = 3 * sizeof(uint16_t);
            break;
        }

        case EPS_GET_ISUN:
        {
            output->retval = eps_get_isun();
            output->data_size = 0;
            break;
        }

        case EPS_GET_LOOP_TIMER:
        {
            output->retval = eps_get_loop_timer();
            output->data_size = 0;
            break;
        }

        case EPS_SET_LOOP_TIMER:
        {
            output->retval = eps_set_loop_timer(*(int *)(input->data));
            output->data_size = 0;
            break;
        }

        default:
        case EPS_INVALID_ID:
        {
            output->retval = -1;
            output->data_size = 0;
            break; // EPS_INVALID_ID
        }
        }
    }
    else if (module_id == XBAND_ID)
    {
        switch (function_id)
        {
        case XBAND_SET_TX:
        {
            if (input->data_size != sizeof(xband_set_data))
            {
                output->retval = -50;
                output->data_size = 0;
                break;
            }
            xband_set_data *ds = (xband_set_data *)input->data;
            float LO = ds->LO;
            float bw = ds->bw;
            uint16_t samp = ds->samp;
            uint8_t phy_gain = ds->phy_gain;
            uint8_t adar_gain = ds->adar_gain;
            float phase[16];
            for (int i = 0; i < 16; i++)
            {
                phase[i] = ds->phase[i] * 0.01;
            }
            char *ftr_name = NULL;
            if ((ds->ftr >= 0) && (ds->ftr < (sizeof(XBAND_FTR_NAMES) / sizeof(XBAND_FTR_NAMES[0]))))
            {
                ftr_name = XBAND_FTR_NAMES[ds->ftr];
            }
            output->retval = xband_set_tx(LO, bw, samp, phy_gain, ftr_name, phase, adar_gain);
            output->data_size = 0;
            break;
        }
        case XBAND_SET_RX:
        {
            if (input->data_size != sizeof(xband_set_data))
            {
                output->retval = -50;
                output->data_size = 0;
                break;
            }
            xband_set_data *ds = (xband_set_data *)input->data;
            float LO = ds->LO;
            float bw = ds->bw;
            uint16_t samp = ds->samp;
            uint8_t phy_gain = ds->phy_gain;
            uint8_t adar_gain = ds->adar_gain;
            float phase[16];
            for (int i = 0; i < 16; i++)
            {
                phase[i] = ds->phase[i] * 0.01;
            }
            char *ftr_name = NULL;
            if ((ds->ftr >= 0) && (ds->ftr < sizeof(XBAND_FTR_NAMES) / sizeof(XBAND_FTR_NAMES[0])))
            {
                ftr_name = XBAND_FTR_NAMES[ds->ftr];
            }
            output->retval = xband_set_rx(LO, bw, samp, phy_gain, ftr_name, phase, adar_gain);
            output->data_size = 0;
            break;
        }
        case XBAND_DO_TX:
        {
            if (input->data_size != sizeof(xband_tx_data))
            {
                output->retval = -50;
                break;
            }
            xband_tx_data *ds = (xband_tx_data *)input->data;
            if (ds->type > 1)
                ds->type = 0; // default: text
            char fname[128];
            snprintf(fname, sizeof(fname), "xband_tx_data/%d.%s", ds->f_id, ds->type > 0 ? ".jpg" : ".txt");
            int fd = open(fname, O_RDONLY);
            if (fd < 3)
            {
                shprintf("File %s does not exist\n", fname);
                output->retval = -51;
                break;
            }
            ssize_t fsize = lseek(fd, 0, SEEK_END);
            lseek(fd, 0, SEEK_SET);
            uint8_t *data = malloc(fsize);
            fsize = read(fd, data, fsize);
            close(fd);
            output->retval = xband_do_tx(data, fsize, ds->mtu);
            free(data);
            break;
        }
        case XBAND_SET_MAX_ON:
        {
            output->retval = xband_set_max_on(*(int8_t *)(input->data));
            break;
        }
        case XBAND_GET_MAX_ON:
        {
            output->retval = xband_get_max_on();
            break;
        }
        case XBAND_SET_TMP_SHDN:
        {
            output->retval = xband_set_tmp_shutdown(*(int8_t *)(input->data));
            break;
        }
        case XBAND_GET_TMP_SHDN:
        {
            output->retval = xband_get_tmp_shutdown();
            break;
        }
        case XBAND_SET_TMP_OP:
        {
            output->retval = xband_set_tmp_op(*(int8_t *)(input->data));
            break;
        }
        case XBAND_GET_TMP_OP:
        {
            output->retval = xband_get_tmp_op();
            break;
        }
        case XBAND_SET_LOOP_TIME:
        {
            output->retval = xband_set_loop_time(*(int8_t *)(input->data));
            break;
        }
        case XBAND_GET_LOOP_TIME:
        {
            output->retval = xband_get_loop_time();
            break;
        }
        default:
        case XBAND_INVALID_ID:
        {
            break; // XBAND_INVALID_ID
        }
        }
    }
    return;
}

static uhf_modem_t uhf_dev = -1;

int prsr_init(void)
{
    uhf_dev = uhf_init("/dev/ttyPS1");
    if (uhf_dev < 3)
    {
        shprintf("Failed to initialize UHF, return %d\n", uhf_dev);
    }
    return uhf_dev;
}

void *prsr_thread(void *tid)
{
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);

    printf("[parser] Main binary name: %s\n", sys_bin_name);

    int retval = 0;

    cmd_input_t input[1];
    cmd_output_t output[1];

    while (!done)
    {
        if (prsr_wait_on_command(uhf_dev, input) > 0) // blocking
        {
            prsr_parse_command(uhf_dev, input, output);
        }
        if (prsr_write_reply(uhf_dev, output) <= 0)
        {
            shprintf("Error writing reply\n");
        }
    }
    pthread_exit(NULL);
}

void prsr_destroy(void)
{
    if (uhf_dev > 2)
        uhf_destroy(uhf_dev);
}