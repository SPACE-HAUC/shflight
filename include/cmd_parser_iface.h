/**
 * @file cmd_parser_iface.h
 * @author Sunip K. Mukherjee (sunipkmukherjee@gmail.com)
 * @brief 
 * @version 0.1
 * @date 2021-06-18
 * 
 * @copyright Copyright (c) 2021
 * 
 */

#ifndef _CMD_PARSER_IFACE_H
#define _CMD_PARSER_IFACE_H
int prsr_init(void);
void *prsr_thread(void *tid);
void prsr_destroy(void);
#endif