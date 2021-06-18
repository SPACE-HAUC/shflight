/**
 * @file sw_update_sh.h
 * @author Mit Bailey (mitbailey99@gmail.com)
 * @author Sunip K. Mukherjee (sunipkmukherjee@gmail.com)
 * @brief 
 * @version 0.2
 * @date 2021-05-10
 * 
 * @copyright Copyright (c) 2021
 * 
 */

#ifndef _SW_UPDATE_SH_H
#define _SW_UPDATE_SH_H
#ifdef __cplusplus
// extern "C"
// {
#endif

#include <termios.h>
#include <stdbool.h>
#include "sw_update_packdef.h"

// If not commented out, this activates the debug modes for _sh. This enables command line argument intake.
#define DEBUG_MODE_ACTIVE_SH

#define MAX_WRITE_ATTEMPTS 64
#define MAX_READ_ATTEMPTS 64

// This structure will help us to read the temporary binary file which includes the CRCs.
typedef struct __attribute__((packed))
{
    uint32_t crc;
    char data[DATA_SIZE_MAX];
} sw_upd_tmpfile_t;

/**
 * @brief Initialize serial device with baud.
 * 
 * @param sername Serial device name.
 * @param baud Baud rate (e.g. B9600, from termios.h).
 * @return sw_sh_dev Positive on success, negative on error (0 = stdin, 1 = stdout, 2 = stderr).
 */
uhf_modem_t sw_sh_init(const char *sername);

/**
 * @brief Called when expecting to get a request to write a file.
 * 
 * @param fd Serial value.
 * @param done_recv If True, ends immediately.
 * @return int Negative on failure, positive on success.
 */
int sw_sh_receive_file(uhf_modem_t fd, bool *done_recv);

/**
 * @brief Writes data to the temporary file.
 * 
 * A sw_sh_receive_file helper function.
 * 
 * @param data The data to write.
 * @param data_size Amount of data to write.
 * @param filename The filename to write to. 
 * @param byte_location The number of bytes offset from the start to begin writing.
 * @return ssize_t 
 */
ssize_t sw_sh_write_to_file(char *data, ssize_t data_size, char filename[], int byte_location);

/**
 * @brief Retrieve the number of bytes received of a file.
 * 
 * @param filename Name of the file for which to check the number of received bytes.
 * @return ssize_t Number of received bytes.
 */
ssize_t sw_sh_get_recv_bytes(const char filename[]);

/**
 * @brief Set the number of bytes received of a file.
 * 
 * @param filename Name of the file for whcih to check the number.
 * @param sent_bytes The value to update to.
 * @return int Negative on failure, positive on success.
 */
int sw_sh_set_recv_bytes(const char filename[], ssize_t sent_bytes);

/**
 * @brief 
 * 
 * @param dev 
 */
void sw_sh_destroy(uhf_modem_t fd);

#ifdef __cplusplus
// }
#endif
#endif // _SW_UPDATE_SH_H