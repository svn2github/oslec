/*
 * SpanDSP - a series of DSP components for telephony
 *
 * t31.h - A T.31 compatible class 1 FAX modem interface.
 *
 * Written by Steve Underwood <steveu@coppice.org>
 *
 * Copyright (C) 2004 Steve Underwood
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
 * $Id: t31.h,v 1.37 2006/10/24 13:45:28 steveu Exp $
 */

/*! \file */

#if !defined(_T31_H_)
#define _T31_H_

/*! \page t31_page T.31 Class 1 FAX modem protocol handling
\section t31_page_sec_1 What does it do?
The T.31 class 1 FAX modem modules implements a class 1 interface to the FAX
modems in spandsp.

\section t31_page_sec_2 How does it work?
*/

typedef struct t31_state_s t31_state_t;

typedef int (t31_modem_control_handler_t)(t31_state_t *s, void *user_data, int op, const char *num);

#define T31_TX_BUF_LEN          (4096*32)
#define T31_TX_BUF_HIGH_TIDE    (4096*32 - 1024)
#define T31_TX_BUF_LOW_TIDE     (1024)

/*!
    T.31 descriptor. This defines the working state for a single instance of
    a T.31 FAX modem.
*/
struct t31_state_s
{
    at_state_t at_state;
    t31_modem_control_handler_t *modem_control_handler;
    void *modem_control_user_data;

    /*! The current receive signal handler */
    span_rx_handler_t *rx_handler;
    void *rx_user_data;

    /*! The current transmit signal handler */
    span_tx_handler_t *tx_handler;
    void *tx_user_data;
    /*! The transmit signal handler to be used when the current one has finished sending. */
    span_tx_handler_t *next_tx_handler;
    void *next_tx_user_data;

    /*! If TRUE, transmit silence when there is nothing else to transmit. If FALSE return only
        the actual generated audio. Note that this only affects untimed silences. Timed silences
        (e.g. the 75ms silence between V.21 and a high speed modem) will alway be transmitted as
        silent audio. */
    int transmit_on_idle;

    uint8_t hdlc_tx_buf[256];
    int hdlc_tx_len;
    int hdlc_tx_ptr;
    /*! TRUE if DLE prefix just used */
    int dled;
    uint8_t tx_data[T31_TX_BUF_LEN];
    /*! \brief The number of bytes stored in transmit buffer. */
    int tx_in_bytes;
    /*! \brief The number of bytes sent from the transmit buffer. */
    int tx_out_bytes;
    int tx_holding;
    int tx_data_started;
    int bit_no;
    int current_byte;

    /*! \brief The current bit rate for the FAX fast message transfer modem. */
    int bit_rate;
    int rx_message_received;

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

    /*! \brief Rx power meter, use to detect silence */
    power_meter_t rx_power;
    int16_t last_sample;
    int32_t silence_threshold_power;
    
    t38_core_state_t t38;

	/*! \brief Samples of silence heard */
    int silence_heard;
	/*! \brief Samples of silence awaited */
    int silence_awaited;
    /*! \brief Samples elapsed in the current call */
    int64_t call_samples;
    int64_t dte_data_timeout;
    int modem;
    int short_train;
    int hdlc_final;
    int data_final;
    queue_t rx_queue;

    uint8_t hdlc_rx_buf[256];
    int hdlc_rx_len;
    
    int t38_mode;
    int timed_step;
    int current_tx_data;
    int64_t next_send_samples;
    int next_tx_indicator;

    int current_rx_type;
    int current_tx_type;

    /*! \brief TRUE is there has been some T.38 data missed */
    int missing_data;

    int octets_per_non_ecm_packet;

    /*! \brief Error and flow logging control */
    logging_state_t logging;
};

#ifdef __cplusplus
extern "C" {
#endif

void t31_call_event(t31_state_t *s, int event);

int t31_at_rx(t31_state_t *s, const char *t, int len);

/*! Process a block of received T.31 modem audio samples.
    \brief Process a block of received T.31 modem audio samples.
    \param s The T.31 modem context.
    \param amp The audio sample buffer.
    \param len The number of samples in the buffer.
    \return The number of samples unprocessed. */
int t31_rx(t31_state_t *s, int16_t amp[], int len);

/*! Generate a block of T.31 modem audio samples.
    \brief Generate a block of T.31 modem audio samples.
    \param s The T.31 modem context.
    \param amp The audio sample buffer.
    \param max_len The number of samples to be generated.
    \return The number of samples actually generated.
*/
int t31_tx(t31_state_t *s, int16_t amp[], int max_len);

int t31_t38_send_timeout(t31_state_t *s, int samples);

/*! Select whether silent audio will be sent when transmit is idle.
    \brief Select whether silent audio will be sent when transmit is idle.
    \param s The T.31 modem context.
    \param transmit_on_idle TRUE if silent audio should be output when the transmitter is
           idle. FALSE to transmit zero length audio when the transmitter is idle. The default
           behaviour is FALSE.
*/
void t31_set_transmit_on_idle(t31_state_t *s, int transmit_on_idle);

/*! Initialise a T.31 context. This must be called before the first
    use of the context, to initialise its contents.
    \brief Initialise a T.31 context.
    \param s The T.31 context.
    \param at_tx_handler A callback routine to handle AT interpreter channel output.
    \param at_tx_user_data An opaque pointer passed in called to at_tx_handler.
    \param modem_control_handler A callback routine to handle control of the modem (off-hook, etc).
    \param modem_control_user_data An opaque pointer passed in called to modem_control_handler.
    \param tx_t38_packet_handler ???
    \param tx_t38_packet_user_data ???
    \return A pointer to the T.31 context. */
t31_state_t *t31_init(t31_state_t *s,
                      at_tx_handler_t *at_tx_handler,
                      void *at_tx_user_data,
                      t31_modem_control_handler_t *modem_control_handler,
                      void *modem_control_user_data,
                      t38_tx_packet_handler_t *tx_t38_packet_handler,
                      void *tx_t38_packet_user_data);

/*! Release a T.31 context.
    \brief Release a T.31 context.
    \param s The T.31 context.
    \return 0 for OK */
int t31_release(t31_state_t *s);

#ifdef __cplusplus
}
#endif

#endif
/*- End of file ------------------------------------------------------------*/
