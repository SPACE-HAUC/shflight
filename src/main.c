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
    // write zeros into all buffers and reset heads
    FLUSH_BUFFER_ALL ;
    // calculate target angular momentum
    MATVECMUL(g_L_target,MOI,g_W_target);
    // initialize h-bridge
    hbridge = (ncv7708 *) malloc(sizeof(ncv7708)) ;
    snprintf(hbridge->fname,40,HBRIDGE_DEV_NAME);
    int hbridge_init_status = hbridge_init(hbridge);

    ncv7708_destroy(hbridge);
    return 0 ;
}