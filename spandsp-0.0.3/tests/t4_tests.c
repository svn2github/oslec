/*
 * SpanDSP - a series of DSP components for telephony
 *
 * t4_tests.c - ITU T.4 FAX image to and from TIFF file tests
 *
 * Written by Steve Underwood <steveu@coppice.org>
 *
 * Copyright (C) 2003 Steve Underwood
 *
 * All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2, as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * $Id: t4_tests.c,v 1.34 2006/11/23 15:48:09 steveu Exp $
 */

/*! \file */

/*! \page t4_tests_page T.4 tests
\section t4_tests_page_sec_1 What does it do
These tests exercise the image compression and decompression methods defined
in ITU specifications T.4 and T.6.
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <fcntl.h>
#include <memory.h>
#if defined(HAVE_TGMATH_H)
#include <tgmath.h>
#endif
#if defined(HAVE_MATH_H)
#include <math.h>
#endif

#include <tiffio.h>

#include "spandsp.h"

#define IN_FILE_NAME    "../itutests/fax/itutests.tif"
#define OUT_FILE_NAME   "t4_tests_receive.tif"

#define XSIZE           1728

t4_state_t send_state;
t4_state_t receive_state;

int main(int argc, char* argv[])
{
    int sends;
    int page_no;
    int bit;
    int end_of_page;
    int end_marks;
    int i;
    int decode_test;
    int compression;
    int compression_step;
    int add_page_headers;
    int min_row_bits;
    int restart_pages;
    char buf[512];
    t4_stats_t stats;
    const char *in_file_name;

    i = 1;
    decode_test = FALSE;
    compression = -1;
    add_page_headers = FALSE;
    restart_pages = FALSE;
    in_file_name = IN_FILE_NAME;
    min_row_bits = 0;
    while (i < argc)
    {
        if (strcmp(argv[i], "-d") == 0)
            decode_test = TRUE;
        else if (strcmp(argv[i], "-1") == 0)
            compression = T4_COMPRESSION_ITU_T4_1D;
        else if (strcmp(argv[i], "-2") == 0)
            compression = T4_COMPRESSION_ITU_T4_2D;
        else if (strcmp(argv[i], "-6") == 0)
            compression = T4_COMPRESSION_ITU_T6;
        else if (strcmp(argv[i], "-h") == 0)
            add_page_headers = TRUE;
        else if (strcmp(argv[i], "-r") == 0)
            restart_pages = TRUE;
        else if (strcmp(argv[i], "-i") == 0)
            in_file_name = argv[++i];
        else if (strcmp(argv[i], "-m") == 0)
            min_row_bits = atoi(argv[++i]);
        i++;
    }
    /* Create a send and a receive end */
    memset(&send_state, 0, sizeof(send_state));
    memset(&receive_state, 0, sizeof(receive_state));

    if (decode_test)
    {
        if (compression < 0)
            compression = T4_COMPRESSION_ITU_T4_1D;
        /* Receive end puts TIFF to a new file. We assume the receive width here. */
        if (t4_rx_init(&receive_state, OUT_FILE_NAME, T4_COMPRESSION_ITU_T4_2D))
        {
            printf("Failed to init\n");
            exit(2);
        }
        
        t4_rx_set_rx_encoding(&receive_state, compression);
        t4_rx_set_x_resolution(&receive_state, T4_X_RESOLUTION_R8);
        t4_rx_set_y_resolution(&receive_state, T4_Y_RESOLUTION_FINE);
        t4_rx_set_image_width(&receive_state, XSIZE);

        page_no = 1;
        t4_rx_start_page(&receive_state);
        while (fgets(buf, 511, stdin))
        {
            if (sscanf(buf, "Rx bit %*d - %d", &bit) == 1)
            {
                if ((end_of_page = t4_rx_put_bit(&receive_state, bit)))
                {
                    printf("End of page detected\n");
                    break;
                }
            }
        }
#if 0
        /* Dump the entire image as text 'X's and spaces */
        s = receive_state.image_buffer;
        for (i = 0;  i < receive_state.rows;  i++)
        {
            for (j = 0;  j < receive_state.bytes_per_row;  j++)
            {
                for (k = 0;  k < 8;  k++)
                {
                    printf((receive_state.image_buffer[i*receive_state.bytes_per_row + j] & (0x80 >> k))  ?  "X"  :  " ");
                }
            }
            printf("\n");
        }
#endif
        t4_rx_end_page(&receive_state);
        t4_get_transfer_statistics(&receive_state, &stats);
        printf("Pages = %d\n", stats.pages_transferred);
        printf("Image size = %d x %d\n", stats.width, stats.length);
        printf("Image resolution = %d x %d\n", stats.x_resolution, stats.y_resolution);
        printf("Bad rows = %d\n", stats.bad_rows);
        printf("Longest bad row run = %d\n", stats.longest_bad_row_run);
        t4_rx_end(&receive_state);
    }
    else
    {
        /* Send end gets TIFF from a file */
        if (t4_tx_init(&send_state, in_file_name, -1, -1))
        {
            printf("Failed to init TIFF send\n");
            exit(2);
        }

        /* Receive end puts TIFF to a new file. */
        if (t4_rx_init(&receive_state, OUT_FILE_NAME, T4_COMPRESSION_ITU_T4_2D))
        {
            printf("Failed to init\n");
            exit(2);
        }
    
        t4_tx_set_min_row_bits(&send_state, min_row_bits);

        t4_rx_set_x_resolution(&receive_state, t4_tx_get_x_resolution(&send_state));
        t4_rx_set_y_resolution(&receive_state, t4_tx_get_y_resolution(&send_state));
        t4_rx_set_image_width(&receive_state, t4_tx_get_image_width(&send_state));

        /* Now send and receive all the pages in the source TIFF file */
        page_no = 1;
        t4_tx_set_local_ident(&send_state, "852 2666 0542");
        sends = 0;
        /* Select whether we step round the compression schemes, or use a single specified one. */
        compression_step = (compression < 0)  ?  0  :  -1;
        for (;;)
        {
            end_marks = 0;
            /* Add a header line to alternate pages, if required */
            if (add_page_headers  &&  (sends & 2))
                t4_tx_set_header_info(&send_state, "Header");
            else
                t4_tx_set_header_info(&send_state, NULL);
            if (restart_pages  &&  (sends & 1))
            {
                /* Use restart, to send the page a second time */
                if (t4_tx_restart_page(&send_state))
                    break;
            }
            else
            {
                switch (compression_step)
                {
                case 0:
                    compression = T4_COMPRESSION_ITU_T4_1D;
                    compression_step++;
                    break;
                case 1:
                    compression = T4_COMPRESSION_ITU_T4_2D;
                    compression_step++;
                    break;
                case 2:
                    compression = T4_COMPRESSION_ITU_T6;
                    compression_step = 0;
                    break;
                }
                t4_tx_set_tx_encoding(&send_state, compression);
                t4_rx_set_rx_encoding(&receive_state, compression);

                if (t4_tx_start_page(&send_state))
                    break;
            }
            t4_rx_start_page(&receive_state);
            do
            {
                bit = t4_tx_get_bit(&send_state);
#if 0
                if (--next_hit <= 0)
                {
                    do
                        next_hit = rand() & 0x3FF;
                    while (next_hit < 20);
                    bit ^= (rand() & 1);
                }
#endif
                if (bit == PUTBIT_END_OF_DATA)
                {
                    /* T.6 data does not contain an image termination sequence.
                       T.4 1D and 2D do, and should locate that sequence. */
                    if (compression == T4_COMPRESSION_ITU_T6)
                        break;
                    if (++end_marks > 50)
                    {
                        printf("Receiver missed the end of page mark\n");
                        exit(2);
                    }
                }
                end_of_page = t4_rx_put_bit(&receive_state, bit & 1);
            }
            while (!end_of_page);
            t4_get_transfer_statistics(&receive_state, &stats);
            printf("Pages = %d\n", stats.pages_transferred);
            printf("Image size = %d x %d\n", stats.width, stats.length);
            printf("Image resolution = %d x %d\n", stats.x_resolution, stats.y_resolution);
            printf("Bad rows = %d\n", stats.bad_rows);
            printf("Longest bad row run = %d\n", stats.longest_bad_row_run);
            if (!restart_pages  ||  (sends & 1))
                t4_tx_end_page(&send_state);
            t4_rx_end_page(&receive_state);
            sends++;
        }
        /* And we should now have a matching received TIFF file. Note this will only match
           at the image level. TIFF files allow a lot of ways to express the same thing,
           so bit matching of the files is not the normal case. */
        t4_tx_end(&send_state);
        t4_rx_end(&receive_state);
    }
}
/*- End of function --------------------------------------------------------*/
/*- End of file ------------------------------------------------------------*/
