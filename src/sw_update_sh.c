/**
 * @file sw_update_sh.c
 * @author Mit Bailey (mitbailey99@gmail.com)
 * @author Sunip K. Mukherjee (sunipkmukherjee@gmail.com)
 * @brief 
 * @version 0.2
 * @date 2021-05-09
 * 
 * @copyright Copyright (c) 2021
 * 
 */

#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <uhf_modem/uhf_modem.h>
#include "sw_update_sh.h"
#include "sw_update_debug.h"

// Debug-mode variables.
#ifdef DEBUG_MODE_ACTIVE_SH

int fail_times_sr = 0;
int flip_byte_sr = 0;

int fail_times_dt = 0;
int flip_byte_dt = 0;

int fail_times_cf = 0;
int flip_byte_cf = 0;

#endif // DEBUG_MODE_ACTIVE_SH

uhf_modem_t sw_sh_init(const char *sername)
{
    return uhf_init(sername);
}

int sw_sh_receive_file(uhf_modem_t fd, bool *done_recv)
{
    // The SH end gets its orders from the ground. So, it'll assume its waiting for a S/R primer, but switches b/w S/R and DATA depending on what it gets.

    eprintf(NOTIFY "fd: %d", (int)fd);

    if (fd < 3)
    {
        eprintf(ERR "fd < 3: %d", (int)fd);
        return -1;
    }

    // Reset the condition.
    *done_recv = false;

    eprintf(NOTIFY "Starting SPACE-HAUC's send file executor...");

    // Variables.
    // Note that unlike the GS, some of these variables cannot be filled in until we have more information from the ground.
    char filename[20];
    ssize_t file_size = 0;
    ssize_t recv_bytes = 0;
    int recv_packets = 0; 
    // packets = (bytes / DATA_SIZE_MAX) + ((bytes % DATA_SIZE_MAX) > 0);
    int recv_attempts = 0;
    int max_packets = 0;
    ssize_t retval = 0;

    // Our initial mode is to begin by expecting a primer to tell us the information we require.
    sw_upd_mode mode = primer;

    // Read and write buffers.
    char rd_buf[PACKET_SIZE];
    char wr_buf[PACKET_SIZE];

    eprintf(NOTIFY "~ Entering FILE TRANSFER phase. ~");

    // Outer loop. Runs until we have received the entire file.
    while (!(*done_recv))
    {
        switch (mode)
        {
        case primer:
        {
            // Map a S/R header to the read buffer.
            sw_upd_startresume_t *sr_pmr = (sw_upd_startresume_t *)rd_buf;

            // Map a S/R reply to the write buffer.
            sw_upd_startresume_reply_t *sr_rep = (sw_upd_startresume_reply_t *)wr_buf;

            for (recv_attempts = 0; (mode == primer) && (recv_attempts < MAX_RECV_ATTEMPTS) && !(*done_recv); recv_attempts++)
            {
                eprintf(NOTIFY "Waiting for START/RESUME primer...");

                // Wait for a primer...
                memset(rd_buf, 0x0, PACKET_SIZE);
                retval = uhf_read(fd, rd_buf, PACKET_SIZE);

                // Check if a repeat request is necessary.
                if (retval < 0)
                {
                    eprintf(WARN "START/RESUME primer reading failed with value %d.", retval);

                    // Ask for a repeat.
                    retval = uhf_write(fd, rept_cmd, sizeof(rept_cmd));

                    if (retval <= 0)
                    {
                        eprintf(ERR "REPT command writing failed with value %d.", retval);
                    }
                    else
                    {
                        eprintf(NOTIFY "REPT command written successfully (%d).", retval);
                    }

                    continue;
                }
                else if (retval == 0)
                {
                    eprintf(WARN "START/RESUME primer reading timed out with value %d.", retval);
                    // continue;
                    return 0;
                }

#ifdef DEBUG_MODE_ACTIVE_SH

                if (fail_times_sr > 0)
                {
                    eprintf(WARN "Flipping byte %d from 0x%02hhx to 0x%02hhx.", flip_byte_sr, rd_buf[flip_byte_sr], ~rd_buf[flip_byte_sr]);
                    rd_buf[flip_byte_sr] = ~rd_buf[flip_byte_sr];

                    fail_times_sr--;

                    eprintf(WARN "Error induced. Press Enter to continue...");
                    getchar();
                }

#endif // DEBUG_MODE_ACTIVE_SH

                // Check if we read in a repeat command.
                if (!memcmp(rept_cmd, rd_buf, 5))
                { // We read in a REPT command, so, repeat last.
                    // This SHOULD NEVER OCCUR in SH's S/R section...
                    eprintf(WARN "Repeat of previously sent transmission requested. Sending a duplicate.");
                    retval = uhf_write(fd, wr_buf, PACKET_SIZE);

                    if (retval <= 0)
                    {
                        eprintf(ERR "Repeated message writing failed with value %d.", retval);
                    }
                    else
                    {
                        eprintf(NOTIFY "Repeated message written successfully (%d).", retval);
                    }

                    continue;
                }

                // If we get here we no longer need the content in wr_buf.
                memset(wr_buf, 0x0, PACKET_SIZE);

                // Print the S/R header.
                print_sr_pmr(sr_pmr);
                memprintl_hex(rd_buf, PACKET_SIZE);
                memprintl_char(rd_buf, PACKET_SIZE);

                // Content checks.
                if (sr_pmr->cmd != SW_UPD_SRID)
                {
                    if (sr_pmr->cmd == SW_UPD_DTID)
                    {
                        eprintf(WARN "The CMD value 0x%02x matches a DATA CMD, not a START/RESUME CMD as was expected. Sending START/RESUME reply, and switching to DATA mode.", sr_pmr->cmd);
                        mode = data;
                    }
                    else if (sr_pmr->cmd == SW_UPD_CFID)
                    {
                        eprintf(WARN "The CMD value 0x%02x matches a CONF CMD, not a START/RESUME CMD as was expected. Sending START/RESUME reply, and switching to CONF mode.", sr_pmr->cmd);
                        mode = confirmation;
                    }
                    else
                    {
                        eprintf(WARN "The CMD value 0x%02x matches no known CMDs. Sending START/RESUME reply, will await a legitimate CMD.", sr_pmr->cmd);
                    }
                }
                
                // Set our filename to what the primer's is.
                strcpy(filename, sr_pmr->filename);
                recv_bytes = sw_sh_get_recv_bytes(filename);
                recv_packets = (recv_bytes / DATA_SIZE_MAX) + ((recv_bytes % DATA_SIZE_MAX) > 0);
                
                if (sr_pmr->sent_bytes != sw_sh_get_recv_bytes(sr_pmr->filename)) 
                {
                    eprintf(WARN "The Ground Station sent a START/RESUME primer and claims it sent %d bytes, while SPACE-HAUC confirms receiving %d bytes. Proceeding to reply and remain in START/RESUME mode until byte transfer values agree.", sr_pmr->sent_bytes, sw_sh_get_recv_bytes(sr_pmr->filename));
                }
                else
                {
                    eprintf(NOTIFY "Received legitimate START/RESUME primer. Proceeding to reply and subsequently switch to DATA mode beginning at packet %d.", recv_packets);
                    file_size = sr_pmr->total_bytes;
                    mode = data;
                }

                // Set local variables based on new information.
                // (Some are set above the if-block).

                // Set the max_packets based on this new information.
                max_packets = (file_size / DATA_SIZE_MAX) + ((file_size % DATA_SIZE_MAX) > 0);

                // Construction of reply.
                sr_rep->cmd = SW_UPD_SRID;
                strcpy(sr_rep->filename, filename);
                sr_rep->fid = sr_pmr->fid;
                sr_rep->recv_bytes = recv_bytes;
                sr_rep->total_packets = max_packets;

                eprintf(NOTIFY "Attempting send of START/RESUME reply...");

                // Print S/R reply.
                print_sr_rep(sr_rep);
                memprintl_hex(wr_buf, PACKET_SIZE);
                memprintl_char(wr_buf, PACKET_SIZE);

                // Send S/R reply.
                retval = uhf_write(fd, wr_buf, PACKET_SIZE);

                if (retval <= 0)
                {
                    eprintf(ERR "START/RESUME reply writing failed with value %d.", retval);
                    continue;
                }
            }

            break; // case recv_primer
        }

        case data:
        {
            // Map a DATA header onto the read buffer.
            sw_upd_data_t *dt_hdr = (sw_upd_data_t *)rd_buf;

            // Map a DATA reply header to the write buffer.
            sw_upd_data_reply_t *dt_rep = (sw_upd_data_reply_t *)wr_buf;

            // Make sure the number of the expected packet is correct.
            // packet_number = recv_packets;

            eprintf(NOTIFY "Entering packet %d acquisition loop.", recv_packets);

            // Loop over trying to get this single packet until its acquired, or we decide to switch to S/R instead.
            for (recv_attempts = 0; (mode == data) && (recv_attempts < MAX_RECV_ATTEMPTS) && !(*done_recv); recv_attempts++)
            {

                eprintf(NOTIFY "Waiting for DATA packet...");

                // Read the DATA packet.
                memset(rd_buf, 0x0, PACKET_SIZE);
                retval = uhf_read(fd, rd_buf, PACKET_SIZE);

                // Check if a repeat request is necessary.
                if (retval < 0)
                {
                    eprintf(WARN "DATA packet reading failed with value %d.", retval);

                    // Ask for a repeat.
                    retval = uhf_write(fd, rept_cmd, sizeof(rept_cmd));

                    if (retval <= 0)
                    {
                        eprintf(ERR "REPT command writing failed with value %d.", retval);
                    }
                    else
                    {
                        eprintf(NOTIFY "REPT command written successfully (%d).", retval);
                    }

                    continue;
                }
                else if (retval == 0)
                {
                    eprintf(ERR "DATA packet reading timed out with value %d.");
                    // continue;
                    return 0;
                }

#ifdef DEBUG_MODE_ACTIVE_SH

                if (fail_times_dt > 0)
                {
                    eprintf(WARN "Flipping byte %d from 0x%02hhx to 0x%02hhx.", flip_byte_dt, rd_buf[flip_byte_dt], ~rd_buf[flip_byte_dt]);
                    rd_buf[flip_byte_dt] = ~rd_buf[flip_byte_dt];

                    fail_times_dt--;

                    eprintf(WARN "Error induced. Press Enter to continue...");
                    getchar();
                }

#endif // DEBUG_MODE_ACTIVE_SH

                // Check if we read in a repeat command.
                if (!memcmp(rept_cmd, rd_buf, 5))
                { // We read in a REPT command, so, repeat last.
                    eprintf(WARN "Repeat of previously sent transmission requested. Sending a duplicate.");
                    retval = uhf_write(fd, wr_buf, PACKET_SIZE);

                    if (retval <= 0)
                    {
                        eprintf(ERR "Repeated message writing failed with value %d.", retval);
                    }
                    else
                    {
                        eprintf(NOTIFY "Repeated message written successfully (%d).", retval);
                    }

                    continue;
                }

                // If we get here we no longer need the content in wr_buf.
                memset(wr_buf, 0x0, PACKET_SIZE);

                // Print the DATA packet.
                print_dt_hdr(dt_hdr);
                memprintl_hex(rd_buf, PACKET_SIZE);
                memprintl_char(rd_buf, PACKET_SIZE);

                // Construct a reply.
                dt_rep->cmd = SW_UPD_DTID;
                dt_rep->packet_number = recv_packets;
                dt_rep->total_packets = (dt_hdr->total_bytes / DATA_SIZE_MAX) + ((dt_hdr->total_bytes % DATA_SIZE_MAX) > 0);
                dt_rep->received = 0; // Assume the receive was bad until confirmed otherwise.

                if (dt_hdr->cmd != SW_UPD_DTID)
                {
                    if (dt_hdr->cmd == SW_UPD_SRID)
                    {
                        eprintf(WARN "The CMD value 0x%02x matches a START/RESUME CMD, not a DATA CMD as was expected. Sending negative acknowledgement, and switching to START/RESUME mode.", dt_hdr->cmd);
                        mode = primer;
                    }
                    else if (dt_hdr->cmd == SW_UPD_CFID)
                    {
                        eprintf(WARN "The CMD value 0x%02x matches a CONF CMD, not a DATA CMD as was expected. Sending negative acknowledgement, and switching to CONF mode.", dt_hdr->cmd);
                        mode = confirmation;
                    }
                    else
                    {
                        eprintf(WARN "The CMD value 0x%02x matches no known CMDs. Sending negative acknowledgement, will await a legitimate CMD.", dt_hdr->cmd);
                    }
                }
                else if (dt_hdr->packet_number != recv_packets)
                {
                    eprintf(WARN "The Ground Station's sent packet number (%d) does not match our expected packet number (%d). Requesting packet %d and switching to START/RESUME intake mode.", dt_hdr->packet_number, recv_packets, recv_packets);
                    mode = primer;
                }
                else
                {
                    eprintf(NOTIFY "Received legitimate DATA packet. Proceeding to reply and await subsequent DATA packet number %d.", recv_packets + 1);

                    // Write the successfully acquired and verified data.
                    retval = sw_sh_write_to_file(rd_buf + sizeof(sw_upd_data_t), dt_hdr->data_size, filename, recv_bytes);

                    if (retval == dt_hdr->data_size)
                    {
                        eprintf(NOTIFY "Successfully wrote %ld bytes (%d bytes of data) to file.", retval, dt_hdr->data_size);
                        recv_bytes += dt_hdr->data_size;
                        sw_sh_set_recv_bytes(filename, recv_bytes);
                        dt_rep->received = 1;
                        recv_packets++;
                    }
                    else
                    {
                        eprintf(ERR "Data write to file failed. Will request data packet again.");
                    }
                }

                eprintf(NOTIFY "Attempting send of DATA reply...");

                // Print DATA reply.
                print_dt_rep(dt_rep);
                memprintl_hex(wr_buf, PACKET_SIZE);
                memprintl_char(wr_buf, PACKET_SIZE);

                // Send DATA reply.
                retval = uhf_write(fd, wr_buf, PACKET_SIZE);

                if (retval <= 0)
                {
                    eprintf(ERR "DATA reply writing failed with value %d.", retval);
                    continue;
                }

                // Check if we have received the entire file. If this is the case, set finished to true. This will back us out of these loops.
                if (recv_bytes >= file_size)
                {
                    eprintf(NOTIFY "Received %ld/%ld bytes of file %s.", recv_bytes, file_size, filename);

                    mode = transfer_complete;
                    break;
                }
                else if (dt_rep->received == 1)
                {
                    // If we haven't received the entire file, but we did just successfully get a packet, then we should break from this loop (since this loop runs over a single packet).
                    eprintf(NOTIFY "Moving to next data packet acquisition.");
                    break;
                }
            }

            break; // case recv_data
        }

        case transfer_complete:
        {
            if (recv_bytes == file_size)
            {
                // Complete
                eprintf(NOTIFY "File transfer complete with %ld/%ld bytes of %s having been successfully received and confirmed per packet.", recv_bytes, file_size, filename);
            }
            else if (*done_recv)
            {
                // Interrupted
                eprintf(WARN "File transfer interrupted with %ld/%ld bytes of %s having been successfully received and confirmed per packet.", recv_bytes, file_size, filename);
            }
            else if (recv_bytes <= 0)
            {
                // Error
                eprintf(ERR "An error has been encountered with %ld/%ld bytes of %s sent.", recv_bytes, file_size, filename);

                return recv_bytes;
            }
            else
            {
                /// NOTE: Will reach this case if (recv_bytes != file_size).
                // ???
                eprintf(ERR "Confused.");

                return -1;
            }

            mode = confirmation;

            break; // transfer_complete
        }

        case confirmation:
        {

            eprintf(NOTIFY "~ Entering FILE CONFIRMATION phase. ~");

            // Map a CONF header to the write buffer.
            sw_upd_conf_t *cf_hdr = (sw_upd_conf_t *)rd_buf;

            // Map a CONF reply to the read buffer.
            sw_upd_conf_reply_t *cf_rep = (sw_upd_conf_reply_t *)wr_buf;

            // Look for reception of a CONF packet consisting of the original file's hash. Calculate our own hash. If the hashes do not agree, respond with a CONF reply and devolve to START/RESUME primer intake (and if we had a CRC issue, make sure the request_packet field is some value greater than -1). Otherwise, if the hashes agree, everything should be all set, and we should move the binary to wherever its final destination is.

            eprintf(NOTIFY "Waiting for CONF header...");

            // Wait for CONF header.
            memset(rd_buf, 0x0, PACKET_SIZE);
            retval = uhf_read(fd, rd_buf, PACKET_SIZE);

            // Check if a repeat request is necessary.
            if (retval < 0)
            {
                eprintf(WARN "CONF header reading failed with value %d.", retval);

                // Ask for a repeat.
                retval = uhf_write(fd, rept_cmd, sizeof(rept_cmd));

                if (retval <= 0)
                {
                    eprintf(ERR "REPT command writing failed with value %d.", retval);
                }
                else
                {
                    eprintf(NOTIFY "REPT command written successfully (%d).", retval);
                }

                continue;
            }
            else if (retval == 0)
            {
                eprintf(WARN "CONF packet reading timed out with value %d.", retval);
                // continue;
                return 0;
            }

#ifdef DEBUG_MODE_ACTIVE_SH

            if (fail_times_cf > 0)
            {
                eprintf(WARN "Flipping byte %d from 0x%02hhx to 0x%02hhx.", flip_byte_cf, rd_buf[flip_byte_cf], ~rd_buf[flip_byte_cf]);
                rd_buf[flip_byte_cf] = ~rd_buf[flip_byte_cf];

                fail_times_cf--;

                eprintf(WARN "Error induced. Press Enter to continue...");
                getchar();
            }

#endif // DEBUG_MODE_ACTIVE_SH

            // Check if we read in a repeat command.
            if (!memcmp(rept_cmd, rd_buf, 5))
            { // We read in a REPT command, so, repeat last.
                eprintf(WARN "Repeat of previously sent transmission requested. Sending a duplicate.");
                retval = uhf_write(fd, wr_buf, PACKET_SIZE);

                if (retval <= 0)
                {
                    eprintf(ERR "Repeated message writing failed with value %d.", retval);
                }
                else
                {
                    eprintf(NOTIFY "Repeated message written successfully (%d).", retval);
                }

                continue;
            }

            // If we get here we no longer need the content in wr_buf.
            memset(wr_buf, 0x0, PACKET_SIZE);

            // Print the CONF header.
            print_cf_hdr(cf_hdr);
            memprintl_hex(rd_buf, PACKET_SIZE);
            memprintl_char(rd_buf, PACKET_SIZE);

            // Set CONF reply values.
            cf_rep->cmd = SW_UPD_CFID;
            cf_rep->request_packet = REQ_PKT_RESEND; // May be changed in the Checks. Assume the CONF requires a re-send until confirmed otherwise.
            cf_rep->total_packets = max_packets;
            checksum_md5(filename, cf_rep->hash, 32);

            if (cf_hdr->cmd != SW_UPD_CFID)
            {
                if (cf_hdr->cmd == SW_UPD_SRID)
                {
                    eprintf(WARN "The CMD value 0x%02x matches a START/RESUME CMD, not a CONF CMD as was expected. Sending CONF reply, and switching to START/RESUME mode.", cf_hdr->cmd);
                    mode = primer;
                }
                else if (cf_hdr->cmd == SW_UPD_DTID)
                {
                    eprintf(WARN "The CMD value 0x%02x matches a DATA CMD, not a CONF CMD as was expected. Sending CONF reply, and switching to DATA mode.", cf_hdr->cmd);
                    mode = data;
                }
                else
                {
                    eprintf(WARN "The CMD value 0x%02x matches no known CMDs. Sending CONF reply, will await a legitimate CMD.", cf_hdr->cmd);
                }
            }
            else if (cf_hdr->total_packets != max_packets)
            {
                eprintf(WARN "The Ground Station has calculated the file's maximum packet value to %d, as opposed to SPACE-HAUC's %d. Requesting a re-send of CONF header.", cf_hdr->total_packets, max_packets);
            }
            else if (cf_hdr->packet_number != recv_packets)
            {
                eprintf(WARN "The Ground Station claims the current packet number is %d, while SPACE-HAUC claims it is %d. Requesting a re-send of CONF header.", cf_hdr->packet_number, recv_packets);
            }
            else if (memcmp(cf_hdr->hash, cf_rep->hash, HASH_SIZE) != 0)
            {
                eprintf(WARN "SPACE-HAUC has calculated a hash value for %s which differs from the Ground Station's.", filename);
                eprintlf(WARN "SPACE-HAUC: ");
                memprint_hex(cf_rep->hash, HASH_SIZE);
                eprintlf(WARN "Ground Station: ");
                memprint_hex(cf_rep->hash, HASH_SIZE);
                eprintf(WARN "Requesting a restart of the file transfer.");
                recv_packets = 0;
                recv_bytes = 0;
                sw_sh_set_recv_bytes(filename, 0);
                cf_rep->request_packet = 0;
                mode = primer;
            }
            else
            {
                eprintf(NOTIFY "Received affirmative file confirmation. Successfully confirmed %ld/%ld bytes of %s.", recv_bytes, file_size, filename);
                cf_rep->request_packet = REQ_PKT_AFFIRM; // Success
                mode = finish;
            }

            // CONF reply values set prior to Checks.
            if (recv_packets < max_packets)
            {
                eprintf(WARN "During binary writing, only %ld of %d packets were written to the file. Requesting a re-send of the entire file from this packet onward.", recv_packets, max_packets);
                cf_rep->request_packet = recv_packets;
            }

            eprintf(NOTIFY "Sending CONF reply...");

            // Print CONF reply.
            print_cf_rep(cf_rep);
            memprintl_hex(wr_buf, PACKET_SIZE);
            memprintl_char(wr_buf, PACKET_SIZE);

            // Send CONF reply.
            retval = uhf_write(fd, wr_buf, PACKET_SIZE);

            if (retval <= 0)
            {
                eprintf(ERR "CONF reply writing failed with value %d.", retval);
                continue;
            }

            break; // case confirmation
        }

        case finish:
        {

            eprintf(NOTIFY "Currently, the file will remain in the home directory. This is normal.");
            eprintf(NOTIFY "DON'T PANIC!");

            return 1;
        }
        }
    }

    return -1;
}

