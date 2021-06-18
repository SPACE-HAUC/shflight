/**
 * @file datalogger.c
 * @author Mit Bailey (mitbailey99@gmail.com)
 * @brief Implementation of datalogger functions.
 * @version 0.6
 * @date 2021-04-07
 * 
 * @copyright Copyright (c) 2021
 * 
 */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "datalogger.h"
#include "datalogger_extern.h"

#define FILE_EXISTS ({snprintf(fname_buf, fname_buf_size, "%s_%d.log", var_name, var_index--); access(fname_buf, F_OK | R_OK);}) == 0

// Example Directory (NEW)
/* datalogger/
 * read_primer.tmp      <-- Holds info from last prime_read if read hasnt been called yet (read clears it).
 * acs_settings.cfg     <-- Holds module-specific logging settings.
 * acs_VAR1.idx         <-- Holds current log index of this variable.
 * acs_VAR1_0.log       <-- MODULE_VARIABLE_LOGNUMBER.dat
 * acs_VAR1_1.log
 * acs_VAR2.idx
 * acs_VAR2_0.log
 * <Other Modules>
 */

// settings.cfg file format (index moved to index.inf)
/* 1. max file size (Bytes)
 * 2. max dir size (Bytes)
 */

// char* moduleName is just a placeholder. Later, we will get the
// module names from somewhere else.

int dlgr_register(const char* var_name, int var_size){
    // Check if var_name is NULL.
    if (var_name == NULL){
        eprintf("Variable name is NULL.");
        return -1;
    }

    // Check if var_size is valid.
    if ((var_size < 1) || (var_size > MAX_VAR_SIZE)){
        eprintf("Variable size %d invalid.", var_size);
        return -1;
    }

    // Calculate filename buffer size, create it.
    const int fname_buf_size = strlen(var_name) + 7; // 7 == sizeof("_0.reg") + '\0'
    char fname_buf[fname_buf_size];

    // var_name should actually be modulename_variablename
    // Registration files take the form module_variable.reg, ie acs_x.reg
    // Construct the registration file's name.
    snprintf(fname_buf, fname_buf_size, "%s.reg", var_name); 

    // Check if the registration file exists.
    if ((access(fname_buf, F_OK | R_OK)) == 0)
    {
        // File exists.
        eprintf("WARNING: Variable name %s was previously registered in %s. Overwriting old registration.", var_name, fname_buf);
    }
    else
    {
        // File does not exist.
        eprintf("Registering new variable name %s in %s.", var_name, fname_buf);
    }

    // Open the registration file.
    FILE* var_registration_f = fopen(fname_buf, "w");
    if(var_registration_f == NULL){
        eprintf("Registration failed: Could not open %s for writing.", fname_buf);
        return -1;
    }

    // Write the variable size to the registration file.
    int retval = fprintf(var_registration_f, "%d", var_size);
    if(retval <= 0) {
        eprintf("Registration failed: Writing to registration file failed with value %d.", retval);
        fclose(var_registration_f);
        return -1;
    }

    // Close the registration file.
    fclose(var_registration_f);

    // Zero-out filename buffer, construct index file name.
    // memset(fname_buf, 0x0, fname_buf_size);
    snprintf(fname_buf, fname_buf_size, "%s.idx", var_name);

    // Create index file.
    FILE* var_index_f = fopen(fname_buf, "w");
    if(var_index_f == NULL){
        eprintf("Registration failed: Could not open %s for writing.", fname_buf);
        return -1;
    }

    // Write the initial index '0' to the idx file.
    retval = fprintf(var_index_f, "%d", 0);
    if((fprintf(var_index_f, "%d", 0)) <= 0){
        eprintf("Registration failed: Writing to index file failed with value %d.", retval);
        fclose(var_index_f);
        return -1;
    }

    fclose(var_index_f);

    // Create an initial _0.log file.
    snprintf(fname_buf, fname_buf_size, "%s_%d.log", var_name, 0);

    // Create the log file.
    FILE* var_log_f = fopen(fname_buf, "w");
    if(var_log_f == NULL){
        eprintf("Registration failed: Could not open %s for writing.", fname_buf);
        return -1;
    }

    fclose(var_log_f);

    return 1;
}

