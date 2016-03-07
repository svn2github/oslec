/*
 * SpanDSP - a series of DSP components for telephony
 *
 * t38_terminal.c - An implementation of a T.38 terminal, less the packet exchange part
 *
 * Written by Steve Underwood <steveu@coppice.org>
 *
 * Copyright (C) 2005, 2006 Steve Underwood
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
 * $Id: t38_terminal.c,v 1.50 2006/12/08 12:47:29 steveu Exp $
 */

/*! \file */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <stdio.h>
#include <inttypes.h>
#include <fcntl.h>
#include <time.h>
#include <string.h>
#if defined(HAVE_TGMATH_H)
#include <tgmath.h>
#endif
#if defined(HAVE_MATH_H)
#include <math.h>
#endif
#include <assert.h>
#include <tiffio.h>

#include "spandsp/telephony.h"
#include "spandsp/logging.h"
#include "spandsp/bit_operations.h"
#include "spandsp/queue.h"
#include "spandsp/power_meter.h"
#include "spandsp/complex.h"
#include "spandsp/tone_generate.h"
#include "spandsp/async.h"
#include "spandsp/hdlc.h"
#include "spandsp/fsk.h"
#include "spandsp/v29rx.h"
#include "spandsp/v29tx.h"
#include "spandsp/v27ter_rx.h"
#include "spandsp/v27ter_tx.h"
#if defined(ENABLE_V17)
#include "spandsp/v17rx.h"
#include "spandsp/v17tx.h"
#endif
#include "spandsp/t4.h"

#include "spandsp/t30_fcf.h"
#include "spandsp/t35.h"
#include "spandsp/t30.h"

#include "spandsp/t38_core.h"
#include "spandsp/t38_terminal.h"

#define MS_PER_TX_CHUNK             30

#define INDICATOR_TX_COUNT          3
/* Backstop timeout if reception of packets stops in the middle of a burst */
#define MID_RX_TIMEOUT              15000

enum
{
    T38_TIMED_STEP_NONE = 0,
    T38_TIMED_STEP_NON_ECM_MODEM,
    T38_TIMED_STEP_NON_ECM_MODEM_2,
    T38_TIMED_STEP_NON_ECM_MODEM_3,
    T38_TIMED_STEP_HDLC_MODEM,
    T38_TIMED_STEP_HDLC_MODEM_2,
    T38_TIMED_STEP_HDLC_MODEM_3,
    T38_TIMED_STEP_CED,
    T38_TIMED_STEP_CNG,
    T38_TIMED_STEP_PAUSE
};

static int get_non_ecm_image_chunk(t38_terminal_state_t *s, uint8_t *buf, int len)
{
    int i;
    int j;
    int bit;
    int byte;

    for (i = 0;  i < len;  i++)
    {
        byte = 0;
        for (j = 0;  j < 8;  j++)
        {
            if ((bit = t30_non_ecm_get_bit(&s->t30_state)) == PUTBIT_END_OF_DATA)
            {
                if (j > 0)
                {
                    byte <<= (8 - j);
                    buf[i++] = (uint8_t) byte;
                }
                return -i;
            }
            byte = (byte << 1) | (bit & 0x01);
        }
        buf[i] = (uint8_t) byte;
    }
    return i;
}
/*- End of function --------------------------------------------------------*/

static int process_rx_missing(t38_core_state_t *s, void *user_data, int rx_seq_no, int expected_seq_no)
{
    t38_terminal_state_t *t;
    
    t = (t38_terminal_state_t *) user_data;
    t->missing_data = TRUE;
    return 0;
}
/*- End of function --------------------------------------------------------*/

