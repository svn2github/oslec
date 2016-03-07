/*
 * SpanDSP - a series of DSP components for telephony
 *
 * t4.h - definitions for T.4 fax processing
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
 * $Id: t4.h,v 1.29 2006/10/24 13:45:28 steveu Exp $
 */

/*! \file */

#if !defined(_T4_H_)
#define _T4_H_

/*! \page t4_page T.4 image compression and decompression

\section t4_page_sec_1 What does it do?
The T.4 image compression and decompression routines implement the 1D and 2D
encoding methods defined in ITU specification T.4. They also implement the pure
2D encoding method defined in T.6. These are image compression algorithms used
for FAX transmission.

\section t4_page_sec_1 How does it work?
*/

#define T4_COMPRESSION_ITU_T4_1D    1
#define T4_COMPRESSION_ITU_T4_2D    2
#define T4_COMPRESSION_ITU_T6       3

#define T4_X_RESOLUTION_R4          4019
#define T4_X_RESOLUTION_R8          8037
#define T4_X_RESOLUTION_R16         16074

#define T4_Y_RESOLUTION_STANDARD    3850
#define T4_Y_RESOLUTION_FINE        7700
#define T4_Y_RESOLUTION_SUPERFINE   15400

/*!
    T.4 FAX compression/decompression descriptor. This defines the working state
    for a single instance of a T.4 FAX compression or decompression channel.
*/
typedef struct
{
    /* "Background" information about the FAX, which can be stored in a TIFF file. */
    /*! \brief The vendor of the machine which produced the TIFF file. */ 
    const char      *vendor;
    /*! \brief The model of machine which produced the TIFF file. */ 
    const char      *model;
    /*! \brief The local ident string. */ 
    const char      *local_ident;
    /*! \brief The remote end's ident string. */ 
    const char      *far_ident;
    /*! \brief The FAX sub-address. */ 
    const char      *sub_address;
    /*! \brief The text which will be used in FAX page header. No text results
               in no header line. */
    const char      *header_info;

    /*! \brief The type of compression used between the FAX machines. */
    int             line_encoding;
    /*! \brief The minimum number of bits per scan row. This is a timing thing
               for hardware FAX machines. */
    int             min_scan_line_bits;
    
    int             output_compression;
    int             output_t4_options;

    time_t          page_start_time;

    int             bytes_per_row;
    int             image_size;
    int             image_buffer_size;
    uint8_t         *image_buffer;

    TIFF            *tiff_file;
    const char      *file;
    int             start_page;
    int             stop_page;

    int             pages_transferred;
    /*! Column-to-column (X) resolution in pixels per metre. */
    int             x_resolution;
    /*! Row-to-row (Y) resolution in pixels per metre. */
    int             y_resolution;
    /*! Width of the current page, in pixels. */
    int             image_width;
    /*! Current pixel row number. */
    int             row;
    /*! Total pixel rows in the current page. */
    int             image_length;
    /*! The current number of consecutive bad rows. */
    int             curr_bad_row_run;
    /*! The longest run of consecutive bad rows seen in the current page. */
    int             longest_bad_row_run;
    /*! The total number of bad rows in the current page. */
    int             bad_rows;

    /* Decode state */
    uint32_t        bits_to_date;
    int             bits;

    /*! \brief This variable is set if we are treating the current row as a 2D encoded
               one. */
    int             row_is_2d;
    int             its_black;
    int             row_len;
    /*! \brief This variable is used to record the fact we have seen at least one EOL
               since we started decoding. We will not try to interpret the received
               data as an image until we have seen the first EOL. */
    int             first_eol_seen;
    /*! \brief This variable is used to count the consecutive EOLS we have seen. If it
               reaches six, this is the end of the image. */
    int             consecutive_eols;

    /*! \brief B&W runs for reference line */
    uint32_t        *ref_runs;
    /*! \brief B&W runs for current line */
    uint32_t        *cur_runs;

    uint32_t        *pa;
    uint32_t        *pb;
    int             a0;
    int             b1;
    /*! \brief The length of the current run of black or white. */
    int             run_length;
    int             black_white;

    uint32_t        data;
    int             bit;

    /*! \brief A point into the image buffer indicating where the last row begins */
    int             last_row_starts_at;
    /*! \brief A point into the image buffer indicating where the current row begins */
    int             row_starts_at;
    
    /* Encode state */

    /*! Pointer to the buffer for the current pixel row. */
    uint8_t         *row_buf;
    
    int             bit_pos;
    int             bit_ptr;

    /*! \brief The reference pixel row for 2D encoding. */
    uint8_t         *ref_row_buf;
    /*! \brief The maximum contiguous rows that will be 2D encoded. */
    int             max_rows_to_next_1d_row;
    /*! \brief Number of rows left that can be 2D encoded, before a 1D encoded row
               must be used. */
    int             rows_to_next_1d_row;
    /*! \brief The minimum number of encoded bits per row. */
    int             min_row_bits;
    /*! \brief The current number of bits in the current encoded row. */
    int             row_bits;

    /*! \brief Error and flow logging control */
    logging_state_t logging;
} t4_state_t;

