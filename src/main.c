#include <main.h>
#include <main_helper.h>

/* 
 * Variable naming convention: separated by underscore, clear naming. Globals
 * require a `g_' in front.
 * 
 * Function naming convention: capitalize letters of words after the first one.
 */

int main(int argc, char * argv[])
{
    g_boot_count = bootCount() ; // determines system boot count
    g_program_state = SH_SYS_INIT ; // On first boot, similar to detumble. On next boots, check all sensors and select appropriate state
    fprintf(stderr, "State: %d\n", g_program_state) ;
    g_bootup = 1 ; // the program is booting up
    // write zeros into all buffers
    for ( uint8_t counter = SH_BUFFER_SIZE; counter > 0; )
    {
        counter-- ;
        
        g_Bx[counter] = 0 ; g_By[counter] = 0 ; g_Bz[counter] = 0 ;

        g_Wx[counter] = 0 ; g_Wy[counter] = 0 ; g_Wz[counter] = 0 ;

        g_Sx[counter] = 0 ; g_Sy[counter] = 0 ; g_Sz[counter] = 0 ;
    }
    return 0 ;
}