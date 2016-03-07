/*
 * SpanDSP - a series of DSP components for telephony
 *
 * t38_terminal.h - An implementation of T.38, less the packet exchange part
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
 * $Id: t38_terminal.h,v 1.11 2006/12/07 13:22:26 steveu Exp $
 */

/*! \file */

#if !defined(_T38_TERMINAL_H_)
#define _T38_TERMINAL_H_

/*! \page t38_terminal_page T.38 real time FAX over IP termination
\section t38_terminal_page_sec_1 What does it do?

\section t38_terminal_page_sec_2 How does it work?
*/

#define T38_RX_BUF_LEN          2048
#define T38_TX_BUF_LEN          16384

/* Make sure the HDLC frame buffers are big enough for ECM frames. */
#define T38_MAX_HDLC_LEN        260

typedef struct
{
    t38_core_state_t t38;

    uint8_t hdlc_tx_buf[T38_MAX_HDLC_LEN];
    int hdlc_tx_len;
    int hdlc_tx_ptr;
    int timed_step;

    uint8_t tx_data[T38_TX_BUF_LEN];
    int tx_out_bytes;

    int next_tx_indicator;
    int current_tx_data_type;

    /*! \brief TRUE is a carrier is presnt. Otherwise FALSE. */
    int rx_signal_present;

    /*! \brief A tone generator context used to generate supervisory tones during
               FAX handling. */
    tone_gen_state_t tone_gen;

    t30_state_t t30_state;

    int current_rx_type;
    int current_tx_type;

    /*! \brief TRUE is there has been some T.38 data missed */
    int missing_data;

    /*! The number of octets to send in each image packet (non-ECM or ECM) at the current
        rate and the current specified packet interval. */
    int octets_per_data_packet;

    int32_t samples;
    int32_t next_tx_samples;
    int32_t timeout_rx_samples;

    logging_state_t logging;
} t38_terminal_state_t;

#ifdef __cplusplus
extern "C" {
#endif

int t38_terminal_send_timeout(t38_terminal_state_t *s, int samples);

/*! \brief Initialise a termination mode T.38 context.
    \param s The T.38 context.
    \param calling_party TRUE if the context is for a calling party. FALSE if the
           context is for an answering party.
    \param tx_packet_handler A callback routine to encapsulate and transmit T.38 packets.
    \param tx_packet_user_data An opaque pointer passed to the tx_packet_handler routine.
    \return A pointer to the termination mode T.38 context, or NULL if there was a problem. */
t38_terminal_state_t *t38_terminal_init(t38_terminal_state_t *s,
                                        int calling_party,
                                        t38_tx_packet_handler_t *tx_packet_handler,
                                        void *tx_packet_user_data);

#ifdef __cplusplus
}
#endif

#endif
/*- End of file ------------------------------------------------------------*/
