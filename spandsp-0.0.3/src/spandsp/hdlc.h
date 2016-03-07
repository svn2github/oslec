/*
 * SpanDSP - a series of DSP components for telephony
 *
 * hdlc.h
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
 * $Id: hdlc.h,v 1.27 2006/10/24 13:45:28 steveu Exp $
 */

/*! \file */

/*! \page hdlc_page HDLC

\section hdlc_page_sec_1 What does it do?
The HDLC module provides bit stuffing, destuffing, framing and deframing,
according to the HDLC protocol. It also provides 16 and 32 bit CRC generation
and checking services for HDLC frames.

HDLC may not be a DSP function, but is needed to accompany several DSP components.

\section hdlc_page_sec_2 How does it work?
*/


#if !defined(_HDLC_H_)
#define _HDLC_H_

/*! 
    HDLC_MAXFRAME_LEN is the maximum length of a stuffed HDLC frame, excluding the CRC.
*/
#define HDLC_MAXFRAME_LEN       400	

typedef void (*hdlc_frame_handler_t)(void *user_data, int ok, const uint8_t *pkt, int len);
typedef void (*hdlc_underflow_handler_t)(void *user_data);

/*!
    HDLC receive descriptor. This contains all the state information for an HDLC receiver.
 */
typedef struct
{
    /*! 2 for CRC-16, 4 for CRC-32 */
    int crc_bytes;
    /*! \brief The callback routine called to process each good received frame. */
    hdlc_frame_handler_t frame_handler;
    /*! \brief An opaque parameter passed to the callback routine. */
    void *user_data;
    /*! \brief TRUE if bad frames are to be reported. */
    int report_bad_frames;
    /*! \brief The number of consecutive flags which must be seen before framing is
        declared OK. */
    int framing_ok_threshold;
    /*! \brief TRUE if framing OK has been announced. */
    int framing_ok_announced;
    /*! \brief Number of consecutive flags seen so far. */
    int flags_seen;

    /*! \brief The raw (stuffed) bit stream buffer. */
    unsigned int raw_bit_stream;
    /*! \brief The destuffed bit stream buffer. */
    unsigned int byte_in_progress;
    /*! \brief The current number of bits in byte_in_progress. */
    int num_bits;
	
    /*! \brief Buffer for a frame in progress. */
    uint8_t buffer[HDLC_MAXFRAME_LEN + 4];
    /*! \brief Length of a frame in progress. */
    int len;

    /*! \brief The number of bytes of good frames received (CRC not included). */
    unsigned long int rx_bytes;
    /*! \brief The number of good frames received. */
    unsigned long int rx_frames;
    /*! \brief The number of frames with CRC errors received. */
    unsigned long int rx_crc_errors;
    /*! \brief The number of too short and too long frames received. */
    unsigned long int rx_length_errors;
    /*! \brief The number of HDLC aborts received. */
    unsigned long int rx_aborts;
} hdlc_rx_state_t;

/*!
    HDLC received data statistics.
 */
typedef struct
{
    /*! \brief The number of bytes of good frames received (CRC not included). */
    unsigned long int bytes;
    /*! \brief The number of good frames received. */
    unsigned long int good_frames;
    /*! \brief The number of frames with CRC errors received. */
    unsigned long int crc_errors;
    /*! \brief The number of too short and too long frames received. */
    unsigned long int length_errors;
    /*! \brief The number of HDLC aborts received. */
    unsigned long int aborts;
} hdlc_rx_stats_t;

/*!
    HDLC transmit descriptor. This contains all the state information for an
    HDLC transmitter.
 */
typedef struct
{
    /*! 2 for CRC-16, 4 for CRC-32 */
    int crc_bytes;
    /*! \brief The callback routine called to indicate transmit underflow. */
    hdlc_underflow_handler_t underflow_handler;
    /*! \brief An opaque parameter passed to the callback routine. */
    void *user_data;
    /*! \brief The minimum flag octets to insert between frames. */
    int inter_frame_flags;
    /*! \brief TRUE if frame creation works in progressive mode. */
    int progressive;
    /*! \brief Maximum permitted frame length. */
    int max_frame_len;

    /*! \brief The stuffed bit stream being created. */
    uint32_t octets_in_progress;
    /*! \brief The number of bits currently in octets_in_progress. */
    int num_bits;
    /*! \brief The currently rotated state of the flag octet. */
    int idle_octet;
    /*! \brief The number of flag octets to send for a timed burst of flags. */
    int flag_octets;
    /*! \brief TRUE if the next underflow of timed flag octets should be reported */
    int report_flag_underflow;

    /*! \brief The current message being transmitted, with its CRC attached. */
    uint8_t buffer[HDLC_MAXFRAME_LEN + 4];
    /*! \brief The length of the message in the buffer. */
    int len;
    /*! \brief The current send position within the buffer. */
    int pos;
    /*! \brief The running CRC, as data fills the frame buffer. */
    uint32_t crc;

    /*! \brief The current byte being broken into bits for transmission. */
    int byte;
    /*! \brief The number of bits remaining in byte. */
    int bits;
    
    /*! \brief TRUE if transmission should end on buffer underflow .*/
    int tx_end;
} hdlc_tx_state_t;

