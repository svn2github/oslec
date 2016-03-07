/*
 * SpanDSP - a series of DSP components for telephony
 *
 * fax.h - definitions for analogue line ITU T.30 fax processing
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
 * $Id: fax.h,v 1.20 2006/10/24 13:45:28 steveu Exp $
 */

/*! \file */

#if !defined(_FAX_H_)
#define _FAX_H_

/*! \page fax_page FAX over analogue modem handling

\section fax_page_sec_1 What does it do?

\section fax_page_sec_2 How does it work?
*/

typedef struct fax_state_s fax_state_t;

typedef void (fax_flush_handler_t)(fax_state_t *s, void *user_data, int which);

/*!
    Analogue line T.30 FAX channel descriptor. This defines the state of a single working
    instance of an analogue line soft-FAX machine.
*/
struct fax_state_s
{
    /* This must be kept the first thing in the structure, so it can be pointed
       to reliably as the structures change over time. */
    t30_state_t t30_state;

    /*! TRUE is talker echo protection should be sent for the image modems */
    int use_tep;

    fax_flush_handler_t *flush_handler;
    void *flush_user_data;

    /*! The current receive signal handler */
    span_rx_handler_t *rx_handler;
    void *rx_user_data;

    /*! The current transmit signal handler */
    span_tx_handler_t *tx_handler;
    void *tx_user_data;
    int tx_hdlc_preamble_len;
    /*! The transmit signal handler to be used when the current one has finished sending. */
    span_tx_handler_t *next_tx_handler;
    void *next_tx_user_data;
    
    /*! If TRUE, transmission is in progress */
    int transmit;

    /*! If TRUE, transmit silence when there is nothing else to transmit. If FALSE return only
        the actual generated audio. Note that this only affects untimed silences. Timed silences
        (e.g. the 75ms silence between V.21 and a high speed modem) will alway be transmitted as
        silent audio. */
    int transmit_on_idle;

    /*! \brief A tone generator context used to generate supervisory tones during
               FAX handling. */
    tone_gen_state_t tone_gen;
    /*! \brief An HDLC context used when receiving HDLC over V.21 messages. */
    hdlc_rx_state_t hdlcrx;
    /*! \brief An HDLC context used when transmitting HDLC over V.21 messages. */
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
    /*! \brief A V.27ter modem context used when sending FAXes at 2400bps or
               4800bps */
    v27ter_tx_state_t v27ter_tx;
    /*! \brief A V.27ter modem context used when receiving FAXes at 2400bps or
               4800bps */
    v27ter_rx_state_t v27ter_rx;
    /*! \brief A V.29 modem context used when sending FAXes at 7200bps or
               9600bps */
    v29_tx_state_t v29tx;
    /*! \brief A V.29 modem context used when receiving FAXes at 7200bps or
               9600bps */
    v29_rx_state_t v29rx;
    /*! \brief Used to insert timed silences. */
    silence_gen_state_t silence_gen;
    /*! \brief */
    dc_restore_state_t dc_restore;

    /*! \brief TRUE is the short training sequence should be used. */
    int short_train;

    /*! The currently select receiver type */
    int current_rx_type;
    /*! The currently select transmitter type */
    int current_tx_type;

    int first_tx_hdlc_frame;

    /*! Audio logging file handles */
    int fax_audio_rx_log;
    int fax_audio_tx_log;
    /*! \brief Error and flow logging control */
    logging_state_t logging;
};

#ifdef __cplusplus
extern "C" {
#endif

/*! Apply T.30 receive processing to a block of audio samples.
    \brief Apply T.30 receive processing to a block of audio samples.
    \param s The FAX context.
    \param amp The audio sample buffer.
    \param len The number of samples in the buffer.
    \return The number of samples unprocessed. This should only be non-zero if
            the software has reached the end of the FAX call.
*/
int fax_rx(fax_state_t *s, int16_t *amp, int len);

/*! Apply T.30 transmit processing to generate a block of audio samples.
    \brief Apply T.30 transmit processing to generate a block of audio samples.
    \param s The FAX context.
    \param amp The audio sample buffer.
    \param max_len The number of samples to be generated.
    \return The number of samples actually generated. This will be zero when
            there is nothing to send.
*/
int fax_tx(fax_state_t *s, int16_t *amp, int max_len);

void fax_set_flush_handler(fax_state_t *s, fax_flush_handler_t *handler, void *user_data);

/*! Select whether silent audio will be sent when FAX transmit is idle.
    \brief Select whether silent audio will be sent when FAX transmit is idle.
    \param s The FAX context.
    \param transmit_on_idle TRUE if silent audio should be output when the FAX transmitter is
           idle. FALSE to transmit zero length audio when the FAX transmitter is idle. The default
           behaviour is FALSE.
*/
void fax_set_transmit_on_idle(fax_state_t *s, int transmit_on_idle);

/*! Select whether talker echo protection tone will be sent for the image modems.
    \brief Select whether TEP will be sent for the image modems.
    \param s The FAX context.
    \param use_tep TRUE if TEP should be sent.
*/
void fax_set_tep_mode(fax_state_t *s, int use_tep);

/*! Initialise a FAX context.
    \brief Initialise a FAX context.
    \param s The FAX context.
    \param calling_party TRUE if the context is for a calling party. FALSE if the
           context is for an answering party.
    \return 0 for OK, else -1.
*/
int fax_init(fax_state_t *s, int calling_party);

/*! Release a FAX context.
    \brief Release a FAX context.
    \param s The FAX context. */
int fax_release(fax_state_t *s);

#ifdef __cplusplus
}
#endif

#endif
/*- End of file ------------------------------------------------------------*/