static int process_rx_indicator(t38_core_state_t *s, void *user_data, int indicator)
{
    t38_terminal_state_t *t;
    
    t = (t38_terminal_state_t *) user_data;
    /* In termination mode we don't care very much about indicators telling us training
       is starting. We only care about the actual data. */
    switch (indicator)
    {
    case T38_IND_NO_SIGNAL:
        if (t->t38.current_rx_indicator == T38_IND_V21_PREAMBLE
            &&
            (t->current_rx_type == T30_MODEM_V21  ||  t->current_rx_type == T30_MODEM_CNG))
        {
            t30_hdlc_accept(&(t->t30_state), TRUE, NULL, PUTBIT_CARRIER_DOWN);
        }
        t->timeout_rx_samples = 0;
        break;
    case T38_IND_CNG:
        break;
    case T38_IND_CED:
        break;
    case T38_IND_V21_PREAMBLE:
        if (t->current_rx_type == T30_MODEM_V21)
        {
            t30_hdlc_accept(&(t->t30_state), TRUE, NULL, PUTBIT_CARRIER_UP);
            t30_hdlc_accept(&(t->t30_state), TRUE, NULL, PUTBIT_FRAMING_OK);
        }
        break;
    case T38_IND_V27TER_2400_TRAINING:
        t->timeout_rx_samples = t->samples + ms_to_samples(MID_RX_TIMEOUT);
        break;
    case T38_IND_V27TER_4800_TRAINING:
        t->timeout_rx_samples = t->samples + ms_to_samples(MID_RX_TIMEOUT);
        break;
    case T38_IND_V29_7200_TRAINING:
        t->timeout_rx_samples = t->samples + ms_to_samples(MID_RX_TIMEOUT);
        break;
    case T38_IND_V29_9600_TRAINING:
        t->timeout_rx_samples = t->samples + ms_to_samples(MID_RX_TIMEOUT);
        break;
    case T38_IND_V17_7200_SHORT_TRAINING:
        t->timeout_rx_samples = t->samples + ms_to_samples(MID_RX_TIMEOUT);
        break;
    case T38_IND_V17_7200_LONG_TRAINING:
        t->timeout_rx_samples = t->samples + ms_to_samples(MID_RX_TIMEOUT);
        break;
    case T38_IND_V17_9600_SHORT_TRAINING:
        t->timeout_rx_samples = t->samples + ms_to_samples(MID_RX_TIMEOUT);
        break;
    case T38_IND_V17_9600_LONG_TRAINING:
        t->timeout_rx_samples = t->samples + ms_to_samples(MID_RX_TIMEOUT);
        break;
    case T38_IND_V17_12000_SHORT_TRAINING:
        t->timeout_rx_samples = t->samples + ms_to_samples(MID_RX_TIMEOUT);
        break;
    case T38_IND_V17_12000_LONG_TRAINING:
        t->timeout_rx_samples = t->samples + ms_to_samples(MID_RX_TIMEOUT);
        break;
    case T38_IND_V17_14400_SHORT_TRAINING:
        t->timeout_rx_samples = t->samples + ms_to_samples(MID_RX_TIMEOUT);
        break;
    case T38_IND_V17_14400_LONG_TRAINING:
        t->timeout_rx_samples = t->samples + ms_to_samples(MID_RX_TIMEOUT);
        break;
    case T38_IND_V8_ANSAM:
        break;
    case T38_IND_V8_SIGNAL:
        break;
    case T38_IND_V34_CNTL_CHANNEL_1200:
        break;
    case T38_IND_V34_PRI_CHANNEL:
        break;
    case T38_IND_V34_CC_RETRAIN:
        break;
    case T38_IND_V33_12000_TRAINING:
        t->timeout_rx_samples = t->samples + ms_to_samples(MID_RX_TIMEOUT);
        break;
    case T38_IND_V33_14400_TRAINING:
        t->timeout_rx_samples = t->samples + ms_to_samples(MID_RX_TIMEOUT);
        break;
    default:
        break;
    }
    t->tx_out_bytes = 0;
    t->missing_data = FALSE;
    return 0;
}
/*- End of function --------------------------------------------------------*/