int dlgr_write(const char* var_name, void* data){
    // eprintf("Write started.");

    // Check if var_name is NULL.
    if (var_name == NULL){
        eprintf("Variable name is NULL.");
        return -1;
    }
    
    // Check if data is NULL.
    if (data == NULL){
        eprintf("Data is NULL.");
        return -1;
    }

    const int fname_buf_size = strlen(var_name) + 17; // 16 == sizeof("_nnnnnnnnnn.log") + '\0'
    // eprintf("Size of strlen(%s) + 5 = %d", var_name, fname_buf_size);
    int var_size = dlgr_check_registration(var_name, fname_buf_size);

    char fname_buf[fname_buf_size];

    // Get this variable's current log file index.
    int var_index = dlgr_get_log_index(var_name);
    if(var_index < 0){
        eprintf("Write failed: Index acquisition failed with value %d.", var_index);
        return -1;
    }

    // Create the full log file name.
    snprintf(fname_buf, fname_buf_size, "%s_%d.log", var_name, var_index);

    // Check if this log file exists.
    if ((access(fname_buf, F_OK | R_OK)) != 0){
        eprintf("Log file %s does not exist.", fname_buf);
        return -1;
    }

    // Open the log file.
    FILE* var_log_f = fopen(fname_buf, "ab");
    if(var_log_f == NULL){
        eprintf("Write failed: Could not open %s.", fname_buf);
        return -1;
    }

    // If the file will exceed MAX, iterate to next file.
    // If the next file exists, measure its size.

    // Find the size of the log file.
    fseek(var_log_f, 0, SEEK_END);
    int log_file_size = ftell(var_log_f);
    fseek(var_log_f, 0, SEEK_SET);

    if((log_file_size + var_size) > MAX_FILE_SIZE){
        fclose(var_log_f);
        do {
            // Create the full log file name and iterate the index.
            snprintf(fname_buf, fname_buf_size, "%s_%d.log", var_name, var_index = dlgr_iterate_log_index(var_name));
            // eprintf("File name: %s", fname_buf);
            // While the log file of the next index exists and its size plus our variable's size is greater than the maximum allowable file size.
            // Until the log file of the next index does not exist or the log file of the next index's size plus our variable's size is less than the maximum allowable file size.
        } while ((var_log_f = fopen(fname_buf, "rb")) != NULL && CALCULATE_NEXT_FILE_SIZE > MAX_FILE_SIZE);
        
        // When we get here, var_log_f is CLOSED and the index indicates either
        // A) a log file that does not yet exist, because the previous one is full, or
        // B) a non-full log file.
        // Here we open to append if it exists, or write if it doesn't.
        if ((access(fname_buf, F_OK | R_OK)) == 0){
            // File exists.
            if((var_log_f = fopen(fname_buf, "ab")) == NULL){
                eprintf("Failed to open %s.", fname_buf);
                return -1;
            }
        } else {
            // File does not exist.
            if((var_log_f = fopen(fname_buf, "wb")) == NULL){
                eprintf("Failed to create %s.", fname_buf);
                return -1;
            }
        }
    }

    // Write our data to the log file.
    // int retval = fwrite(data, var_size, 1, var_log_f);
    int retval = write(fileno(var_log_f), data, var_size);
    if(retval <= 0){
        eprintf("Write failed: Failed to write to %s: wrote %d bytes.", fname_buf, retval);
        fclose(var_log_f);
        return -1;
    }

    // eprintf("Wrote %d bytes.", retval);

    // Close the log file.
    fclose(var_log_f);
    sync();

    // Check if we need to delete an old file and do so.
    const int max_logs = MAX_LOG_SET_SIZE / MAX_FILE_SIZE;
    snprintf(fname_buf, fname_buf_size, "%s_%d.log", var_name, var_index - max_logs);
    // eprintf("Checking #%d.", var_index - max_logs);
    if ((var_index >= max_logs) && (access(fname_buf, F_OK | R_OK) == 0)) {
        // eprintf("DELETING #%d.", var_index - max_logs);
        remove(fname_buf);
    }

    // eprintf("Write finished.");
    return 1;
}

