/**
 * @file datalogger.h
 * @author Mit Bailey (mitbailey99@gmail.com)
 * @brief Declarations of datalogger functions and structures.
 * @version 0.5
 * @date 2021-04-07
 * 
 * @copyright Copyright (c) 2021
 * 
 */

#ifndef DATALOGGER_H
#define DATALOGGER_H

#ifndef eprintf
#define eprintf(str, ...)                                                    \
    fprintf(stderr, "%s, %d: " str "\n", __func__, __LINE__, ##__VA_ARGS__); \
    fflush(stderr);
#endif

// File and directories cannot exceed these limits.
// #define MAX_FILE_SIZE 0x100000 // 1MB
#define MAX_FILE_SIZE 0x10 // FOR DEBUG PURPOSES ONLY.
#define MAX_FNAME_SIZE 0x80
#define MAX_VAR_SIZE 0x100000
// #define MAX_LOG_SET_SIZE 0xC800000 // 200MB
#define MAX_LOG_SET_SIZE 0x40 // FOR DEBUG PURPOSES ONLY.

#define CALCULATE_NEXT_FILE_SIZE ({fseek(var_log_f, 0, SEEK_END); log_file_size = ftell(var_log_f); fseek(var_log_f, 0, SEEK_SET); fclose(var_log_f); log_file_size + var_size;})

// #define CALCULATE_NEXT_FILE_SIZE ({struct stat stbuf[1]; stat(fname_buf, stbuf); stbuf->st_size;})

/**
 * @brief INTERNAL USE ONLY. Registers named data with a byte-size.
 * 
 * Creates a var_name.reg file containing the var_size, and an initial var_name.idx file initially containing the value 0, and iterated each time a new log file is created for this var_name.
 * 
 * @param var_name The name to register.
 * @param var_size The byte-size to register.
 * @return int Negative on failure, 1 on success.
 */
int dlgr_register(const char* var_name, int var_size);

/**
 * @brief INTERNAL USE ONLY. Writes named data to a log.
 * 
 * @param var_name The name of the data to store.
 * @param data The data to store.
 * @return int 
 */
int dlgr_write(const char* var_name, void* data);

/**
 * @brief INTERNAL USE ONLY. Returns the number of bytes needed to store a number of read named-data of some previously defined (by dlgr_register()) size.
 * 
 * Failure modes include if var_name has not been previously registered.
 * 
 * @param var_name The name of the data to be read.
 * @param number The number of these data chunks to be read.
 * @return int Negative on failure, bytes required to store this data.
 */
int dlgr_prime_read(const char* var_name, int number);

/**
 * @brief INTERNAL USE ONLY. Reads data from logs and stores it.
 * 
 * Failure modes include if dlgr_prime_read() was not called immediately prior. This is checked by seeing if number * primed_varname_size = primed_byte_size.
 * 
 * @param storage Where the read data will be stored. 
 * @param number The number of data to be read (i.e. 'number'-many ints).
 * @return int Negative on failure, positive on success.
 */
int dlgr_perform_read(void* storage, int number);

/**
 * @brief 
 * 
 * @param var_name 
 * @param fname_buf_size 
 * @return int 
 */
int dlgr_check_registration(const char* var_name, const int fname_buf_size);

/**
 * @brief INTERNAL USE ONLY. Returns the index of the current log.
 * 
 * @param var_name Name of the variable whose log index to check.
 * @return int Negative on failure, log index on success.
 */
int dlgr_get_log_index(const char* var_name);

/**
 * @brief INTERNAL USE ONLY. Iterates the log index up by one.
 * 
 * @param var_name Name of the variable whose log index to iterate.
 * @return int Negative on failure, new log index on success.
 */
int dlgr_iterate_log_index(const char* var_name);

/**
 * @brief 
 * 
 * @param var_name 
 * @return int 
 */
int dlgr_count_logs(const char* var_name);

#endif // DATALOGGER_H