static int process_rx_data(t38_core_state_t *s, void *user_data, int data_type, int field_type, const uint8_t *buf, int len)
{
    int i;
    t38_terminal_state_t *t;
    
    t = (t38_terminal_state_t *) user_data;
#if 0
    /* In termination mode we don't care very much what the data type is. */
    switch (data_type)
    {
    case T38_DATA_V21:
    case T38_DATA_V27TER_2400:
    case T38_DATA_V27TER_4800:
    case T38_DATA_V29_7200:
    case T38_DATA_V29_9600:
    case T38_DATA_V17_7200:
    case T38_DATA_V17_9600:
    case T38_DATA_V17_12000:
    case T38_DATA_V17_14400:
    case T38_DATA_V8:
    case T38_DATA_V34_PRI_RATE:
    case T38_DATA_V34_CC_1200:
    case T38_DATA_V34_PRI_CH:
    case T38_DATA_V33_12000:
    case T38_DATA_V33_14400:
    default:
        break;
    }
#endif

    switch (field_type)
    {
    case T38_FIELD_HDLC_DATA:
        if (t->tx_out_bytes + len <= T38_MAX_HDLC_LEN)
        {
            for (i = 0;  i < len;  i++)
                t->tx_data[t->tx_out_bytes++] = bit_reverse8(buf[i]);
        }
        t->timeout_rx_samples = t->samples + ms_to_samples(MID_RX_TIMEOUT);
        break;
    case T38_FIELD_HDLC_FCS_OK:
        if (len > 0)
        {
            span_log(&t->logging, SPAN_LOG_WARNING, "There is data in a T38_FIELD_HDLC_FCS_OK!\n");
            /* The sender has incorrectly included data in this message. It is unclear what we should do
               with it, to maximise tolerance of buggy implementations. */
        }
        span_log(&t->logging, SPAN_LOG_FLOW, "Type %s - CRC OK (%s)\n", (t->tx_out_bytes >= 3)  ?  t30_frametype(t->tx_data[2])  :  "???", (t->missing_data)  ?  "missing octets"  :  "clean");
        /* Don't deal with zero length frames. Some T.38 implementations send multiple T38_FIELD_HDLC_FCS_OK
           packets, when they have sent no data for the body of the frame. */
        if (t->tx_out_bytes > 0)
            t30_hdlc_accept(&(t->t30_state), !t->missing_data, t->tx_data, t->tx_out_bytes);
        t->tx_out_bytes = 0;
        t->missing_data = FALSE;
        t->timeout_rx_samples = t->samples + ms_to_samples(MID_RX_TIMEOUT);
        break;
    case T38_FIELD_HDLC_FCS_BAD:
        if (len > 0)
        {
            span_log(&t->logging, SPAN_LOG_WARNING, "There is data in a T38_FIELD_HDLC_FCS_BAD!\n");
            /* The sender has incorrectly included data in this message. We can safely ignore it, as the
               bad FCS means we will throw away the whole message, anyway. */
        }
        span_log(&t->logging, SPAN_LOG_FLOW, "Type %s - CRC bad (%s)\n", (t->tx_out_bytes >= 3)  ?  t30_frametype(t->tx_data[2])  :  "???", (t->missing_data)  ?  "missing octets"  :  "clean");
        if (t->tx_out_bytes > 0)
            t30_hdlc_accept(&(t->t30_state), FALSE, t->tx_data, t->tx_out_bytes);
        t->tx_out_bytes = 0;
        t->missing_data = FALSE;
        t->timeout_rx_samples = t->samples + ms_to_samples(MID_RX_TIMEOUT);
        break;
    case T38_FIELD_HDLC_FCS_OK_SIG_END:
        if (len > 0)
        {
            span_log(&t->logging, SPAN_LOG_WARNING, "There is data in a T38_FIELD_HDLC_FCS_OK_SIG_END!\n");
            /* The sender has incorrectly included data in this message. It is unclear what we should do
               with it, to maximise tolerance of buggy implementations. */
        }
        span_log(&t->logging, SPAN_LOG_FLOW, "Type %s - CRC OK, sig end (%s)\n", (t->tx_out_bytes >= 3)  ?  t30_frametype(t->tx_data[2])  :  "???", (t->missing_data)  ?  "missing octets"  :  "clean");
        /* Don't deal with zero length frames. Some T.38 implementations send multiple T38_FIELD_HDLC_FCS_OK
           packets, when they have sent no data for the body of the frame. */
        if (t->tx_out_bytes > 0)
            t30_hdlc_accept(&(t->t30_state), !t->missing_data, t->tx_data, t->tx_out_bytes);
        t30_hdlc_accept(&(t->t30_state), TRUE, NULL, PUTBIT_CARRIER_DOWN);
        t->tx_out_bytes = 0;
        t->missing_data = FALSE;
        t->timeout_rx_samples = 0;
        break;
    case T38_FIELD_HDLC_FCS_BAD_SIG_END:
        if (len > 0)
        {
            span_log(&t->logging, SPAN_LOG_WARNING, "There is data in a T38_FIELD_HDLC_FCS_BAD_SIG_END!\n");
            /* The sender has incorrectly included data in this message. We can safely ignore it, as the
               bad FCS means we will throw away the whole message, anyway. */
        }
        span_log(&t->logging, SPAN_LOG_FLOW, "Type %s - CRC bad, sig end (%s)\n", (t->tx_out_bytes >= 3)  ?  t30_frametype(t->tx_data[2])  :  "???", (t->missing_data)  ?  "missing octets"  :  "clean");
        if (t->tx_out_bytes > 0)
            t30_hdlc_accept(&(t->t30_state), FALSE, t->tx_data, t->tx_out_bytes);
        t30_hdlc_accept(&(t->t30_state), TRUE, NULL, PUTBIT_CARRIER_DOWN);
        t->tx_out_bytes = 0;
        t->missing_data = FALSE;
        t->timeout_rx_samples = 0;
        break;
    case T38_FIELD_HDLC_SIG_END:
        if (len > 0)
        {
            span_log(&t->logging, SPAN_LOG_WARNING, "There is data in a T38_FIELD_HDLC_SIG_END!\n");
            /* The sender has incorrectly included data in this message, but there seems nothing meaningful
               it could be. There could not be an FCS good/bad report beyond this. */
        }
        /* This message is expected under 2 circumstances. One is as an alternative to T38_FIELD_HDLC_FCS_OK_SIG_END - 
           i.e. they send T38_FIELD_HDLC_FCS_OK, and then T38_FIELD_HDLC_SIG_END when the carrier actually drops.
           The other is because the HDLC signal drops unexpectedly - i.e. not just after a final frame. */
        /* WORKAROUND: At least some Mediatrix boxes have a bug, where they can send this message at the
                       end of non-ECM data. We need to tolerate this. We use the generic receive complete
                       indication, rather than the specific HDLC carrier down. */
        t->tx_out_bytes = 0;
        t->missing_data = FALSE;
        t->timeout_rx_samples = 0;
        t30_receive_complete(&(t->t30_state));
        break;
    case T38_FIELD_T4_NON_ECM_DATA:
        if (!t->rx_signal_present)
        {
            t30_non_ecm_put_bit(&(t->t30_state), PUTBIT_TRAINING_SUCCEEDED);
            t->rx_signal_present = TRUE;
        }
        for (i = 0;  i < len;  i++)
            t30_non_ecm_putbyte(&(t->t30_state), buf[i]);
        t->timeout_rx_samples = t->samples + ms_to_samples(MID_RX_TIMEOUT);
        break;
    case T38_FIELD_T4_NON_ECM_SIG_END:
        if (len > 0)
        {
            if (!t->rx_signal_present)
            {
                t30_non_ecm_put_bit(&(t->t30_state), PUTBIT_TRAINING_SUCCEEDED);
                t->rx_signal_present = TRUE;
            }
            for (i = 0;  i < len;  i++)
                t30_non_ecm_putbyte(&(t->t30_state), buf[i]);
        }
        /* WORKAROUND: At least some Mediatrix boxes have a bug, where they can send HDLC signal end where
                       they should send non-ECM signal end. It is possible they also do the opposite.
                       We need to tolerate this, so we use the generic receive complete
                       indication, rather than the specific non-ECM carrier down. */
        t30_receive_complete(&(t->t30_state));
        t->rx_signal_present = FALSE;
        t->timeout_rx_samples = 0;
        break;
    case T38_FIELD_CM_MESSAGE:
    case T38_FIELD_JM_MESSAGE:
    case T38_FIELD_CI_MESSAGE:
    case T38_FIELD_V34RATE:
    default:
        break;
    }
    return 0;
}
/*- End of function --------------------------------------------------------*/