int dlgr_prime_read(const char* var_name, int number){
    // Check if var_name is NULL.
    if (var_name == NULL){
        eprintf("Variable name is NULL.");
        return -1;
    }

    // Check if number is invalid.
    if (number <= 0){
        eprintf("Number is invalid.");
        return -1;
    }

    const int fname_buf_size = strlen(var_name) + 17; // 16 == sizeof("_nnnnnnnnnn.log") + '\0'
    int var_size = dlgr_check_registration(var_name, fname_buf_size);

    char fname_buf[fname_buf_size];

    int required_bytes = var_size * number;

    // Create the full read primer file name.
    snprintf(fname_buf, fname_buf_size, "read_primer.tmp", var_name);

    // Open the primer file.
    FILE* var_primer_f = fopen(fname_buf, "w");
    if(var_primer_f == NULL){
        eprintf("Could not open %s.", fname_buf);
        return -1;
    }

    // Write the number of required bytes we calculated.
    fprintf(var_primer_f, "%d\n%d\n%s", number, required_bytes, var_name);

    fclose(var_primer_f);
    sync();
    return required_bytes;
}

int dlgr_perform_read(void* storage, int allocated_bytes){
    if(storage == NULL){
        eprintf("Storage is NULL.");
        return -1;
    }

    unsigned char* storage_ptr = (unsigned char *) storage;

    if(allocated_bytes <= 0){
        eprintf("Allocated bytes is invalid.");
        return -1;
    }

    const char* primer_fname_buf = "read_primer.tmp";

    if((access(primer_fname_buf, F_OK | R_OK)) != 0){
        eprintf("No read is primed! Call DLGR_PRIME_READ(varname, number)");
        return -1;
    }

    FILE* var_primer_f = fopen(primer_fname_buf, "r");
    if(var_primer_f == NULL){
        eprintf("Could not open %s.", primer_fname_buf);
        return -1;
    }

    // Get our primed var_name and required bytes.
    int number_requested = 0, required_bytes = 0;
    char var_name[128];
    fscanf(var_primer_f, "%d %d %s", &number_requested, &required_bytes, var_name);

    // Compare it to what we calculate now.
    if(allocated_bytes != required_bytes){
        eprintf("Cannot continue, descrepancy found between primed byte value (%d) and passed byte value (%d).", required_bytes, allocated_bytes);
        return -1;
    }
    
    // Clean up.
    fclose(var_primer_f);
    remove(primer_fname_buf);

    // At this point we are now ready to read from the logs.

    // Establish the buffer size, get the variable size, check if the variable was registered, and create a filename buffer.
    const int fname_buf_size = strlen(var_name) + 17; // 16 == sizeof("_nnnnnnnnnn.log") + '\0'
    int var_size = dlgr_check_registration(var_name, fname_buf_size);

    if((var_size * number_requested) != required_bytes){
        eprintf("Variable size (%d) times the number requested (%d) does not equal the primed required bytes (%d).", var_size, number_requested, required_bytes);
        return -1;
    }

    char fname_buf[fname_buf_size];

    // Get current log index.
    int var_index = dlgr_get_log_index(var_name);

    // Construct the log file name.
    snprintf(fname_buf, fname_buf_size, "%s_%d.log", var_name, var_index);

    // Read from logs.

    FILE* var_log_f = fopen(fname_buf, "rb");
    if(var_log_f == NULL){
        eprintf("Cannot open %s.", fname_buf);
        return -1;
    }

    // Go to end of file, pull back var_size bytes, read var_size bytes.
    // Pull back 2*var_size bytes, read var_size bytes.
    // etc
    int number_read = 0, number_read_this_file = 0/*, retval = 0*/;
    while(number_read < number_requested){
        // eprintf("%d %d", number_read, number_read_this_file);

        fseek(var_log_f, -((number_read_this_file + 1) * var_size), SEEK_END);
        /*retval = */ fread(&storage_ptr[number_read * var_size], var_size, 0x1, var_log_f);
        // retval = read(fileno(var_log_f), &storage_ptr[number_read * var_size], var_size);

        // eprintf("retval: %d", retval);
        // eprintf("read: 0x%02x", storage_ptr[number_read * var_size]);

        number_read++;
        number_read_this_file++;

        // If we just read the first variable in the file, seamlessly iterate back a log index.
        // eprintf("ftell(var_log_f) - var_size: %d - %d = %d", ftell(var_log_f), var_size, ftell(var_log_f) - var_size);
        if(((ftell(var_log_f) - var_size) == 0) /* && (var_index > 0)*/){

            // If this then we're done, no more we can read.
            if (var_index == 0){
                // eprintf("Reached end of available data to read.");
                break;
            }

            fclose(var_log_f);
            var_index--;
            snprintf(fname_buf, fname_buf_size, "%s_%d.log", var_name, var_index);
            if((var_log_f = fopen(fname_buf, "rb")) == NULL){
                eprintf("Failed to open %s.", fname_buf);
                return -1;
            }
            number_read_this_file = 0;
        }
    }

    fclose(var_log_f);
    
    return 1;
}

