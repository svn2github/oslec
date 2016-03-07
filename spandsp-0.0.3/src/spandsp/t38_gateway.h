/*
 * SpanDSP - a series of DSP components for telephony
 *
 * t38_gateway.h - An implementation of T.38, less the packet exchange part
 *
 * Written by Steve Underwood <steveu@coppice.org>
 *
 * Copyright (C) 2005 Steve Underwood
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
 * $Id: t38_gateway.h,v 1.17 2006/12/07 13:22:26 steveu Exp $
 */

/*! \file */

#if !defined(_T38_GATEWAY_H_)
#define _T38_GATEWAY_H_

/*! \page t38_gateway_page T.38 real time FAX over IP PSTN gateway
\section t38_gateway_page_sec_1 What does it do?

The T.38 gateway facility provides a robust interface between T.38 IP packet streams and
and 8k samples/second audio streams. It provides the buffering and flow control features needed
to maximum the tolerance of jitter and packet loss on the IP network.

\section t38_gateway_page_sec_2 How does it work?
*/

#define T38_RX_BUF_LEN          2048
#define T38_TX_BUF_LEN          16384

#define T38_TX_HDLC_BUFS        256

/* Make sure the HDLC frame buffers are big enough for ECM frames. */
#define T38_MAX_HDLC_LEN        260

typedef struct
{
    t38_core_state_t t38;
    
    int ecm_allowed;

    /*! \brief HDLC message buffers. */
    uint8_t hdlc_buf[T38_TX_HDLC_BUFS][T38_MAX_HDLC_LEN];
    /*! \brief HDLC message lengths. */
    int hdlc_len[T38_TX_HDLC_BUFS];
    /*! \brief HDLC message status flags. */
    int hdlc_flags[T38_TX_HDLC_BUFS];
    /*! \brief HDLC buffer contents. */
    int hdlc_contents[T38_TX_HDLC_BUFS];
    /*! \brief HDLC buffer number for input. */
    int hdlc_in;
    /*! \brief HDLC buffer number for output. */
    int hdlc_out;

    /*! \brief Progressively calculated CRC for HDLC messaging received from a modem. */
    uint16_t crc;

    /*! \brief non-ECM modem receive data buffer */
    uint8_t rx_data[T38_RX_BUF_LEN];
    int rx_data_ptr;

    /*! \brief non-ECM modem transmit data buffer */
    uint8_t non_ecm_tx_data[T38_TX_BUF_LEN];
    int non_ecm_tx_in_ptr;
    int non_ecm_tx_out_ptr;

    /*! \brief The location of the most recent EOL marker in the non-ECM data buffer */
    int non_ecm_tx_latest_eol_ptr;
    unsigned int bit_stream;
    /*! \brief The non-ECM flow control fill octet (0xFF before the first data, and 0x00
               once data has started). */
    uint8_t non_ecm_flow_control_fill_octet;
    /*! \brief TRUE if we are in the initial all ones part of non-ECM transmission. */
    int non_ecm_at_initial_all_ones;
    /*! \brief TRUE is the end of non-ECM data indication has been received. */
    int non_ecm_data_finished;
    /*! \brief The current octet being sent as non-ECM data. */
    int current_non_ecm_octet;
    /*! \brief The current bit number in the current non-ECM octet. */
    int non_ecm_bit_no;
    /*! \brief A count of the number of non-ECM fill octets generated for flow control control
               purposes. */
    int non_ecm_flow_control_fill_octets;

    int current_rx_data_type;
    int current_rx_field_type;
    int current_rx_field_class;
    int in_progress_rx_indicator;

    /*! \brief The current T.38 data type being sent. */
    int current_tx_data_type;

    /*! \brief TRUE if we are in error correcting (ECM) mode */
    int ecm_mode;
    /*! \brief The current bit rate for the fast modem. */
    int fast_bit_rate;
    /*! \brief The current fast modem type. */
    int fast_modem;
    /*! \brief Use talker echo protection when transmitting. */
    int use_tep;    
    /*! \brief TRUE if a carrier is present. Otherwise FALSE. */
    int rx_signal_present;
    /*! \brief TRUE if a modem has trained correctly. */
    int rx_trained;

    /*! \brief A tone generator context used to generate supervisory tones during
               FAX handling. */
    tone_gen_state_t tone_gen;
    /*! \brief An HDLC context used when receiving HDLC messages. */
    hdlc_rx_state_t hdlcrx;

    /*! \brief HDLC data used when transmitting HDLC messages. */
    hdlc_tx_state_t hdlctx;

    /*! \brief A V.21 FSK modem context used when transmitting HDLC over V.21
               messages. */
    fsk_tx_state_t v21tx;
    /*! \brief A V.21 FSK modem context used when receiving HDLC over V.21
               messages. */
    fsk_rx_state_t v21rx;

#if defined(ENABLE_V17)
    /*! \brief A V.17 modem context used when sending FAXes at 7200bps, 9600bps
               12000bps or 14400bps*/
    v17_tx_state_t v17tx;
    /*! \brief A V.29 modem context used when receiving FAXes at 7200bps, 9600bps
               12000bps or 14400bps*/
    v17_rx_state_t v17rx;
#endif

    /*! \brief A V.29 modem context used when sending FAXes at 7200bps or
               9600bps */
    v29_tx_state_t v29tx;
    /*! \brief A V.29 modem context used when receiving FAXes at 7200bps or
               9600bps */
    v29_rx_state_t v29rx;

    /*! \brief A V.27ter modem context used when sending FAXes at 2400bps or
               4800bps */
    v27ter_tx_state_t v27ter_tx;
    /*! \brief A V.27ter modem context used when receiving FAXes at 2400bps or
               4800bps */
    v27ter_rx_state_t v27ter_rx;

    /*! \brief Used to insert timed silences. */
    silence_gen_state_t silence_gen;

    /*! The current receive signal handler */
    span_rx_handler_t *rx_handler;
    void *rx_user_data;

    /*! The current transmit signal handler */
    span_tx_handler_t *tx_handler;
    void *tx_user_data;
    /*! The transmit signal handler to be used when the current one has finished sending. */
    span_tx_handler_t *next_tx_handler;
    void *next_tx_user_data;

    /*! The number of octets to send in each image packet (non-ECM or ECM) at the current
        rate and the current specified packet interval. */
    int octets_per_data_packet;

    int v21_rx_active;
    int fast_rx_active;
    int samples_since_last_tx_packet;
    int octets_since_last_tx_packet;
    int short_train;

    /*! \brief TRUE if we need to corrupt the HDLC frame in progress, so the receiver cannot
               interpret it. */
    int corrupt_the_frame;
    
    int current_rx_type;
    int current_tx_type;

    logging_state_t logging;
} t38_gateway_state_t;