static void send_hdlc(void *user_data, const uint8_t *msg, int len)
{
    t38_terminal_state_t *s;
    int j;

    s = (t38_terminal_state_t *) user_data;
    if (len <= 0)
    {
        s->hdlc_tx_len = -1;
    }
    else
    {
        for (j = 0;  j < len;  j++)
            s->hdlc_tx_buf[j] = bit_reverse8(msg[j]);
        s->hdlc_tx_len = len;
        s->hdlc_tx_ptr = 0;
    }
}
/*- End of function --------------------------------------------------------*/

int t38_terminal_send_timeout(t38_terminal_state_t *s, int samples)
{
    int len;
    int i;
    int previous;
    uint8_t buf[100];
    /* Training times for all the modem options, with and without TEP */
    static const int training_time[] =
    {
           0,      0,   /* T38_IND_NO_SIGNAL */
           0,      0,   /* T38_IND_CNG */
           0,      0,   /* T38_IND_CED */
        1000,   1000,   /* T38_IND_V21_PREAMBLE */
         943,   1158,   /* T38_IND_V27TER_2400_TRAINING */
         708,    923,   /* T38_IND_V27TER_4800_TRAINING */
         234,    454,   /* T38_IND_V29_7200_TRAINING */
         234,    454,   /* T38_IND_V29_9600_TRAINING */
         142,    367,   /* T38_IND_V17_7200_SHORT_TRAINING */
        1393,   1618,   /* T38_IND_V17_7200_LONG_TRAINING */
         142,    367,   /* T38_IND_V17_9600_SHORT_TRAINING */
        1393,   1618,   /* T38_IND_V17_9600_LONG_TRAINING */
         142,    367,   /* T38_IND_V17_12000_SHORT_TRAINING */
        1393,   1618,   /* T38_IND_V17_12000_LONG_TRAINING */
         142,    367,   /* T38_IND_V17_14400_SHORT_TRAINING */
        1393,   1618,   /* T38_IND_V17_14400_LONG_TRAINING */
           0,      0,   /* T38_IND_V8_ANSAM */
           0,      0,   /* T38_IND_V8_SIGNAL */
           0,      0,   /* T38_IND_V34_CNTL_CHANNEL_1200 */
           0,      0,   /* T38_IND_V34_PRI_CHANNEL */
           0,      0,   /* T38_IND_V34_CC_RETRAIN */
           0,      0,   /* T38_IND_V33_12000_TRAINING */
           0,      0,   /* T38_IND_V33_14400_TRAINING */
    };

    if (s->current_rx_type == T30_MODEM_DONE  ||  s->current_tx_type == T30_MODEM_DONE)
        return TRUE;

    s->samples += samples;
    t30_timer_update(&s->t30_state, samples);
    if (s->timeout_rx_samples  &&  s->samples > s->timeout_rx_samples)
    {
        span_log(&s->logging, SPAN_LOG_FLOW, "Timeout mid-receive\n");
        s->timeout_rx_samples = 0;
        t30_receive_complete(&(s->t30_state));
    }
    if (s->timed_step == T38_TIMED_STEP_NONE)
        return FALSE;
    if (s->samples < s->next_tx_samples)
        return FALSE;
    /* Its time to send something */
    switch (s->timed_step)
    {
    case T38_TIMED_STEP_NON_ECM_MODEM:
        /* Create a 75ms silence */
        if (s->t38.current_tx_indicator != T38_IND_NO_SIGNAL)
            t38_core_send_indicator(&s->t38, T38_IND_NO_SIGNAL, INDICATOR_TX_COUNT);
        s->timed_step = T38_TIMED_STEP_NON_ECM_MODEM_2;
        s->next_tx_samples += ms_to_samples(75);
        break;
    case T38_TIMED_STEP_NON_ECM_MODEM_2:
        /* Switch on a fast modem, and give the training time to complete */
        t38_core_send_indicator(&s->t38, s->next_tx_indicator, INDICATOR_TX_COUNT);
        s->timed_step = T38_TIMED_STEP_NON_ECM_MODEM_3;
        s->next_tx_samples += ms_to_samples(training_time[s->next_tx_indicator << 1]);
        break;
    case T38_TIMED_STEP_NON_ECM_MODEM_3:
        /* Send a chunk of non-ECM image data */
        /* T.38 says it is OK to send the last of the non-ECM data in the signal end message.
           However, I think the early versions of T.38 said the signal end message should not
           contain data. Hopefully, following the current spec will not cause compatibility
           issues. */
        if ((len = get_non_ecm_image_chunk(s, buf, s->octets_per_data_packet)) > 0)
        {
            s->next_tx_samples += ms_to_samples(MS_PER_TX_CHUNK);
            t38_core_send_data(&s->t38, s->current_tx_data_type, T38_FIELD_T4_NON_ECM_DATA, buf, len);
        }
        else
        {
            t38_core_send_data(&s->t38, s->current_tx_data_type, T38_FIELD_T4_NON_ECM_SIG_END, buf, -len);
            /* This should not be needed, since the message above indicates the end of the signal, but it
               seems like it can improve compatibility with quirky implementations. */
            t38_core_send_indicator(&s->t38, T38_IND_NO_SIGNAL, INDICATOR_TX_COUNT);
            s->timed_step = T38_TIMED_STEP_NONE;
            t30_send_complete(&(s->t30_state));
        }
        break;
    case T38_TIMED_STEP_HDLC_MODEM:
        /* Send HDLC preambling */
        t38_core_send_indicator(&s->t38, s->next_tx_indicator, INDICATOR_TX_COUNT);
        s->next_tx_samples += ms_to_samples(training_time[s->next_tx_indicator << 1]);
        s->timed_step = T38_TIMED_STEP_HDLC_MODEM_2;
        break;
    case T38_TIMED_STEP_HDLC_MODEM_2:
        /* Send a chunk of HDLC data */
        i = s->octets_per_data_packet;
        if (i >= (s->hdlc_tx_len - s->hdlc_tx_ptr))
        {
            i = s->hdlc_tx_len - s->hdlc_tx_ptr;
            s->timed_step = T38_TIMED_STEP_HDLC_MODEM_3;
        }
        t38_core_send_data(&s->t38, s->current_tx_data_type, T38_FIELD_HDLC_DATA, &s->hdlc_tx_buf[s->hdlc_tx_ptr], i);
        s->hdlc_tx_ptr += i;
        s->next_tx_samples += ms_to_samples(MS_PER_TX_CHUNK);
        break;
    case T38_TIMED_STEP_HDLC_MODEM_3:
        /* End of HDLC frame */
        previous = s->current_tx_data_type;
        s->hdlc_tx_ptr = 0;
        s->hdlc_tx_len = 0;
        t30_send_complete(&s->t30_state);
        if (s->hdlc_tx_len < 0)
        {
            t38_core_send_data(&s->t38, previous, T38_FIELD_HDLC_FCS_OK_SIG_END, NULL, 0);
            /* We have already sent T38_FIELD_HDLC_FCS_OK_SIG_END. It seems some boxes may not like
               us sending a T38_FIELD_HDLC_SIG_END at this point. Just say there is no signal. */
            t38_core_send_indicator(&s->t38, T38_IND_NO_SIGNAL, INDICATOR_TX_COUNT);
            s->hdlc_tx_len = 0;
            t30_send_complete(&s->t30_state);
            if (s->hdlc_tx_len)
                s->timed_step = T38_TIMED_STEP_HDLC_MODEM;
        }
        else
        {
            t38_core_send_data(&s->t38, previous, T38_FIELD_HDLC_FCS_OK, NULL, 0);
            if (s->hdlc_tx_len)
                s->timed_step = T38_TIMED_STEP_HDLC_MODEM_2;
        }
        s->next_tx_samples += ms_to_samples(MS_PER_TX_CHUNK);
        break;
    case T38_TIMED_STEP_CED:
        /* Initial 200ms delay over. Send the CED indicator */
        s->next_tx_samples = s->samples + ms_to_samples(3000);
        s->timed_step = T38_TIMED_STEP_PAUSE;
        t38_core_send_indicator(&s->t38, T38_IND_CED, INDICATOR_TX_COUNT);
        s->current_tx_data_type = T38_DATA_NONE;
        break;
    case T38_TIMED_STEP_CNG:
        /* Initial short delay over. Send the CNG indicator */
        s->timed_step = T38_TIMED_STEP_NONE;
        t38_core_send_indicator(&s->t38, T38_IND_CNG, INDICATOR_TX_COUNT);
        s->current_tx_data_type = T38_DATA_NONE;
        break;
    case T38_TIMED_STEP_PAUSE:
        /* End of timed pause */
        s->timed_step = T38_TIMED_STEP_NONE;
        t30_send_complete(&s->t30_state);
        break;
    }
    return FALSE;
}
/*- End of function --------------------------------------------------------*/

