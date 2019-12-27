#include <main.h>
#include <main_helper.h>

/* 
 * Variable naming convention: separated by underscore, clear naming. Globals
 * require a `g_' in front.
 * 
 * Function naming convention: capitalize letters of words after the first one.
 */

// Main function of the system
int main(int argc, char * argv[])
{
    g_boot_count = bootCount() ; // determines system boot count
    g_program_state = SH_SYS_INIT ; // On first boot, similar to detumble. On next boots, check all sensors and select appropriate state
    fprintf(stderr, "State: %d\n", g_program_state) ;
    g_bootup = 1 ; // the program is booting up
    // write zeros into all buffers
    FLUSH_BUFFER(g_B);
    FLUSH_BUFFER(g_Bt);
    FLUSH_BUFFER(g_W);
    FLUSH_BUFFER(g_S);
    // reset buffer heads
    mag_index = -1 ; bdot_index = -1 ; omega_index = -1 ; sol_index = -1 ; L_err_index = -1 ;
    // calculate target angular momentum
    MATVECMUL(g_L_target,MOI,g_W_target);
    return 0 ;
}