#ifdef __cplusplus
extern "C" {
#endif

/*! \brief Initialise a gateway mode T.38 context.
    \param s The T.38 context.
    \param tx_packet_handler A callback routine to encapsulate and transmit T.38 packets.
    \param tx_packet_user_data An opaque pointer passed to the tx_packet_handler routine.
    \return A pointer to the termination mode T.38 context, or NULL if there was a problem. */
t38_gateway_state_t *t38_gateway_init(t38_gateway_state_t *s,
                                      t38_tx_packet_handler_t *tx_packet_handler,
                                      void *tx_packet_user_data);

/*! Process a block of received FAX audio samples.
    \brief Process a block of received FAX audio samples.
    \param s The T.38 context.
    \param amp The audio sample buffer.
    \param len The number of samples in the buffer.
    \return The number of samples unprocessed. */
int t38_gateway_rx(t38_gateway_state_t *s, const int16_t *amp, int len);

/*! Generate a block of FAX audio samples.
    \brief Generate a block of FAX audio samples.
    \param s The T.38 context.
    \param amp The audio sample buffer.
    \param max_len The number of samples to be generated.
    \return The number of samples actually generated.
*/
int t38_gateway_tx(t38_gateway_state_t *s, int16_t *amp, int max_len);

/*! Control whether error correcting mode (ECM) is allowed.
    \brief Control whether error correcting mode (ECM) is allowed.
    \param s The T.38 context.
    \param ecm_allowed TRUE is ECM is to be allowed.
*/
void t38_gateway_ecm_control(t38_gateway_state_t *s, int ecm_allowed);

#ifdef __cplusplus
}
#endif

#endif
/*- End of file ------------------------------------------------------------*/