static void set_rx_type(void *user_data, int type, int short_train, int use_hdlc)
{
    t38_terminal_state_t *s;

    s = (t38_terminal_state_t *) user_data;
    span_log(&s->logging, SPAN_LOG_FLOW, "Set rx type %d\n", type);
    s->current_rx_type = type;
}
/*- End of function --------------------------------------------------------*/

static void set_tx_type(void *user_data, int type, int short_train, int use_hdlc)
{
    t38_terminal_state_t *s;

    s = (t38_terminal_state_t *) user_data;
    span_log(&s->logging, SPAN_LOG_FLOW, "Set tx type %d\n", type);
    if (s->current_tx_type == type)
        return;

    switch (type)
    {
    case T30_MODEM_NONE:
        s->timed_step = T38_TIMED_STEP_NONE;
        s->current_tx_data_type = T38_DATA_NONE;
        break;
    case T30_MODEM_PAUSE:
        s->next_tx_samples = s->samples + ms_to_samples(short_train);
        s->timed_step = T38_TIMED_STEP_PAUSE;
        s->current_tx_data_type = T38_DATA_NONE;
        break;
    case T30_MODEM_CED:
        /* A 200ms initial delay is specified. Delay this amount before the CED indicator is sent. */
        s->next_tx_samples = s->samples + ms_to_samples(200);
        s->timed_step = T38_TIMED_STEP_CED;
        s->current_tx_data_type = T38_DATA_NONE;
        break;
    case T30_MODEM_CNG:
        /* Allow a short initial delay, so the chances of the other end actually being ready to receive
           the CNG indicator are improved. */
        s->next_tx_samples = s->samples + ms_to_samples(200);
        s->timed_step = T38_TIMED_STEP_CNG;
        s->current_tx_data_type = T38_DATA_NONE;
        break;
    case T30_MODEM_V21:
        if (s->current_tx_type > T30_MODEM_V21)
        {
            /* Pause before switching from phase C, as per T.30. If we omit this, the receiver
               might not see the carrier fall between the high speed and low speed sections. */
            s->next_tx_samples = s->samples + ms_to_samples(75);
        }
        else
        {
            s->next_tx_samples = s->samples;
        }
        s->octets_per_data_packet = MS_PER_TX_CHUNK*300/(8*1000);
        s->next_tx_indicator = T38_IND_V21_PREAMBLE;
        s->current_tx_data_type = T38_DATA_V21;
        s->timed_step = (use_hdlc)  ?  T38_TIMED_STEP_HDLC_MODEM  :  T38_TIMED_STEP_NON_ECM_MODEM;
        break;
    case T30_MODEM_V27TER_2400:
        s->octets_per_data_packet = MS_PER_TX_CHUNK*2400/(8*1000);
        s->next_tx_indicator = T38_IND_V27TER_2400_TRAINING;
        s->current_tx_data_type = T38_DATA_V27TER_2400;
        s->next_tx_samples = s->samples + ms_to_samples(MS_PER_TX_CHUNK);
        s->timed_step = (use_hdlc)  ?  T38_TIMED_STEP_HDLC_MODEM  :  T38_TIMED_STEP_NON_ECM_MODEM;
        break;
    case T30_MODEM_V27TER_4800:
        s->octets_per_data_packet = MS_PER_TX_CHUNK*4800/(8*1000);
        s->next_tx_indicator = T38_IND_V27TER_4800_TRAINING;
        s->current_tx_data_type = T38_DATA_V27TER_4800;
        s->next_tx_samples = s->samples + ms_to_samples(MS_PER_TX_CHUNK);
        s->timed_step = (use_hdlc)  ?  T38_TIMED_STEP_HDLC_MODEM  :  T38_TIMED_STEP_NON_ECM_MODEM;
        break;
    case T30_MODEM_V29_7200:
        s->octets_per_data_packet = MS_PER_TX_CHUNK*7200/(8*1000);
        s->next_tx_indicator = T38_IND_V29_7200_TRAINING;
        s->current_tx_data_type = T38_DATA_V29_7200;
        s->next_tx_samples = s->samples + ms_to_samples(MS_PER_TX_CHUNK);
        s->timed_step = (use_hdlc)  ?  T38_TIMED_STEP_HDLC_MODEM  :  T38_TIMED_STEP_NON_ECM_MODEM;
        break;
    case T30_MODEM_V29_9600:
        s->octets_per_data_packet = MS_PER_TX_CHUNK*9600/(8*1000);
        s->next_tx_indicator = T38_IND_V29_9600_TRAINING;
        s->current_tx_data_type = T38_DATA_V29_9600;
        s->next_tx_samples = s->samples + ms_to_samples(MS_PER_TX_CHUNK);
        s->timed_step = (use_hdlc)  ?  T38_TIMED_STEP_HDLC_MODEM  :  T38_TIMED_STEP_NON_ECM_MODEM;
        break;
    case T30_MODEM_V17_7200:
        s->octets_per_data_packet = MS_PER_TX_CHUNK*7200/(8*1000);
        s->next_tx_indicator = (short_train)  ?  T38_IND_V17_7200_SHORT_TRAINING  :  T38_IND_V17_7200_LONG_TRAINING;
        s->current_tx_data_type = T38_DATA_V17_7200;
        s->next_tx_samples = s->samples + ms_to_samples(MS_PER_TX_CHUNK);
        s->timed_step = (use_hdlc)  ?  T38_TIMED_STEP_HDLC_MODEM  :  T38_TIMED_STEP_NON_ECM_MODEM;
        break;
    case T30_MODEM_V17_9600:
        s->octets_per_data_packet = MS_PER_TX_CHUNK*9600/(8*1000);
        s->next_tx_indicator = (short_train)  ?  T38_IND_V17_9600_SHORT_TRAINING  :  T38_IND_V17_9600_LONG_TRAINING;
        s->current_tx_data_type = T38_DATA_V17_9600;
        s->next_tx_samples = s->samples + ms_to_samples(MS_PER_TX_CHUNK);
        s->timed_step = (use_hdlc)  ?  T38_TIMED_STEP_HDLC_MODEM  :  T38_TIMED_STEP_NON_ECM_MODEM;
        break;
    case T30_MODEM_V17_12000:
        s->octets_per_data_packet = MS_PER_TX_CHUNK*12000/(8*1000);
        s->next_tx_indicator = (short_train)  ?  T38_IND_V17_12000_SHORT_TRAINING  :  T38_IND_V17_12000_LONG_TRAINING;
        s->current_tx_data_type = T38_DATA_V17_12000;
        s->next_tx_samples = s->samples + ms_to_samples(MS_PER_TX_CHUNK);
        s->timed_step = (use_hdlc)  ?  T38_TIMED_STEP_HDLC_MODEM  :  T38_TIMED_STEP_NON_ECM_MODEM;
        break;
    case T30_MODEM_V17_14400:
        s->octets_per_data_packet = MS_PER_TX_CHUNK*14400/(8*1000);
        s->next_tx_indicator = (short_train)  ?  T38_IND_V17_14400_SHORT_TRAINING  :  T38_IND_V17_14400_LONG_TRAINING;
        s->current_tx_data_type = T38_DATA_V17_14400;
        s->next_tx_samples = s->samples + ms_to_samples(MS_PER_TX_CHUNK);
        s->timed_step = (use_hdlc)  ?  T38_TIMED_STEP_HDLC_MODEM  :  T38_TIMED_STEP_NON_ECM_MODEM;
        break;
    case T30_MODEM_DONE:
        span_log(&s->logging, SPAN_LOG_FLOW, "FAX exchange complete\n");
        s->timed_step = T38_TIMED_STEP_NONE;
        s->current_tx_data_type = T38_DATA_NONE;
        break;
    }
    s->current_tx_type = type;
}
/*- End of function --------------------------------------------------------*/