ssize_t sw_sh_write_to_file(char *data, ssize_t data_size, char filename[], int byte_location)
{
    // This function should write data to the file of name filename.

    FILE *bin_fp = NULL;
    ssize_t writ_bytes = 0;

    // If we are beginning from byte 0, there is no reason to not overwrite the whole file. This ensures that if we have mangled the file, eventually it will be overwritten and fixed.
    if (access(filename, F_OK) != 0 || byte_location == 0)
    {
        // File does not exist.
        bin_fp = fopen(filename, "wb");
        if (bin_fp == NULL)
        {
            eprintf(ERR "Couldn't open %s.", filename);
            return writ_bytes;
        }
        writ_bytes = fwrite(data, 1, data_size, bin_fp);
        fclose(bin_fp);
    }
    else if (access(filename, F_OK) == 0)
    {
        // File exists.
        bin_fp = fopen(filename, "ab");
        if (bin_fp == NULL)
        {
            eprintf(ERR "Couldn't open %s.", filename);
            return writ_bytes;
        }
        fseek(bin_fp, byte_location, SEEK_SET);
        writ_bytes = fwrite(data, 1, data_size, bin_fp);
        fclose(bin_fp);
    }

    return writ_bytes;
}

ssize_t sw_sh_get_recv_bytes(const char filename[])
{
    // Read from {filename}.shbytes to see if this file was already mid-transfer and needs to continue at some specific point.
    char filename_bytes[128];
    snprintf(filename_bytes, 128, "%s.%s", filename, "shbytes");

    FILE *bytes_fp = NULL;
    ssize_t recv_bytes = 0;

    if (access(filename_bytes, F_OK) == 0)
    {
        // File exists.
        bytes_fp = fopen(filename_bytes, "rb");
        if (bytes_fp == NULL)
        {
            eprintf(ERR "%s exists but could not be opened.", filename_bytes);
            return -1;
        }
        if (fread(&recv_bytes, sizeof(ssize_t), 1, bytes_fp) != 1)
        {
            eprintf(ERR "Error reading recv_bytes.");
        }
        eprintf(NOTIFY "%ld bytes of current transfer previously received.", recv_bytes);
        fclose(bytes_fp);
    }
    else
    {
        // File does not exist.
        bytes_fp = fopen(filename_bytes, "wb");
        if (bytes_fp == NULL)
        {
            eprintf(ERR "%s does not exist and could not be created.", filename_bytes);
            return -1;
        }
        if (fwrite(&recv_bytes, sizeof(ssize_t), 1, bytes_fp) != 1)
        {
            eprintf(ERR "Error.");
        }
        eprintf(NOTIFY "%s does not exist. Assuming transfer should start at packet 0.", filename_bytes);
        fclose(bytes_fp);
    }

    return recv_bytes;
}

int sw_sh_set_recv_bytes(const char filename[], ssize_t recv_bytes)
{
    // Overwrite {filename}.shbytes to contain {recv_bytes}.
    char filename_bytes[128];
    snprintf(filename_bytes, 128, "%s.%s", filename, "shbytes");

    FILE *bytes_fp = NULL;
    bytes_fp = fopen(filename_bytes, "wb");
    if (bytes_fp == NULL)
    {
        eprintf(ERR "Could not open %s for overwriting.", filename_bytes);
        return -1;
    }
    fwrite(&recv_bytes, sizeof(ssize_t), 1, bytes_fp);
    fclose(bytes_fp);

#ifdef DEBUG_MODE_ACTIVE_SH
    eprintf("Set .shbytes to %d.", recv_bytes);
#endif // DEBUG_MODE_ACTIVE_SH

    return 1;
}

void sw_sh_destroy(uhf_modem_t fd)
{
    if (fd < 3)
    {
        eprintf(ERR "%d not valid file descriptor", fd);
    }
    else
    {
        close(fd);
    }
}