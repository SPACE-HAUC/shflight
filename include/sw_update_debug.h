#include <stdint.h>
#include <unistd.h>
#include <stdio.h>
#include "sw_update_packdef.h"

#ifndef SW_UPDATE_DEBUG_H
#define SW_UPDATE_DEBUG_H

/**
 * @brief Prints n bytes starting at address pointed to by m in hexadecimal with prefix and newline.
 * 
 * @param m 
 * @param n 
 */
static void memprintl_hex(void *m, ssize_t n)
{
    eprintlf(NOTIFY "(%p) ", m);
    for (int i = 0; i < n; i++)
    {
        printf("%02hhx", *((unsigned char *)m + i));
    }
    printf("(END)\n");
    fflush(stdout);
}

/**
 * @brief Prints n bytes starting at address pointed to by m in hexadecimal.
 * 
 * @param m 
 * @param n 
 */
static void memprint_hex(void *m, ssize_t n)
{
    for (int i = 0; i < n; i++)
    {
        printf("%02hhx", *((unsigned char *)m + i));
    }
    fflush(stdout);
}

/**
 * @brief Prints n bytes starting at address pointed to by m in characters with prefix and newline.
 * 
 * @param m 
 * @param n 
 */
static void memprintl_char(void *m, ssize_t n)
{
    eprintlf(NOTIFY "(%p) ", m);
    unsigned char c;
    for (int i = 0; i < n; i++)
    {
        c = *((unsigned char *)m + i);
        if (c == '\0')
        {
            printf(" ");
        }
        else
        {
            printf("%c", *((unsigned char *)m + i));
        }
    }
    printf("(END)\n");
    fflush(stdout);
}

/**
 * @brief 
 * 
 * @param sr_hdr 
 */
static void print_sr_pmr(sw_upd_startresume_t *sr_hdr)
{
    eprintf(NOTIFY "| S/R PRIMER PRINTOUT");
    memprintl_hex(sr_hdr, sizeof(sw_upd_startresume_t));
    eprintf(NOTIFY "| cmd -------------- 0x%02hhx", sr_hdr->cmd);
    eprintf(NOTIFY "| Filename --------- %s", sr_hdr->filename);
    eprintf(NOTIFY "| FID -------------- %d", sr_hdr->fid);
    eprintf(NOTIFY "| Bytes sent ------- %d", sr_hdr->sent_bytes);
    eprintf(NOTIFY "| Total bytes ------ %d", sr_hdr->total_bytes);
    fflush(stdout);
}

/**
 * @brief 
 * 
 * @param sr_rep 
 */
static void print_sr_rep(sw_upd_startresume_reply_t *sr_rep)
{
    eprintf(NOTIFY "| S/R PRIMER REPLY PRINTOUT");
    memprintl_hex(sr_rep, sizeof(sw_upd_startresume_reply_t));
    eprintf(NOTIFY "| cmd -------------- 0x%02hhx", sr_rep->cmd);
    eprintf(NOTIFY "| Filename --------- %s", sr_rep->filename);
    eprintf(NOTIFY "| FID -------------- %d", sr_rep->fid);
    eprintf(NOTIFY "| Bytes received --- %d", sr_rep->recv_bytes);
    eprintf(NOTIFY "| Total packets ---- %d", sr_rep->total_packets);
    fflush(stdout);
}

/**
 * @brief 
 * 
 * @param dt_hdr 
 */
static void print_dt_hdr(sw_upd_data_t *dt_hdr)
{
    eprintf(NOTIFY "| DATA HEADER PRINTOUT");
    memprintl_hex(dt_hdr, sizeof(sw_upd_data_t));
    eprintf(NOTIFY "| cmd -------------- 0x%02hhx", dt_hdr->cmd);
    eprintf(NOTIFY "| Packet number ---- %d", dt_hdr->packet_number);
    eprintf(NOTIFY "| Total bytes ------ %d", dt_hdr->total_bytes);
    eprintf(NOTIFY "| Data size -------- %d", dt_hdr->data_size);
    fflush(stdout);
}

/**
 * @brief 
 * 
 * @param dt_rep 
 */
static void print_dt_rep(sw_upd_data_reply_t *dt_rep)
{
    eprintf(NOTIFY "| DATA HEADER REPLY PRINTOUT");
    memprintl_hex(dt_rep, sizeof(sw_upd_data_reply_t));
    eprintf(NOTIFY "| cmd -------------- 0x%02hhx", dt_rep->cmd);
    eprintf(NOTIFY "| Packet number ---- %d", dt_rep->packet_number);
    eprintf(NOTIFY "| Total packets ---- %d", dt_rep->total_packets);
    eprintf(NOTIFY "| Received --------- %d", dt_rep->received);
    fflush(stdout);
}

/**
 * @brief 
 * 
 * @param cf_hdr 
 */
static void print_cf_hdr(sw_upd_conf_t *cf_hdr)
{
    eprintf(NOTIFY "| CONF HEADER PRINTOUT");
    memprintl_hex(cf_hdr, sizeof(sw_upd_conf_t));
    eprintf(NOTIFY "| cmd -------------- 0x%02hhx", cf_hdr->cmd);
    eprintf(NOTIFY "| Packet number ---- %d", cf_hdr->packet_number);
    eprintf(NOTIFY "| Total packets ---- %d", cf_hdr->total_packets);
    eprintf(NOTIFY "| Hash ------------- ");
    memprint_hex(cf_hdr->hash, HASH_SIZE);
    printf("\n");
    fflush(stdout);
}

/**
 * @brief 
 * 
 * @param cf_rep 
 */
static void print_cf_rep(sw_upd_conf_reply_t *cf_rep)
{
    eprintf(NOTIFY "| CONF REPLY PRINTOUT");
    memprintl_hex(cf_rep, sizeof(sw_upd_conf_reply_t));
    eprintf(NOTIFY "| cmd -------------- 0x%02hhx", cf_rep->cmd);
    eprintf(NOTIFY "| Request packet --- %d", cf_rep->request_packet);
    eprintf(NOTIFY "| Total packets ---- %d", cf_rep->total_packets);
    eprintf(NOTIFY "| Hash ------------- ");
    memprint_hex(cf_rep->hash, HASH_SIZE);
    printf("\n");
    fflush(stdout);
}

#endif // SW_UPDATE_DEBUG_H