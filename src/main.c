#include <main.h>
#include <main_helper.h>

const float MOI[3][3] = {{1, 0, 0}, {0, 1, 0}, {0, 0, 1}};

int8_t mag_index = -1, bdot_index = -1, omega_index = -1, sol_index = -1, L_err_index = -1;
uint8_t omega_ready = 0;

DECLARE_BUFFER(g_B, float);

DECLARE_BUFFER(g_W, float);

DECLARE_BUFFER(g_Bt, float);

DECLARE_BUFFER(g_S, float);

volatile uint8_t g_nightmode = 0;

// target angular speed
DECLARE_VECTOR(g_W_target, float);

// target angular momentum
DECLARE_VECTOR(g_L_target, float);

float g_L_pointing[SH_BUFFER_SIZE];
float g_L_mag[SH_BUFFER_SIZE];

p31u *g_eps;

ncv7708 *hbridge;

/* 
 * Variable naming convention: separated by underscore, clear naming. Globals
 * require a `g_' in front.
 * 
 * Function naming convention: capitalize letters of words after the first one.
 */

// Main function of the system
int main(int argc, char *argv[])
{
    g_boot_count = bootCount();    // determines system boot count
    g_bootmoment = get_usec();     // store moment of boot in usec
    g_program_state = SH_SYS_INIT; // On first boot, similar to detumble. On next boots, check all sensors and select appropriate state
    fprintf(stderr, "State: %d\n", g_program_state);
    g_bootup = 1; // the program is booting up
    // write zeros into all buffers and reset heads
    FLUSH_BUFFER_ALL;
    // calculate target angular momentum
    MATVECMUL(g_L_target, MOI, g_W_target);
    // initialize h-bridge
    hbridge = (ncv7708 *)malloc(sizeof(ncv7708));
    snprintf(hbridge->fname, 40, HBRIDGE_DEV_NAME);
    int hbridge_init_status = hbridge_init(hbridge);
    // implement thread spawning
    int rc[5];
    //pthread_t threads[5] ;
    //func_list threadList[5] = {data_ack, acs, xband, uhf, eps_telem} ;
    ncv7708_destroy(hbridge);
    return 0;
}