t38_terminal_state_t *t38_terminal_init(t38_terminal_state_t *s,
                                        int calling_party,
                                        t38_tx_packet_handler_t *tx_packet_handler,
                                        void *tx_packet_user_data)
{
    if (tx_packet_handler == NULL)
        return NULL;

    memset(s, 0, sizeof(*s));
    span_log_init(&s->logging, SPAN_LOG_NONE, NULL);
    span_log_set_protocol(&s->logging, "T.38T");
    s->rx_signal_present = FALSE;

    s->timed_step = T38_TIMED_STEP_NONE;
    s->hdlc_tx_ptr = 0;

    t38_core_init(&s->t38, process_rx_indicator, process_rx_data, process_rx_missing, (void *) s);
    s->t38.iaf = TRUE;
    s->t38.tx_packet_handler = tx_packet_handler;
    s->t38.tx_packet_user_data = tx_packet_user_data;
    s->t38.fastest_image_data_rate = 14400;

    s->timed_step = T38_TIMED_STEP_NONE;
    s->current_tx_data_type = T38_DATA_NONE;
    s->next_tx_samples = 0;

    t30_init(&(s->t30_state),
             calling_party,
             set_rx_type,
             (void *) s,
             set_tx_type,
             (void *) s,
             send_hdlc,
             (void *) s);
    t30_set_supported_modems(&(s->t30_state),
                             T30_SUPPORT_V27TER | T30_SUPPORT_V29 | T30_SUPPORT_V17);
    t30_restart(&s->t30_state);
    return s;
}
/*- End of function --------------------------------------------------------*/
/*- End of file ------------------------------------------------------------*/