#ifdef __cplusplus
extern "C" {
#endif

/*! \brief Calculate the ITU/CCITT CRC-32 value in buffer.
    \param buf The buffer containing the data.
    \param len The length of the frame.
    \param crc The initial CRC value. This is usually 0xFFFFFFFF, or 0 for a new block (it depends on
           the application). It is previous returned CRC value for the continuation of a block.
    \return The CRC value.
*/
uint32_t crc_itu32_calc(const uint8_t *buf, int len, uint32_t crc);

/*! \brief Append an ITU/CCITT CRC-32 value to a frame.
    \param buf The buffer containing the frame. This must be at least 2 bytes longer than
               the frame it contains, to allow room for the CRC value.
    \param len The length of the frame.
    \return The new length of the frame.
*/
int crc_itu32_append(uint8_t *buf, int len);

/*! \brief Check the ITU/CCITT CRC-32 value in a frame.
    \param buf The buffer containing the frame.
    \param len The length of the frame.
    \return TRUE if the CRC is OK, else FALSE.
*/
int crc_itu32_check(const uint8_t *buf, int len);

/*! \brief Calculate the ITU/CCITT CRC-16 value in buffer.
    \param buf The buffer containing the data.
    \param len The length of the frame.
    \param crc The initial CRC value. This is usually 0xFFFF, or 0 for a new block (it depends on
           the application). It is previous returned CRC value for the continuation of a block.
    \return The CRC value.
*/
uint16_t crc_itu16_calc(const uint8_t *buf, int len, uint16_t crc);

/*! \brief Append an ITU/CCITT CRC-16 value to a frame.
    \param buf The buffer containing the frame. This must be at least 2 bytes longer than
               the frame it contains, to allow room for the CRC value.
    \param len The length of the frame.
    \return The new length of the frame.
*/
int crc_itu16_append(uint8_t *buf, int len);

/*! \brief Check the ITU/CCITT CRC-16 value in a frame.
    \param buf The buffer containing the frame.
    \param len The length of the frame.
    \return TRUE if the CRC is OK, else FALSE.
*/
int crc_itu16_check(const uint8_t *buf, int len);

/*! \brief Initialise an HDLC receiver context.
    \param s A pointer to an HDLC receiver context.
    \param crc32 TRUE to use ITU CRC32. FALSE to use ITU CRC16.
    \param report_bad_frames TRUE to request the reporting of bad frames.
    \param framing_ok_threshold The number of back-to-back flags needed to
           start the framing OK condition. This may be used where a series of
           flag octets is used as a preamble, such as in the T.30 protocol.
    \param handler The function to be called when a good HDLC frame is received.
    \param user_data An opaque parameter for the callback routine.
    \return A pointer to the HDLC receiver context.
*/
hdlc_rx_state_t *hdlc_rx_init(hdlc_rx_state_t *s,
                              int crc32,
                              int report_bad_frames,
                              int framing_ok_threshold,
                              hdlc_frame_handler_t handler,
                              void *user_data);

/*! \brief Get the current receive statistics.
    \param s A pointer to an HDLC receiver context.
    \param t A pointer to the buffer for the statistics.
    \return 0 for OK, else -1.
*/
int hdlc_rx_get_stats(hdlc_rx_state_t *s,
                      hdlc_rx_stats_t *t);

/* Use either the bit-by-bit or byte-by-byte routines. Do not mix them is a
   single instance of HDLC */
void hdlc_rx_put_bit(hdlc_rx_state_t *s, int new_bit);
void hdlc_rx_put_byte(hdlc_rx_state_t *s, int new_byte);

/*! \brief Initialise an HDLC transmitter context.
    \param s A pointer to an HDLC transmitter context.
    \param crc32 TRUE to use ITU CRC32. FALSE to use ITU CRC16.
    \param inter_frame_flags The minimum flag octets to insert between frames (usually one).
    \param progressive TRUE if frame creation works in progressive mode.
    \param handler The callback function called when the HDLC transmitter underflows.
    \param user_data An opaque parameter for the callback routine.
    \return A pointer to the HDLC transmitter context.
*/
hdlc_tx_state_t *hdlc_tx_init(hdlc_tx_state_t *s,
                              int crc32,
                              int inter_frame_flags,
                              int progressive,
                              hdlc_underflow_handler_t handler,
                              void *user_data);

/*! \brief Set the maximum frame length for an HDLC transmitter context.
    \param s A pointer to an HDLC transmitter context.
    \param max_len The maximum length.
*/
void hdlc_tx_set_max_frame_len(hdlc_tx_state_t *s, int max_len);

/*! \brief Transmit a frame.
    \param s A pointer to an HDLC transmitter context.
    \param frame A pointer to the frame to be transmitted.
    \param len The length of the frame to be transmitted.
    \return 0 if the frame was accepted for transmission, else -1.
*/
int hdlc_tx_frame(hdlc_tx_state_t *s, const uint8_t *frame, int len);

/*! \brief Corrupt the frame currently being transmitted, by giving it the wrong CRC.
    \param s A pointer to an HDLC transmitter context.
    \return 0 if the frame was corrupted, else -1.
*/
int hdlc_tx_corrupt_frame(hdlc_tx_state_t *s);

/*! \brief Transmit a specified quantity of flag octets as a preamble.
    \param s A pointer to an HDLC transmitter context.
    \param len The length of the required preamble, in flag octets. If len is zero this
           requests that HDLC transmission be terminated when the buffers have fully
           drained.
    \return 0 if the preamble was accepted for transmission, else -1.
*/
int hdlc_tx_preamble(hdlc_tx_state_t *s, int len);

/*! \brief Get the next bit for transmission.
    \param s A pointer to an HDLC transmitter context.
    \return The next bit for transmission.
*/
int hdlc_tx_get_bit(hdlc_tx_state_t *s);

/*! \brief Get the next byte for transmission.
    \param s A pointer to an HDLC transmitter context.
    \return The next byte for transmission.
*/
int hdlc_tx_get_byte(hdlc_tx_state_t *s);

#ifdef __cplusplus
}
#endif

#endif
/*- End of file ------------------------------------------------------------*/
