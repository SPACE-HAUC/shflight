#include <main_helper.h>
#ifndef DEBUG_ALWAYS_S0
int bootCount() // returns current boot count, and increases by 1 and stores it in nvmem
// expected to be invoked only by _main()
{
    int _bootCount = 0 ; // assume 0 boot
    if ( access(BOOTCOUNT_FNAME, F_OK) != -1 ) //file exists
    {
        FILE * fp ; 
        fp = fopen(BOOTCOUNT_FNAME, "r+") ; // open file for reading
        int read_bytes = fscanf(fp,"%d",&_bootCount) ; // read bootcount
        if(read_bytes < 0)
            perror("File not read") ;
        fclose(fp) ; // close
        fp = fopen(BOOTCOUNT_FNAME, "w") ; // reopen to overwrite
        fprintf(fp, "%d", ++_bootCount) ; // write var+1
        fclose(fp) ; // close
        sync() ; // sync file system
    }
    else //file does not exist, create it
    {
        FILE * fp ;
        fp = fopen(BOOTCOUNT_FNAME, "w") ; // open for writing
        fprintf(fp, "%d", ++_bootCount) ; // write 1
        fclose(fp) ; // close
        sync() ; // sync file system
    }
    return --_bootCount ; // return 0 on first boot, return 1 on second boot etc
}
#else
int bootCount()
{
    return 0 ;
}
#endif // DEBUG_ALWAYS_S0