int dlgr_check_registration(const char* var_name, const int fname_buf_size){
    // Establish filename buffer and its size.
    char fname_buf[fname_buf_size];

    // Create the full registration file name, and check for registration.
    snprintf(fname_buf, fname_buf_size, "%s.reg", var_name);
    if((access(fname_buf, F_OK || R_OK)) != 0){
        eprintf("Failed: %s has not been registered.", var_name);
        return -1;
    }

    // Open the registration file.
    FILE* var_registration_f = fopen(fname_buf, "r");
    if(var_registration_f == NULL){
        eprintf("Failed: Could not open %s.", fname_buf);
        return -1;
    }

    // Get the variable size from the registration file.
    int var_size = 0;
    fscanf(var_registration_f, "%d", &var_size);
    if(var_size <= 0){
        eprintf("Failed: var_size invalid (%d).", var_size);
        fclose(var_registration_f);
        return -1;
    }

    // Close the registration file.
    fclose(var_registration_f);
    return var_size;
}

int dlgr_get_log_index(const char* var_name){
    const int fname_buf_size = strlen(var_name) + 5; // 5 == sizeof(".idx") + '\0'
    char fname_buf[fname_buf_size];

    snprintf(fname_buf, fname_buf_size, "%s.idx", var_name);

    // Open index file.
    FILE* var_index_f = fopen(fname_buf, "r");
    if(var_index_f == NULL){
        eprintf("Could not open %s for reading.", fname_buf);
        return -1;
    }

    // Read the index.
    int var_index = 0;
    fscanf(var_index_f, "%d", &var_index);

    fclose(var_index_f);

    return var_index;
}

// Returns new log index
int dlgr_iterate_log_index(const char* var_name){
    const int fname_buf_size = strlen(var_name) + 5; // 5 == sizeof(".idx") + '\0'
    char fname_buf[fname_buf_size];

    snprintf(fname_buf, fname_buf_size, "%s.idx", var_name);

    // Open index file.
    FILE* var_index_f = fopen(fname_buf, "r");
    if(var_index_f == NULL){
        eprintf("Could not open %s for reading.", fname_buf);
        return -1;
    }

    // Read the index.
    int var_index = 0;
    fscanf(var_index_f, "%d", &var_index);

    fclose(var_index_f);

    var_index++;

    var_index_f = fopen(fname_buf, "w");
    if(var_index_f == NULL){
        eprintf("Could not open %s for writing.", fname_buf);
        return -1;
    }

    // Write the iterated index to the idx file.
    int retval = fprintf(var_index_f, "%d", var_index);
    if(retval <= 0){
        eprintf("Writing to index file failed with value %d.", retval);
        fclose(var_index_f);
        return -1;
    }

    fclose(var_index_f);

    return var_index;
}

int dlgr_count_logs(const char* var_name){
    const int fname_buf_size = strlen(var_name) + 17; // 16 == sizeof("_nnnnnnnnnn.log") + '\0'
    char fname_buf[fname_buf_size];
    int log_count = 0;
    for(int var_index = dlgr_get_log_index(var_name); FILE_EXISTS; log_count++);

    return log_count;
}