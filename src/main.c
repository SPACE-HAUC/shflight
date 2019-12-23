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
    int boot_count = bootCount() ;
    
    if ( boot_count == 0 )
        g_program_state = SH_ACS_DETUMBLE ; // on first boot, go to detumble state
    else
        g_program_state = SH_SYS_INIT ;
    
    fprintf(stderr, "State: %d\n", g_program_state) ;
    return 0 ;
}