/*!
    T.4 FAX compression/decompression statistics.
*/
typedef struct
{
    /*! \brief The number of pages transferred so far. */
    int pages_transferred;
    /*! \brief The number of horizontal pixels in the most recent page. */
    int width;
    /*! \brief The number of vertical pixels in the most recent page. */
    int length;
    /*! \brief The number of bad pixel rows in the most recent page. */
    int bad_rows;
    /*! \brief The largest number of bad pixel rows in a block in the most recent page. */
    int longest_bad_row_run;
    /*! \brief The horizontal resolution of the page in pixels per metre */
    int x_resolution;
    /*! \brief The vertical resolution of the page in pixels per metre */
    int y_resolution;
    /*! \brief The type of compression used between the FAX machines */
    int encoding;
    /*! \brief The size of the image, in bytes */
    int image_size;
} t4_stats_t;
    
#ifdef __cplusplus
extern "C" {
#endif

/*! \brief Allocate a T.4 transmit handling context, and
           initialise it.
    \param file The name of the file to be received.
    \param output_encoding The output encoding.
    \return The T.4 context, or NULL. */
t4_state_t *t4_rx_create(const char *file, int output_encoding);

/*! \brief Prepare for reception of a document.
    \param s The T.4 context.
    \param file The name of the file to be received.
    \param output_encoding The output encoding.
    \return 0 for success, otherwise -1. */
int t4_rx_init(t4_state_t *s, const char *file, int output_encoding);

/*! \brief Prepare to receive the next page of the current document.
    \param s The T.4 context.
    \return zero for success, -1 for failure. */
int t4_rx_start_page(t4_state_t *s);

/*! \brief Put a bit of the current document page.
    \param s The T.4 context.
    \param bit The data bit.
    \return TRUE when the bit ends the document page, otherwise FALSE. */
int t4_rx_put_bit(t4_state_t *s, int bit);

/*! \brief Complete the reception of a page.
    \param s The T.4 receive context.
    \return 0 for success, otherwise -1. */
int t4_rx_end_page(t4_state_t *s);

/*! \brief End reception of a document. Tidy up, close the file and
           free the context. This should be used to end T.4 reception
           started with t4_rx_create.
    \param s The T.4 receive context.
    \return 0 for success, otherwise -1. */
int t4_rx_delete(t4_state_t *s);

/*! \brief End reception of a document. Tidy up and close the file.
           This should be used to end T.4 reception started with
           t4_rx_init.
    \param s The T.4 context.
    \return 0 for success, otherwise -1. */
int t4_rx_end(t4_state_t *s);

/*! \brief Set the encoding for the received data.
    \param s The T.4 context.
    \param encoding The encoding. */
void t4_rx_set_rx_encoding(t4_state_t *s, int encoding);

/*! \brief Set the expected width of the received image, in pixel columns.
    \param s The T.4 context.
    \param columns The number of pixels across the image. */
void t4_rx_set_image_width(t4_state_t *s, int width);

/*! \brief Set the row-to-row (y) resolution to expect for a received image.
    \param s The T.4 context.
    \param resolution The resolution, in pixels per metre. */
void t4_rx_set_y_resolution(t4_state_t *s, int resolution);

/*! \brief Set the column-to-column (x) resolution to expect for a received image.
    \param s The T.4 context.
    \param resolution The resolution, in pixels per metre. */
void t4_rx_set_x_resolution(t4_state_t *s, int resolution);

/*! \brief Set the sub-address of the fax, for inclusion in the file.
    \param s The T.4 context.
    \param sub_address The sub-address string. */
void t4_rx_set_sub_address(t4_state_t *s, const char *sub_address);

/*! \brief Set the identity of the remote machine, for inclusion in the file.
    \param s The T.4 context.
    \param ident The identity string. */
void t4_rx_set_far_ident(t4_state_t *s, const char *ident);

/*! \brief Set the vendor of the remote machine, for inclusion in the file.
    \param s The T.4 context.
    \param vendor The vendor string, or NULL. */
void t4_rx_set_vendor(t4_state_t *s, const char *vendor);

/*! \brief Set the model of the remote machine, for inclusion in the file.
    \param s The T.4 context.
    \param model The model string, or NULL. */
void t4_rx_set_model(t4_state_t *s, const char *model);

/*! \brief Allocate a T.4 receive handling context, and
           initialise it.
    \param s The T.4 context.
    \param file The name of the file to be sent.
    \return 0 for success, otherwise -1. */
t4_state_t *t4_tx_create(const char *file, int start_page, int stop_page);

/*! \brief Prepare for transmission of a document.
    \param s The T.4 context.
    \param file The name of the file to be sent.
    \param start_page The first page to send. -1 for no restriction.
    \param stop_page The last page to send. -1 for no restriction.
    \return The T.4 context, or NULL. */
int t4_tx_init(t4_state_t *s, const char *file, int start_page, int stop_page);

/*! \brief Prepare to send the next page of the current document.
    \param s The T.4 context.
    \return zero for success, -1 for failure. */
int t4_tx_start_page(t4_state_t *s);

/*! \brief Prepare the current page for a resend.
    \param s The T.4 context.
    \return zero for success, -1 for failure. */
int t4_tx_restart_page(t4_state_t *s);

/*! \brief Check for the existance of the next page. This information can
    be needed before it is determined that the current page is finished with.
    \param s The T.4 context.
    \return zero for next page found, -1 for failure. */
int t4_tx_more_pages(t4_state_t *s);

/*! \brief Complete the sending of a page.
    \param s The T.4 context.
    \return zero for success, -1 for failure. */
int t4_tx_end_page(t4_state_t *s);

/*! \brief Get the next bit of the current document page. The document will
           be padded for the current minimum scan line time. If the
           file does not contain an RTC (return to control) code at
           the end of the page, one will be added.
    \param s The T.4 context.
    \return The next bit (i.e. 0 or 1). For the last bit of data, bit 1 is
            set (i.e. the returned value is 2 or 3). */
int t4_tx_get_bit(t4_state_t *s);

/*! \brief Return the next bit of the current document page, without actually
           moving forward in the buffer. The document will be padded for the
           current minimum scan line time. If the file does not contain an
           RTC (return to control) code at the end of the page, one will be
           added.
    \param s The T.4 context.
    \return The next bit (i.e. 0 or 1). For the last bit of data, bit 1 is
            set (i.e. the returned value is 2 or 3). */
int t4_tx_check_bit(t4_state_t *s);

/*! \brief End the transmission of a document. Tidy up, close the file and
           free the context. This should be used to end T.4 transmission
           started with t4_tx_create.
    \param s The T.4 context.
    \return 0 for success, otherwise -1. */
int t4_tx_delete(t4_state_t *s);

/*! \brief End the transmission of a document. Tidy up and close the file.
           This should be used to end T.4 transmission started with t4_tx_init.
    \param s The T.4 context.
    \return 0 for success, otherwise -1. */
int t4_tx_end(t4_state_t *s);

/*! \brief Set the encoding for the encoded data.
    \param s The T.4 context.
    \param encoding The encoding. */
void t4_tx_set_tx_encoding(t4_state_t *s, int encoding);

/*! \brief Set the minimum number of encoded bits per row. This allows the
           makes the encoding process to be set to comply with the minimum row
           time specified by a remote receiving machine.
    \param s The T.4 context.
    \param bits The minimum number of bits per row. */
void t4_tx_set_min_row_bits(t4_state_t *s, int bits);

/*! \brief Set the identity of the local machine, for inclusion in page headers.
    \param s The T.4 context.
    \param ident The identity string. */
void t4_tx_set_local_ident(t4_state_t *s, const char *ident);

/*! Set the info field, included in the header line included in each page of an encoded
    FAX. This is a string of up to 50 characters. Other information (date, local ident, etc.)
    are automatically included in the header. If the header info is set to NULL or a zero
    length string, no header lines will be added to the encoded FAX.
    \brief Set the header info.
    \param s The T.4 context.
    \param info A string, of up to 50 bytes, which will form the info field. */
void t4_tx_set_header_info(t4_state_t *s, const char *info);

/*! \brief Get the row-to-row (y) resolution of the current page.
    \param s The T.4 context.
    \return The resolution, in pixels per metre. */
int t4_tx_get_y_resolution(t4_state_t *s);

/*! \brief Get the column-to-column (x) resolution of the current page.
    \param s The T.4 context.
    \return The resolution, in pixels per metre. */
int t4_tx_get_x_resolution(t4_state_t *s);

/*! \brief Get the width of the current page, in pixel columns.
    \param s The T.4 context.
    \return The number of columns. */
int t4_tx_get_image_width(t4_state_t *s);

/*! Get the current image transfer statistics. 
    \brief Get the current transfer statistics.
    \param s The T.4 context.
    \param t A pointer to a statistics structure. */
void t4_get_transfer_statistics(t4_state_t *s, t4_stats_t *t);

/*! Get the short text name of an encoding format. 
    \brief Get the short text name of an encoding format.
    \param encoding The encoding type.
    \return A pointer to the string. */
const char *t4_encoding_to_str(int encoding);

#ifdef __cplusplus
}
#endif

#endif
/*- End of file ------------------------------------------------------------*/
