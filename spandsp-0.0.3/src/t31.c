/*
 * SpanDSP - a series of DSP components for telephony
 *
 * t31.c - A T.31 compatible class 1 FAX modem interface.
 *
 * Written by Steve Underwood <steveu@coppice.org>
 *
 * Special thanks to Lee Howard <faxguy@howardsilvan.com>
 * for his great work debugging and polishing this code.
 *
 * Copyright (C) 2004, 2005, 2006 Steve Underwood
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
 * $Id: t31.c,v 1.83 2006/11/30 15:41:47 steveu Exp $
 */

/*! \file */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#define _GNU_SOURCE

#include <inttypes.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <time.h>
#include <memory.h>
#include <string.h>
#include <ctype.h>
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
#include "spandsp/dc_restore.h"
#include "spandsp/queue.h"
#include "spandsp/power_meter.h"
#include "spandsp/complex.h"
#include "spandsp/tone_generate.h"
#include "spandsp/async.h"
#include "spandsp/hdlc.h"
#include "spandsp/silence_gen.h"
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
#include "spandsp/t30.h"
#include "spandsp/t38_core.h"

#include "spandsp/at_interpreter.h"
#include "spandsp/t31.h"

#define MS_PER_TX_CHUNK             30
#define INDICATOR_TX_COUNT          4
#define DEFAULT_DTE_TIMEOUT         5

typedef const char *(*at_cmd_service_t)(t31_state_t *s, const char *cmd);

#define ETX 0x03
#define DLE 0x10
#define SUB 0x1A

enum
{
    T31_FLUSH,
    T31_SILENCE_TX,
    T31_SILENCE_RX,
    T31_CED_TONE,
    T31_CNG_TONE,
    T31_NOCNG_TONE,
    T31_V21_TX,
    T31_V17_TX,
    T31_V27TER_TX,
    T31_V29_TX,
    T31_V21_RX,
    T31_V17_RX,
    T31_V27TER_RX,
    T31_V29_RX
};

enum
{
    T38_TIMED_STEP_NONE = 0,
    T38_TIMED_STEP_NON_ECM_MODEM,
    T38_TIMED_STEP_NON_ECM_MODEM_2,
    T38_TIMED_STEP_NON_ECM_MODEM_3,
    T38_TIMED_STEP_HDLC_MODEM,
    T38_TIMED_STEP_HDLC_MODEM_2,
    T38_TIMED_STEP_HDLC_MODEM_3,
    T38_TIMED_STEP_HDLC_MODEM_4,
    T38_TIMED_STEP_PAUSE
};

static int restart_modem(t31_state_t *s, int new_modem);
static void hdlc_accept(void *user_data, int ok, const uint8_t *msg, int len);
#if defined(ENABLE_V17)
static int early_v17_rx(void *user_data, const int16_t amp[], int len);
#endif
static int early_v27ter_rx(void *user_data, const int16_t amp[], int len);
static int early_v29_rx(void *user_data, const int16_t amp[], int len);
static int dummy_rx(void *s, const int16_t amp[], int len);
static int silence_rx(void *user_data, const int16_t amp[], int len);
static int cng_rx(void *user_data, const int16_t amp[], int len);

static __inline__ void t31_set_at_rx_mode(t31_state_t *s, int new_mode)
{
    s->at_state.at_rx_mode = new_mode;
}
/*- End of function --------------------------------------------------------*/

static int process_rx_indicator(t38_core_state_t *s, void *user_data, int indicator)
{
    t31_state_t *t;
    
    t = (t31_state_t *) user_data;
    switch (indicator)
    {
    case T38_IND_NO_SIGNAL:
        break;
    case T38_IND_CNG:
        break;
    case T38_IND_CED:
        break;
    case T38_IND_V21_PREAMBLE:
        t->hdlc_rx_len = 0;
        break;
    case T38_IND_V27TER_2400_TRAINING:
        break;
    case T38_IND_V27TER_4800_TRAINING:
        break;
    case T38_IND_V29_7200_TRAINING:
        break;
    case T38_IND_V29_9600_TRAINING:
        break;
    case T38_IND_V17_7200_SHORT_TRAINING:
        break;
    case T38_IND_V17_7200_LONG_TRAINING:
        break;
    case T38_IND_V17_9600_SHORT_TRAINING:
        break;
    case T38_IND_V17_9600_LONG_TRAINING:
        break;
    case T38_IND_V17_12000_SHORT_TRAINING:
        break;
    case T38_IND_V17_12000_LONG_TRAINING:
        break;
    case T38_IND_V17_14400_SHORT_TRAINING:
        break;
    case T38_IND_V17_14400_LONG_TRAINING:
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
        break;
    case T38_IND_V33_14400_TRAINING:
        break;
    default:
        break;
    }
    t->missing_data = FALSE;
    return 0;
}
/*- End of function --------------------------------------------------------*/

static int process_rx_data(t38_core_state_t *s, void *user_data, int data_type, int field_type, const uint8_t *buf, int len)
{
    t31_state_t *t;
    int i;
    
    t = (t31_state_t *) user_data;
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
    switch (field_type)
    {
    case T38_FIELD_HDLC_DATA:
        if (t->hdlc_rx_len + len <= 256 - 2)
        {
            for (i = 0;  i < len;  i++)
                t->hdlc_rx_buf[t->hdlc_rx_len++] = bit_reverse8(buf[i]);
        }
        break;
    case T38_FIELD_HDLC_FCS_OK:
        if (len > 0)
            span_log(&t->logging, SPAN_LOG_WARNING, "There is data in a T38_FIELD_HDLC_FCS_OK!\n");
        span_log(&t->logging, SPAN_LOG_FLOW, "Type %s - CRC OK (%s)\n", t30_frametype(t->tx_data[2]), (t->missing_data)  ?  "missing octets"  :  "clean");
        /* Don't deal with zero length frames. Some T.38 implementations send multiple T38_FIELD_HDLC_FCS_OK
           packets, when they have sent no data for the body of the frame. */
        if (t->current_rx_type == T31_V21_RX  &&  t->tx_out_bytes > 0  &&  !t->missing_data)
            hdlc_accept((void *) t, TRUE, t->hdlc_rx_buf, t->hdlc_rx_len);
        t->hdlc_rx_len = 0;
        t->missing_data = FALSE;
        break;
    case T38_FIELD_HDLC_FCS_BAD:
        if (len > 0)
            span_log(&t->logging, SPAN_LOG_WARNING, "There is data in a T38_FIELD_HDLC_FCS_BAD!\n");
        span_log(&t->logging, SPAN_LOG_FLOW, "Type %s - CRC bad (%s)\n", t30_frametype(t->tx_data[2]), (t->missing_data)  ?  "missing octets"  :  "clean");
        t->hdlc_rx_len = 0;
        t->missing_data = FALSE;
        break;
    case T38_FIELD_HDLC_FCS_OK_SIG_END:
        if (len > 0)
            span_log(&t->logging, SPAN_LOG_WARNING, "There is data in a T38_FIELD_HDLC_FCS_OK_SIG_END!\n");
        span_log(&t->logging, SPAN_LOG_FLOW, "Type %s - CRC OK, sig end (%s)\n", t30_frametype(t->tx_data[2]), (t->missing_data)  ?  "missing octets"  :  "clean");
        if (t->current_rx_type == T31_V21_RX)
        {
            /* Don't deal with zero length frames. Some T.38 implementations send multiple T38_FIELD_HDLC_FCS_OK
               packets, when they have sent no data for the body of the frame. */
            if (t->tx_out_bytes > 0)
                hdlc_accept((void *) t, TRUE, t->hdlc_rx_buf, t->hdlc_rx_len);
            hdlc_accept((void *) t, TRUE, NULL, PUTBIT_CARRIER_DOWN);
        }
        t->tx_out_bytes = 0;
        t->missing_data = FALSE;
        t->hdlc_rx_len = 0;
        t->missing_data = FALSE;
        break;
    case T38_FIELD_HDLC_FCS_BAD_SIG_END:
        if (len > 0)
            span_log(&t->logging, SPAN_LOG_WARNING, "There is data in a T38_FIELD_HDLC_FCS_BAD_SIG_END!\n");
        span_log(&t->logging, SPAN_LOG_FLOW, "Type %s - CRC bad, sig end (%s)\n", t30_frametype(t->tx_data[2]), (t->missing_data)  ?  "missing octets"  :  "clean");
        if (t->current_rx_type == T31_V21_RX)
            hdlc_accept((void *) t, TRUE, NULL, PUTBIT_CARRIER_DOWN);
        t->hdlc_rx_len = 0;
        t->missing_data = FALSE;
        break;
    case T38_FIELD_HDLC_SIG_END:
        if (len > 0)
            span_log(&t->logging, SPAN_LOG_WARNING, "There is data in a T38_FIELD_HDLC_SIG_END!\n");
        /* This message is expected under 2 circumstances. One is as an alternative to T38_FIELD_HDLC_FCS_OK_SIG_END - 
           i.e. they send T38_FIELD_HDLC_FCS_OK, and then T38_FIELD_HDLC_SIG_END when the carrier actually drops.
           The other is because the HDLC signal drops unexpectedly - i.e. not just after a final frame. */
        if (t->current_rx_type == T31_V21_RX)
            hdlc_accept((void *) t, TRUE, NULL, PUTBIT_CARRIER_DOWN);
        t->hdlc_rx_len = 0;
        t->missing_data = FALSE;
        break;
    case T38_FIELD_T4_NON_ECM_DATA:
        break;
    case T38_FIELD_T4_NON_ECM_SIG_END:
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

static int process_rx_missing(t38_core_state_t *s, void *user_data, int rx_seq_no, int expected_seq_no)
{
    t31_state_t *t;
    
    t = (t31_state_t *) user_data;
    t->missing_data = TRUE;
    return 0;
}
/*- End of function --------------------------------------------------------*/

int t31_t38_send_timeout(t31_state_t *s, int samples)
{
    int len;
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

    s->call_samples += samples;
        
    if (s->timed_step == T38_TIMED_STEP_NONE)
        return 0;
    if (s->call_samples < s->next_send_samples)
        return 0;
    len = 0;
    switch (s->timed_step)
    {
    case T38_TIMED_STEP_NON_ECM_MODEM:
        /* Create a 75ms silence */
        t38_core_send_indicator(&s->t38, T38_IND_NO_SIGNAL, INDICATOR_TX_COUNT);
        s->timed_step = T38_TIMED_STEP_NON_ECM_MODEM_2;
        s->next_send_samples += ms_to_samples(75);
        break;
    case T38_TIMED_STEP_NON_ECM_MODEM_2:
        /* Switch on a fast modem, and give the training time to complete */
        t38_core_send_indicator(&s->t38, s->next_tx_indicator, INDICATOR_TX_COUNT);
        s->timed_step = T38_TIMED_STEP_NON_ECM_MODEM_3;
        s->next_send_samples += ms_to_samples(training_time[s->next_tx_indicator << 1]);
        break;
    case T38_TIMED_STEP_NON_ECM_MODEM_3:
        /* Send a chunk of non-ECM image data */
        //if ((len = get_non_ecm_image_chunk(s, buf, s->octets_per_non_ecm_packet)))
        //    t38_core_send_data(&s->t38, s->current_tx_data, T38_FIELD_T4_NON_ECM_DATA, buf, abs(len));
        if (len > 0)
        {
            s->next_send_samples += ms_to_samples(MS_PER_TX_CHUNK);
        }
        else
        {
            t38_core_send_data(&s->t38, s->current_tx_data, T38_FIELD_T4_NON_ECM_SIG_END, NULL, 0);
            t38_core_send_indicator(&s->t38, T38_IND_NO_SIGNAL, INDICATOR_TX_COUNT);
            s->timed_step = T38_TIMED_STEP_NONE;
        }
        break;
    case T38_TIMED_STEP_HDLC_MODEM:
        /* Send HDLC preambling */
        t38_core_send_indicator(&s->t38, s->next_tx_indicator, INDICATOR_TX_COUNT);
        s->next_send_samples += ms_to_samples(training_time[s->next_tx_indicator << 1]);
        s->timed_step = T38_TIMED_STEP_HDLC_MODEM_2;
        break;
    case T38_TIMED_STEP_HDLC_MODEM_2:
        /* Send HDLC octet */
        buf[0] = bit_reverse8(s->hdlc_tx_buf[s->hdlc_tx_ptr++]);
        t38_core_send_data(&s->t38, s->current_tx_data, T38_FIELD_HDLC_DATA, buf, 1);
        if (s->hdlc_tx_ptr >= s->hdlc_tx_len)
            s->timed_step = T38_TIMED_STEP_HDLC_MODEM_3;
        s->next_send_samples += ms_to_samples(MS_PER_TX_CHUNK);
        break;
    case T38_TIMED_STEP_HDLC_MODEM_3:
        /* End of HDLC frame */
        s->hdlc_tx_ptr = 0;
        if (s->hdlc_final)
        {
            s->hdlc_tx_len = 0;
            t38_core_send_data(&s->t38, s->current_tx_data, T38_FIELD_HDLC_FCS_OK_SIG_END, NULL, 0);
            s->timed_step = T38_TIMED_STEP_HDLC_MODEM_4;
            at_put_response_code(&s->at_state, AT_RESPONSE_CODE_OK);
            t31_set_at_rx_mode(s, AT_MODE_OFFHOOK_COMMAND);
            s->hdlc_final = FALSE;
            restart_modem(s, T31_SILENCE_TX);
        }
        else
        {
            t38_core_send_data(&s->t38, s->current_tx_data, T38_FIELD_HDLC_FCS_OK, NULL, 0);
            s->timed_step = T38_TIMED_STEP_HDLC_MODEM_2;
            at_put_response_code(&s->at_state, AT_RESPONSE_CODE_CONNECT);
        }
        s->next_send_samples += ms_to_samples(MS_PER_TX_CHUNK);
        break;
    case T38_TIMED_STEP_HDLC_MODEM_4:
        /* End of HDLC transmission */
        /* We have already sent T38_FIELD_HDLC_FCS_OK_SIG_END. It seems some boxes may not like
           us sending a T38_FIELD_HDLC_SIG_END at this point. Just say there is no signal. */
        //t38_core_send_data(&s->t38, s->current_tx_data, T38_FIELD_HDLC_SIG_END, NULL, 0);
        t38_core_send_indicator(&s->t38, T38_IND_NO_SIGNAL, INDICATOR_TX_COUNT);
        s->timed_step = T38_TIMED_STEP_NONE;
        break;
    case T38_TIMED_STEP_PAUSE:
        /* End of timed pause */
        s->timed_step = T38_TIMED_STEP_NONE;
        break;
    }
    return 0;
}
/*- End of function --------------------------------------------------------*/

static int t31_modem_control_handler(at_state_t *s, void *user_data, int op, const char *num)
{
    t31_state_t *t;
    
    t = (t31_state_t *) user_data;
    switch (op)
    {
    case AT_MODEM_CONTROL_ANSWER:
        t->call_samples = 0;
        break;
    case AT_MODEM_CONTROL_CALL:
        t->call_samples = 0;
        break;
    case AT_MODEM_CONTROL_ONHOOK:
        if (t->tx_holding)
        {
            t->tx_holding = FALSE;
            /* Tell the application to release further data */
            at_modem_control(&t->at_state, AT_MODEM_CONTROL_CTS, (void *) 1);
        }
        if (t->at_state.rx_signal_present)
        {
            t->at_state.rx_data[t->at_state.rx_data_bytes++] = DLE;
            t->at_state.rx_data[t->at_state.rx_data_bytes++] = ETX;
            t->at_state.at_tx_handler(&t->at_state, t->at_state.at_tx_user_data, t->at_state.rx_data, t->at_state.rx_data_bytes);
            t->at_state.rx_data_bytes = 0;
        }
        restart_modem(t, T31_SILENCE_TX);
        break;
    case AT_MODEM_CONTROL_RESTART:
        restart_modem(t, (int) (intptr_t) num);
        return 0;
    case AT_MODEM_CONTROL_DTE_TIMEOUT:
        if (num)
            t->dte_data_timeout = t->call_samples + ms_to_samples((intptr_t) num);
        else
            t->dte_data_timeout = 0;
        return 0;
    }
    return t->modem_control_handler(t, t->modem_control_user_data, op, num);
}
/*- End of function --------------------------------------------------------*/

static void non_ecm_put_bit(void *user_data, int bit)
{
    t31_state_t *s;

    s = (t31_state_t *) user_data;
    if (bit < 0)
    {
        /* Special conditions */
        switch (bit)
        {
        case PUTBIT_TRAINING_FAILED:
            s->at_state.rx_trained = FALSE;
            break;
        case PUTBIT_TRAINING_SUCCEEDED:
            /* The modem is now trained */
            at_put_response_code(&s->at_state, AT_RESPONSE_CODE_CONNECT);
            s->at_state.rx_signal_present = TRUE;
            s->at_state.rx_trained = TRUE;
            break;
        case PUTBIT_CARRIER_UP:
            break;
        case PUTBIT_CARRIER_DOWN:
            if (s->at_state.rx_signal_present)
            {
                s->at_state.rx_data[s->at_state.rx_data_bytes++] = DLE;
                s->at_state.rx_data[s->at_state.rx_data_bytes++] = ETX;
                s->at_state.at_tx_handler(&s->at_state, s->at_state.at_tx_user_data, s->at_state.rx_data, s->at_state.rx_data_bytes);
                s->at_state.rx_data_bytes = 0;
                at_put_response_code(&s->at_state, AT_RESPONSE_CODE_NO_CARRIER);
                t31_set_at_rx_mode(s, AT_MODE_OFFHOOK_COMMAND);
            }
            s->at_state.rx_signal_present = FALSE;
            s->at_state.rx_trained = FALSE;
            break;
        default:
            if (s->at_state.p.result_code_format)
                span_log(&s->logging, SPAN_LOG_FLOW, "Eh!\n");
            break;
        }
        return;
    }
    s->current_byte = (s->current_byte >> 1) | (bit << 7);
    if (++s->bit_no >= 8)
    {
        if (s->current_byte == DLE)
            s->at_state.rx_data[s->at_state.rx_data_bytes++] = (uint8_t) s->current_byte;
        s->at_state.rx_data[s->at_state.rx_data_bytes++] = (uint8_t) s->current_byte;
        if (s->at_state.rx_data_bytes >= 250)
        {
            s->at_state.at_tx_handler(&s->at_state, s->at_state.at_tx_user_data, s->at_state.rx_data, s->at_state.rx_data_bytes);
            s->at_state.rx_data_bytes = 0;
        }
        s->bit_no = 0;
        s->current_byte = 0;
    }
}
/*- End of function --------------------------------------------------------*/

static int non_ecm_get_bit(void *user_data)
{
    t31_state_t *s;
    int bit;
    int fill;

    s = (t31_state_t *) user_data;
    if (s->bit_no <= 0)
    {
        if (s->tx_out_bytes != s->tx_in_bytes)
        {
            /* There is real data available to send */
            s->current_byte = s->tx_data[s->tx_out_bytes];
            s->tx_out_bytes = (s->tx_out_bytes + 1) & (T31_TX_BUF_LEN - 1);
            if (s->tx_holding)
            {
                /* See if the buffer is approaching empty. It might be time to release flow control. */
                fill = (s->tx_in_bytes - s->tx_out_bytes);
                if (s->tx_in_bytes < s->tx_out_bytes)
                    fill += (T31_TX_BUF_LEN + 1);
                if (fill < T31_TX_BUF_LOW_TIDE)
                {
                    s->tx_holding = FALSE;
                    /* Tell the application to release further data */
                    at_modem_control(&s->at_state, AT_MODEM_CONTROL_CTS, (void *) 1);
                }
            }
            s->tx_data_started = TRUE;
        }
        else
        {
            if (s->data_final)
            {
                s->data_final = FALSE;
                /* This will put the modem into its shutdown sequence. When
                   it has finally shut down, an OK response will be sent. */
                return PUTBIT_END_OF_DATA;
            }
            /* Fill with 0xFF bytes at the start of transmission, or 0x00 if we are in
               the middle of transmission. This follows T.31 and T.30 practice. */
            s->current_byte = (s->tx_data_started)  ?  0x00  :  0xFF;
        }
        s->bit_no = 8;
    }
    s->bit_no--;
    bit = s->current_byte & 1;
    s->current_byte >>= 1;
    return bit;
}
/*- End of function --------------------------------------------------------*/

static void hdlc_tx_underflow(void *user_data)
{
    t31_state_t *s;

    s = (t31_state_t *) user_data;
    if (s->hdlc_final)
    {
        s->hdlc_final = FALSE;
        /* Schedule an orderly shutdown of the modem */
        hdlc_tx_frame(&(s->hdlctx), NULL, 0);
    }
    else
    {
        at_put_response_code(&s->at_state, AT_RESPONSE_CODE_CONNECT);
    }
}
/*- End of function --------------------------------------------------------*/

static void hdlc_accept(void *user_data, int ok, const uint8_t *msg, int len)
{
    uint8_t buf[256];
    t31_state_t *s;
    int i;

    s = (t31_state_t *) user_data;
    if (len < 0)
    {
        /* Special conditions */
        switch (len)
        {
        case PUTBIT_TRAINING_FAILED:
            s->at_state.rx_trained = FALSE;
            break;
        case PUTBIT_TRAINING_SUCCEEDED:
            /* The modem is now trained */
            s->at_state.rx_signal_present = TRUE;
            s->at_state.rx_trained = TRUE;
            break;
        case PUTBIT_CARRIER_UP:
            if (s->modem == T31_CNG_TONE  ||  s->modem == T31_NOCNG_TONE  ||  s->modem == T31_V21_RX)
            {
                s->at_state.rx_signal_present = TRUE;
                s->rx_message_received = FALSE;
            }
            break;
        case PUTBIT_CARRIER_DOWN:
            if (s->rx_message_received)
            {
                if (s->at_state.dte_is_waiting)
                {
                    if (s->at_state.ok_is_pending)
                    {
                        at_put_response_code(&s->at_state, AT_RESPONSE_CODE_OK);
                        s->at_state.ok_is_pending = FALSE;
                    }
                    else
                    {
                        at_put_response_code(&s->at_state, AT_RESPONSE_CODE_NO_CARRIER);
                        t31_set_at_rx_mode(s, AT_MODE_OFFHOOK_COMMAND);
                    }
                    s->at_state.dte_is_waiting = FALSE;
                }
                else
                {
                    buf[0] = AT_RESPONSE_CODE_NO_CARRIER;
                    queue_write_msg(&(s->rx_queue), buf, 1);
                }
            }
            s->at_state.rx_signal_present = FALSE;
            s->at_state.rx_trained = FALSE;
            break;
        case PUTBIT_FRAMING_OK:
            if (s->modem == T31_CNG_TONE  ||  s->modem == T31_NOCNG_TONE)
            {
                /* Once we get any valid HDLC the CNG tone stops, and we drop
                   to the V.21 receive modem on its own. */
                s->modem = T31_V21_RX;
                s->at_state.transmit = FALSE;
            }
            if (s->modem != T31_V21_RX)
            {
                /* V.21 has been detected while expecting a different carrier.
                   If +FAR=0 then result +FCERROR and return to command-mode.
                   If +FAR=1 then report +FRH:3 and CONNECT, switching to
                   V.21 receive mode. */
                if (s->at_state.p.adaptive_receive)
                {
                    s->at_state.rx_signal_present = TRUE;
                    s->rx_message_received = TRUE;
                    s->modem = T31_V21_RX;
                    s->at_state.transmit = FALSE;
                    s->at_state.dte_is_waiting = TRUE;
                    at_put_response_code(&s->at_state, AT_RESPONSE_CODE_FRH3);
                    at_put_response_code(&s->at_state, AT_RESPONSE_CODE_CONNECT);
                }
                else
                {
                    s->modem = T31_SILENCE_TX;
                    t31_set_at_rx_mode(s, AT_MODE_OFFHOOK_COMMAND);
                    s->rx_message_received = FALSE;
                    at_put_response_code(&s->at_state, AT_RESPONSE_CODE_FCERROR);
                }
            }
            else
            {
                if (!s->rx_message_received)
                {
                    /* Report CONNECT as soon as possible to avoid a timeout. */
                    at_put_response_code(&s->at_state, AT_RESPONSE_CODE_CONNECT);
                    s->rx_message_received = TRUE;
                }
            }
            break;
        case PUTBIT_ABORT:
            /* Just ignore these */
            break;
        default:
            span_log(&s->logging, SPAN_LOG_WARNING, "Unexpected HDLC special length - %d!\n", len);
            break;
        }
        return;
    }
    /* If OK is pending then we just ignore whatever comes in */
    if (!s->at_state.ok_is_pending)
    {
        if (s->at_state.dte_is_waiting)
        {
            /* Send straight away */
            /* It is safe to look at the two bytes beyond the length of the message,
               and expect to find the FCS there. */
            for (i = 0;  i < len + 2;  i++)
            {
                if (msg[i] == DLE)
                    s->at_state.rx_data[s->at_state.rx_data_bytes++] = DLE;
                s->at_state.rx_data[s->at_state.rx_data_bytes++] = msg[i];
            }
            s->at_state.rx_data[s->at_state.rx_data_bytes++] = DLE;
            s->at_state.rx_data[s->at_state.rx_data_bytes++] = ETX;
            s->at_state.at_tx_handler(&s->at_state, s->at_state.at_tx_user_data, s->at_state.rx_data, s->at_state.rx_data_bytes);
            s->at_state.rx_data_bytes = 0;
            if (msg[1] == 0x13  &&  ok)
            {
                /* This is the last frame.  We don't send OK until the carrier drops to avoid
                   redetecting it later. */
                s->at_state.ok_is_pending = TRUE;
            }
            else
            {
                at_put_response_code(&s->at_state, (ok)  ?  AT_RESPONSE_CODE_OK  :  AT_RESPONSE_CODE_ERROR);
                s->at_state.dte_is_waiting = FALSE;
                s->rx_message_received = FALSE;
            }
        }
        else
        {
            /* Queue it */
            buf[0] = (ok)  ?  AT_RESPONSE_CODE_OK  :  AT_RESPONSE_CODE_ERROR;
            /* It is safe to look at the two bytes beyond the length of the message,
               and expect to find the FCS there. */
            memcpy(buf + 1, msg, len + 2);
            queue_write_msg(&(s->rx_queue), buf, len + 3);
        }
    }
    t31_set_at_rx_mode(s, AT_MODE_OFFHOOK_COMMAND);
}
/*- End of function --------------------------------------------------------*/

static void t31_v21_rx(t31_state_t *s)
{
    hdlc_rx_init(&(s->hdlcrx), FALSE, TRUE, 5, hdlc_accept, s);
    s->at_state.ok_is_pending = FALSE;
    s->hdlc_final = FALSE;
    s->hdlc_tx_len = 0;
    s->dled = FALSE;
    fsk_rx_init(&(s->v21rx), &preset_fsk_specs[FSK_V21CH2], TRUE, (put_bit_func_t) hdlc_rx_put_bit, &(s->hdlcrx));
    s->at_state.transmit = TRUE;
}
/*- End of function --------------------------------------------------------*/

static int restart_modem(t31_state_t *s, int new_modem)
{
    tone_gen_descriptor_t tone_desc;
    int ind;

    span_log(&s->logging, SPAN_LOG_FLOW, "Restart modem %d\n", new_modem);
    if (s->modem == new_modem)
        return 0;
    queue_flush(&(s->rx_queue));
    s->modem = new_modem;
    s->data_final = FALSE;
    s->at_state.rx_signal_present = FALSE;
    s->at_state.rx_trained = FALSE;
    s->rx_message_received = FALSE;
    s->rx_handler = (span_rx_handler_t *) &dummy_rx;
    s->rx_user_data = NULL;
    switch (s->modem)
    {
    case T31_CNG_TONE:
        if (s->t38_mode)
        {
            t38_core_send_indicator(&s->t38, T38_IND_CNG, INDICATOR_TX_COUNT);
        }
        else
        {
            /* CNG is special, since we need to receive V.21 HDLC messages while sending the
               tone. Everything else in FAX processing sends only one way at a time. */
            /* 0.5s of 1100Hz + 3.0s of silence repeating */
            make_tone_gen_descriptor(&tone_desc,
                                     1100,
                                     -11,
                                     0,
                                     0,
                                     500,
                                     3000,
                                     0,
                                     0,
                                     TRUE);
            tone_gen_init(&(s->tone_gen), &tone_desc);
            /* Do V.21/HDLC receive in parallel. The other end may send its
               first message at any time. The CNG tone will continue until
               we get a valid preamble. */
            s->rx_handler = (span_rx_handler_t *) &cng_rx;
            s->rx_user_data = s;
            t31_v21_rx(s);
            s->tx_handler = (span_tx_handler_t *) &tone_gen;
            s->tx_user_data = &(s->tone_gen);
            s->next_tx_handler = NULL;
        }
        s->at_state.transmit = TRUE;
        break;
    case T31_NOCNG_TONE:
        if (s->t38_mode)
        {
        }
        else
        {
            s->rx_handler = (span_rx_handler_t *) &cng_rx;
            s->rx_user_data = s;
            t31_v21_rx(s);
            silence_gen_set(&(s->silence_gen), 0);
            s->tx_handler = (span_tx_handler_t *) &silence_gen;
            s->tx_user_data = &(s->silence_gen);
        }
        s->at_state.transmit = FALSE;
        break;
    case T31_CED_TONE:
        if (s->t38_mode)
        {
            t38_core_send_indicator(&s->t38, T38_IND_CED, INDICATOR_TX_COUNT);
        }
        else
        {
            silence_gen_alter(&(s->silence_gen), ms_to_samples(200));
            make_tone_gen_descriptor(&tone_desc,
                                     2100,
                                     -11,
                                     0,
                                     0,
                                     2600,
                                     75,
                                     0,
                                     0,
                                     FALSE);
            tone_gen_init(&(s->tone_gen), &tone_desc);
            s->tx_handler = (span_tx_handler_t *) &silence_gen;
            s->tx_user_data = &(s->silence_gen);
            s->next_tx_handler = (span_tx_handler_t *) &tone_gen;
            s->next_tx_user_data = &(s->tone_gen);
        }
        s->at_state.transmit = TRUE;
        break;
    case T31_V21_TX:
        if (s->t38_mode)
        {
            t38_core_send_indicator(&s->t38, T38_IND_V21_PREAMBLE, INDICATOR_TX_COUNT);
        }
        else
        {
            hdlc_tx_init(&(s->hdlctx), FALSE, 2, FALSE, hdlc_tx_underflow, s);
            /* The spec says 1s +-15% of preamble. So, the minimum is 32 octets. */
            hdlc_tx_preamble(&(s->hdlctx), 32);
            fsk_tx_init(&(s->v21tx), &preset_fsk_specs[FSK_V21CH2], (get_bit_func_t) hdlc_tx_get_bit, &(s->hdlctx));
            s->tx_handler = (span_tx_handler_t *) &fsk_tx;
            s->tx_user_data = &(s->v21tx);
            s->next_tx_handler = NULL;
        }
        s->hdlc_final = FALSE;
        s->hdlc_tx_len = 0;
        s->dled = FALSE;
        s->at_state.transmit = TRUE;
        break;
    case T31_V21_RX:
        if (s->t38_mode)
        {
        }
        else
        {
            s->rx_handler = (span_rx_handler_t *) &fsk_rx;
            s->rx_user_data = &(s->v21rx);
            t31_v21_rx(s);
        }
        break;
#if defined(ENABLE_V17)
    case T31_V17_TX:
        if (s->t38_mode)
        {
            switch (s->bit_rate)
            {
            case 7200:
                ind = (s->short_train)  ?  T38_IND_V17_7200_SHORT_TRAINING  :  T38_IND_V17_7200_LONG_TRAINING;
                break;
            case 9600:
                ind = (s->short_train)  ?  T38_IND_V17_9600_SHORT_TRAINING  :  T38_IND_V17_9600_LONG_TRAINING;
                break;
            case 12000:
                ind = (s->short_train)  ?  T38_IND_V17_12000_SHORT_TRAINING  :  T38_IND_V17_12000_LONG_TRAINING;
                break;
            case 14400:
            default:
                ind = (s->short_train)  ?  T38_IND_V17_14400_SHORT_TRAINING  :  T38_IND_V17_14400_LONG_TRAINING;
                break;
            }
            t38_core_send_indicator(&s->t38, ind, INDICATOR_TX_COUNT);
        }
        else
        {
            v17_tx_restart(&(s->v17tx), s->bit_rate, FALSE, s->short_train);
            s->tx_handler = (span_tx_handler_t *) &v17_tx;
            s->tx_user_data = &(s->v17tx);
            s->next_tx_handler = NULL;
        }
        s->tx_out_bytes = 0;
        s->tx_data_started = FALSE;
        s->at_state.transmit = TRUE;
        break;
    case T31_V17_RX:
        if (!s->t38_mode)
        {
            s->rx_handler = (span_rx_handler_t *) &early_v17_rx;
            s->rx_user_data = s;
            v17_rx_restart(&(s->v17rx), s->bit_rate, s->short_train);
            /* Allow for +FCERROR/+FRH:3 */
            t31_v21_rx(s);
        }
        s->at_state.transmit = FALSE;
        break;
#endif
    case T31_V27TER_TX:
        if (s->t38_mode)
        {
            switch (s->bit_rate)
            {
            case 2400:
                ind = T38_IND_V27TER_2400_TRAINING;
                break;
            case 4800:
            default:
                ind = T38_IND_V27TER_4800_TRAINING;
                break;
            }
            t38_core_send_indicator(&s->t38, ind, INDICATOR_TX_COUNT);
        }
        else
        {
            v27ter_tx_restart(&(s->v27ter_tx), s->bit_rate, FALSE);
            s->tx_handler = (span_tx_handler_t *) &v27ter_tx;
            s->tx_user_data = &(s->v27ter_tx);
            s->next_tx_handler = NULL;
        }
        s->tx_out_bytes = 0;
        s->tx_data_started = FALSE;
        s->at_state.transmit = TRUE;
        break;
    case T31_V27TER_RX:
        if (!s->t38_mode)
        {
            s->rx_handler = (span_rx_handler_t *) &early_v27ter_rx;
            s->rx_user_data = s;
            v27ter_rx_restart(&(s->v27ter_rx), s->bit_rate, FALSE);
            /* Allow for +FCERROR/+FRH:3 */
            t31_v21_rx(s);
        }
        s->at_state.transmit = FALSE;
        break;
    case T31_V29_TX:
        if (s->t38_mode)
        {
            switch (s->bit_rate)
            {
            case 7200:
                ind = T38_IND_V29_7200_TRAINING;
                break;
            case 9600:
            default:
                ind = T38_IND_V29_9600_TRAINING;
                break;
            }
            t38_core_send_indicator(&s->t38, ind, INDICATOR_TX_COUNT);
        }
        else
        {
            v29_tx_restart(&(s->v29tx), s->bit_rate, FALSE);
            s->tx_handler = (span_tx_handler_t *) &v29_tx;
            s->tx_user_data = &(s->v29tx);
            s->next_tx_handler = NULL;
        }
        s->tx_out_bytes = 0;
        s->tx_data_started = FALSE;
        s->at_state.transmit = TRUE;
        break;
    case T31_V29_RX:
        if (!s->t38_mode)
        {
            s->rx_handler = (span_rx_handler_t *) &early_v29_rx;
            s->rx_user_data = s;
            v29_rx_restart(&(s->v29rx), s->bit_rate, FALSE);
            /* Allow for +FCERROR/+FRH:3 */
            t31_v21_rx(s);
        }
        s->at_state.transmit = FALSE;
        break;
    case T31_SILENCE_TX:
        if (s->t38_mode)
        {
            t38_core_send_indicator(&s->t38, T38_IND_NO_SIGNAL, INDICATOR_TX_COUNT);
        }
        else
        {
            silence_gen_set(&(s->silence_gen), 0);
            s->tx_handler = (span_tx_handler_t *) &silence_gen;
            s->tx_user_data = &(s->silence_gen);
            s->next_tx_handler = NULL;
        }
        s->at_state.transmit = FALSE;
        break;
    case T31_SILENCE_RX:
        if (!s->t38_mode)
        {
            s->rx_handler = (span_rx_handler_t *) &silence_rx;
            s->rx_user_data = s;

            silence_gen_set(&(s->silence_gen), 0);
            s->tx_handler = (span_tx_handler_t *) &silence_gen;
            s->tx_user_data = &(s->silence_gen);
            s->next_tx_handler = NULL;
        }
        s->at_state.transmit = FALSE;
        break;
    case T31_FLUSH:
        /* Send 200ms of silence to "push" the last audio out */
        if (s->t38_mode)
        {
            t38_core_send_indicator(&s->t38, T38_IND_NO_SIGNAL, INDICATOR_TX_COUNT);
        }
        else
        {
            s->modem = T31_SILENCE_TX;
            silence_gen_alter(&(s->silence_gen), ms_to_samples(200));
            s->tx_handler = (span_tx_handler_t *) &silence_gen;
            s->tx_user_data = &(s->silence_gen);
            s->next_tx_handler = NULL;
            s->at_state.transmit = TRUE;
        }
        break;
    }
    s->bit_no = 0;
    s->current_byte = 0xFF;
    s->tx_in_bytes = 0;
    return 0;
}
/*- End of function --------------------------------------------------------*/

static __inline__ void dle_unstuff_hdlc(t31_state_t *s, const char *stuffed, int len)
{
    int i;

    for (i = 0;  i < len;  i++)
    {
        if (s->dled)
        {
            s->dled = FALSE;
            if (stuffed[i] == ETX)
            {
                if (s->t38_mode)
                {
                }
                else
                {
                    hdlc_tx_frame(&(s->hdlctx), s->hdlc_tx_buf, s->hdlc_tx_len);
                }
                s->hdlc_final = (s->hdlc_tx_buf[1] & 0x10);
                s->hdlc_tx_len = 0;
            }
            else if (stuffed[i] == SUB)
            {
                s->hdlc_tx_buf[s->hdlc_tx_len++] = DLE;
                s->hdlc_tx_buf[s->hdlc_tx_len++] = DLE;
            }
            else
            {
                s->hdlc_tx_buf[s->hdlc_tx_len++] = stuffed[i];
            }
        }
        else
        {
            if (stuffed[i] == DLE)
                s->dled = TRUE;
            else
                s->hdlc_tx_buf[s->hdlc_tx_len++] = stuffed[i];
        }
    }
}
/*- End of function --------------------------------------------------------*/

static __inline__ void dle_unstuff(t31_state_t *s, const char *stuffed, int len)
{
    int i;
    int fill;
    int next;
    
    for (i = 0;  i < len;  i++)
    {
        if (s->dled)
        {
            s->dled = FALSE;
            if (stuffed[i] == ETX)
            {
                s->data_final = TRUE;
                t31_set_at_rx_mode(s, AT_MODE_OFFHOOK_COMMAND);
                return;
            }
        }
        else if (stuffed[i] == DLE)
        {
            s->dled = TRUE;
            continue;
        }
        s->tx_data[s->tx_in_bytes] = stuffed[i];
        next = (s->tx_in_bytes + 1) & (T31_TX_BUF_LEN - 1);
        if (next == s->tx_out_bytes)
        {
            /* Oops. We hit the end of the buffer. Give up. Loose stuff. :-( */
            return;
        }
        s->tx_in_bytes = next;
    }
    if (!s->tx_holding)
    {
        /* See if the buffer is approaching full. We might need to apply flow control. */
        fill = (s->tx_in_bytes - s->tx_out_bytes);
        if (s->tx_in_bytes < s->tx_out_bytes)
            fill += (T31_TX_BUF_LEN + 1);
        if (fill > T31_TX_BUF_HIGH_TIDE)
        {
            s->tx_holding = TRUE;
            /* Tell the application to hold further data */
            at_modem_control(&s->at_state, AT_MODEM_CONTROL_CTS, (void *) 0);
        }
    }
}
/*- End of function --------------------------------------------------------*/

static int process_class1_cmd(at_state_t *t, void *user_data, int direction, int operation, int val)
{
    int new_modem;
    int new_transmit;
    int i;
    int len;
    int immediate_response;
    t31_state_t *s;
    uint8_t msg[256];

    s = (t31_state_t *) user_data;
    new_transmit = direction;
    immediate_response = TRUE;
    switch (operation)
    {
    case 'S':
        s->at_state.transmit = new_transmit;
        if (new_transmit)
        {
            /* Send a specified period of silence, to space transmissions. */
            restart_modem(s, T31_SILENCE_TX);
            silence_gen_alter(&(s->silence_gen), val*80);
            s->at_state.transmit = TRUE;
        }
        else
        {
            /* Wait until we have received a specified period of silence. */
            queue_flush(&(s->rx_queue));
            s->silence_awaited = val*80;
            t31_set_at_rx_mode(s, AT_MODE_DELIVERY);
            restart_modem(s, T31_SILENCE_RX);
        }
        immediate_response = FALSE;
        span_log(&s->logging, SPAN_LOG_FLOW, "Silence %dms\n", val*10);
        break;
    case 'H':
        switch (val)
        {
        case 3:
            new_modem = (new_transmit)  ?  T31_V21_TX  :  T31_V21_RX;
            s->short_train = FALSE;
            s->bit_rate = 300;
            break;
        default:
            return -1;
        }
        span_log(&s->logging, SPAN_LOG_FLOW, "HDLC\n");
        if (new_modem != s->modem)
        {
            restart_modem(s, new_modem);
            immediate_response = FALSE;
        }
        s->at_state.transmit = new_transmit;
        if (new_transmit)
        {
            t31_set_at_rx_mode(s, AT_MODE_HDLC);
            at_put_response_code(&s->at_state, AT_RESPONSE_CODE_CONNECT);
        }
        else
        {
            /* Send straight away, if there is something queued. */
            t31_set_at_rx_mode(s, AT_MODE_DELIVERY);
            s->rx_message_received = FALSE;
            do
            {
                if (!queue_empty(&(s->rx_queue)))
                {
                    len = queue_read_msg(&(s->rx_queue), msg, 256);
                    if (len > 1)
                    {
                        if (msg[0] == AT_RESPONSE_CODE_OK)
                            at_put_response_code(&s->at_state, AT_RESPONSE_CODE_CONNECT);
                        for (i = 1;  i < len;  i++)
                        {
                            if (msg[i] == DLE)
                                s->at_state.rx_data[s->at_state.rx_data_bytes++] = DLE;
                            s->at_state.rx_data[s->at_state.rx_data_bytes++] = msg[i];
                        }
                        s->at_state.rx_data[s->at_state.rx_data_bytes++] = DLE;
                        s->at_state.rx_data[s->at_state.rx_data_bytes++] = ETX;
                        s->at_state.at_tx_handler(&s->at_state, s->at_state.at_tx_user_data, s->at_state.rx_data, s->at_state.rx_data_bytes);
                        s->at_state.rx_data_bytes = 0;
                    }
                    at_put_response_code(&s->at_state, msg[0]);
                }
                else
                {
                    s->at_state.dte_is_waiting = TRUE;
                    break;
                }
            }
            while (msg[0] == AT_RESPONSE_CODE_CONNECT);
        }
        immediate_response = FALSE;
        break;
    default:
        switch (val)
        {
        case 24:
            new_modem = (new_transmit)  ?  T31_V27TER_TX  :  T31_V27TER_RX;
            s->short_train = FALSE;
            s->bit_rate = 2400;
            break;
        case 48:
            new_modem = (new_transmit)  ?  T31_V27TER_TX  :  T31_V27TER_RX;
            s->short_train = FALSE;
            s->bit_rate = 4800;
            break;
        case 72:
            new_modem = (new_transmit)  ?  T31_V29_TX  :  T31_V29_RX;
            s->short_train = FALSE;
            s->bit_rate = 7200;
            break;
        case 96:
            new_modem = (new_transmit)  ?  T31_V29_TX  :  T31_V29_RX;
            s->short_train = FALSE;
            s->bit_rate = 9600;
            break;
#if defined(ENABLE_V17)
        case 73:
            new_modem = (new_transmit)  ?  T31_V17_TX  :  T31_V17_RX;
            s->short_train = FALSE;
            s->bit_rate = 7200;
            break;
        case 74:
            new_modem = (new_transmit)  ?  T31_V17_TX  :  T31_V17_RX;
            s->short_train = TRUE;
            s->bit_rate = 7200;
            break;
        case 97:
            new_modem = (new_transmit)  ?  T31_V17_TX  :  T31_V17_RX;
            s->short_train = FALSE;
            s->bit_rate = 9600;
            break;
        case 98:
            new_modem = (new_transmit)  ?  T31_V17_TX  :  T31_V17_RX;
            s->short_train = TRUE;
            s->bit_rate = 9600;
            break;
        case 121:
            new_modem = (new_transmit)  ?  T31_V17_TX  :  T31_V17_RX;
            s->short_train = FALSE;
            s->bit_rate = 12000;
            break;
        case 122:
            new_modem = (new_transmit)  ?  T31_V17_TX  :  T31_V17_RX;
            s->short_train = TRUE;
            s->bit_rate = 12000;
            break;
        case 145:
            new_modem = (new_transmit)  ?  T31_V17_TX  :  T31_V17_RX;
            s->short_train = FALSE;
            s->bit_rate = 14400;
            break;
        case 146:
            new_modem = (new_transmit)  ?  T31_V17_TX  :  T31_V17_RX;
            s->short_train = TRUE;
            s->bit_rate = 14400;
            break;
#endif
        default:
            return -1;
        }
        span_log(&s->logging, SPAN_LOG_FLOW, "Short training = %d, bit rate = %d\n", s->short_train, s->bit_rate);
        if (new_transmit)
        {
            t31_set_at_rx_mode(s, AT_MODE_STUFFED);
            at_put_response_code(&s->at_state, AT_RESPONSE_CODE_CONNECT);
        }
        else
        {
            t31_set_at_rx_mode(s, AT_MODE_DELIVERY);
        }
        restart_modem(s, new_modem);
        immediate_response = FALSE;
        break;
    }
    return immediate_response;
}
/*- End of function --------------------------------------------------------*/

void t31_call_event(t31_state_t *s, int event)
{
    span_log(&s->logging, SPAN_LOG_FLOW, "Call event %d received\n", event);
    at_call_event(&s->at_state, event);
}
/*- End of function --------------------------------------------------------*/

int t31_at_rx(t31_state_t *s, const char *t, int len)
{
    if (s->dte_data_timeout)
        s->dte_data_timeout = s->call_samples + ms_to_samples(5000);
    switch (s->at_state.at_rx_mode)
    {
    case AT_MODE_ONHOOK_COMMAND:
    case AT_MODE_OFFHOOK_COMMAND:
        at_interpreter(&s->at_state, t, len);
        break;
    case AT_MODE_DELIVERY:
        /* Data from the DTE in this state returns us to command mode */
        if (len)
        {
            s->at_state.rx_data_bytes = 0;
            s->at_state.transmit = FALSE;
            s->modem = T31_SILENCE_TX;
            t31_set_at_rx_mode(s, AT_MODE_OFFHOOK_COMMAND);
            at_put_response_code(&s->at_state, AT_RESPONSE_CODE_OK);
        }
        break;
    case AT_MODE_HDLC:
        dle_unstuff_hdlc(s, t, len);
        break;
    case AT_MODE_STUFFED:
        dle_unstuff(s, t, len);
        break;
    }
    return len;
}
/*- End of function --------------------------------------------------------*/

static int dummy_rx(void *user_data, const int16_t amp[], int len)
{
    return 0;
}
/*- End of function --------------------------------------------------------*/

static int silence_rx(void *user_data, const int16_t amp[], int len)
{
    t31_state_t *s;

    /* Searching for a specified minimum period of silence. */
    s = (t31_state_t *) user_data;
    if (s->silence_awaited  &&  s->silence_heard >= s->silence_awaited)
    {
        at_put_response_code(&s->at_state, AT_RESPONSE_CODE_OK);
        t31_set_at_rx_mode(s, AT_MODE_OFFHOOK_COMMAND);
        s->silence_heard = 0;
        s->silence_awaited = 0;
    }
    return 0;
}
/*- End of function --------------------------------------------------------*/

static int cng_rx(void *user_data, const int16_t amp[], int len)
{
    t31_state_t *s;

    s = (t31_state_t *) user_data;
    if (s->call_samples > ms_to_samples(s->at_state.p.s_regs[7]*1000))
    {
        /* After calling, S7 has elapsed... no carrier found. */
        at_put_response_code(&s->at_state, AT_RESPONSE_CODE_NO_CARRIER);
        restart_modem(s, T31_SILENCE_TX);
        at_modem_control(&s->at_state, AT_MODEM_CONTROL_HANGUP, NULL);
        t31_set_at_rx_mode(s, AT_MODE_ONHOOK_COMMAND);
    }
    else
    {
        fsk_rx(&(s->v21rx), amp, len);
    }
    return 0;
}
/*- End of function --------------------------------------------------------*/

#if defined(ENABLE_V17)
static int early_v17_rx(void *user_data, const int16_t amp[], int len)
{
    t31_state_t *s;

    s = (t31_state_t *) user_data;
    v17_rx(&(s->v17rx), amp, len);
    if (s->at_state.rx_trained)
    {
        /* The fast modem has trained, so we no longer need to run the slow
           one in parallel. */
        span_log(&s->logging, SPAN_LOG_FLOW, "Switching from V.17 + V.21 to V.17 (%.2fdBm0)\n", v17_rx_signal_power(&(s->v17rx)));
        s->rx_handler = (span_rx_handler_t *) &v17_rx;
        s->rx_user_data = &(s->v17rx);
    }
    else
    {
        fsk_rx(&(s->v21rx), amp, len);
        if (s->rx_message_received)
        {
            /* We have received something, and the fast modem has not trained. We must
               be receiving valid V.21 */
            span_log(&s->logging, SPAN_LOG_FLOW, "Switching from V.17 + V.21 to V.21\n");
            s->rx_handler = (span_rx_handler_t *) &fsk_rx;
            s->rx_user_data = &(s->v21rx);
        }
    }
    return len;
}
/*- End of function --------------------------------------------------------*/
#endif

static int early_v27ter_rx(void *user_data, const int16_t amp[], int len)
{
    t31_state_t *s;

    s = (t31_state_t *) user_data;
    v27ter_rx(&(s->v27ter_rx), amp, len);
    if (s->at_state.rx_trained)
    {
        /* The fast modem has trained, so we no longer need to run the slow
           one in parallel. */
        span_log(&s->logging, SPAN_LOG_FLOW, "Switching from V.27ter + V.21 to V.27ter (%.2fdBm0)\n", v27ter_rx_signal_power(&(s->v27ter_rx)));
        s->rx_handler = (span_rx_handler_t *) &v27ter_rx;
        s->rx_user_data = &(s->v27ter_rx);
    }
    else
    {
        fsk_rx(&(s->v21rx), amp, len);
        if (s->rx_message_received)
        {
            /* We have received something, and the fast modem has not trained. We must
               be receiving valid V.21 */
            span_log(&s->logging, SPAN_LOG_FLOW, "Switching from V.27ter + V.21 to V.21\n");
            s->rx_handler = (span_rx_handler_t *) &fsk_rx;
            s->rx_user_data = &(s->v21rx);
        }
    }
    return len;
}
/*- End of function --------------------------------------------------------*/

static int early_v29_rx(void *user_data, const int16_t amp[], int len)
{
    t31_state_t *s;

    s = (t31_state_t *) user_data;
    v29_rx(&(s->v29rx), amp, len);
    if (s->at_state.rx_trained)
    {
        /* The fast modem has trained, so we no longer need to run the slow
           one in parallel. */
        span_log(&s->logging, SPAN_LOG_FLOW, "Switching from V.29 + V.21 to V.29 (%.2fdBm0)\n", v29_rx_signal_power(&(s->v29rx)));
        s->rx_handler = (span_rx_handler_t *) &v29_rx;
        s->rx_user_data = &(s->v29rx);
    }
    else
    {
        fsk_rx(&(s->v21rx), amp, len);
        if (s->rx_message_received)
        {
            /* We have received something, and the fast modem has not trained. We must
               be receiving valid V.21 */
            span_log(&s->logging, SPAN_LOG_FLOW, "Switching from V.29 + V.21 to V.21\n");
            s->rx_handler = (span_rx_handler_t *) &fsk_rx;
            s->rx_user_data = &(s->v21rx);
        }
    }
    return len;
}
/*- End of function --------------------------------------------------------*/

int t31_rx(t31_state_t *s, int16_t amp[], int len)
{
    int i;
    int32_t power;

    /* Monitor for received silence.  Maximum needed detection is AT+FRS=255 (255*10ms). */
    /* We could probably only run this loop if (s->modem == T31_SILENCE_RX), however,
       the spec says "when silence has been present on the line for the amount of
       time specified".  That means some of the silence may have occurred before
       the AT+FRS=n command. This condition, however, is not likely to ever be the
       case.  (AT+FRS=n will usually be issued before the remote goes silent.) */
    for (i = 0;  i < len;  i++)
    {
        /* Clean up any DC influence. */
        power = power_meter_update(&(s->rx_power), amp[i] - s->last_sample);
        s->last_sample = amp[i];
        if (power > s->silence_threshold_power)
        {
            s->silence_heard = 0;
        }
        else
        {        
            if (s->silence_heard <= ms_to_samples(255*10))
                s->silence_heard++;
        }
    }

    /* Time is determined by counting the samples in audio packets coming in. */
    s->call_samples += len;

    /* In HDLC transmit mode, if 5 seconds elapse without data from the DTE
       we must treat this as an error. We return the result ERROR, and change
       to command-mode. */
    if (s->dte_data_timeout  &&  s->call_samples > s->dte_data_timeout)
    {
        t31_set_at_rx_mode(s, AT_MODE_OFFHOOK_COMMAND);
        at_put_response_code(&s->at_state, AT_RESPONSE_CODE_ERROR);
        restart_modem(s, T31_SILENCE_TX);
    }

    if (!s->at_state.transmit  ||  s->modem == T31_CNG_TONE)
        s->rx_handler(s->rx_user_data, amp, len);
    return  0;
}
/*- End of function --------------------------------------------------------*/

static int set_next_tx_type(t31_state_t *s)
{
    if (s->next_tx_handler)
    {
        s->tx_handler = s->next_tx_handler;
        s->tx_user_data = s->next_tx_user_data;
        s->next_tx_handler = NULL;
        return 0;
    }
    /* If there is nothing else to change to, so use zero length silence */
    silence_gen_alter(&(s->silence_gen), 0);
    s->tx_handler = (span_tx_handler_t *) &silence_gen;
    s->tx_user_data = &(s->silence_gen);
    s->next_tx_handler = NULL;
    return -1;
}
/*- End of function --------------------------------------------------------*/

int t31_tx(t31_state_t *s, int16_t amp[], int max_len)
{
    int len;

    len = 0;
    if (s->at_state.transmit)
    {
        if ((len = s->tx_handler(s->tx_user_data, amp, max_len)) < max_len)
        {
            /* Allow for one change of tx handler within a block */
            set_next_tx_type(s);
            if ((len += s->tx_handler(s->tx_user_data, amp + len, max_len - len)) < max_len)
            {
                switch (s->modem)
                {
                case T31_SILENCE_TX:
                    s->modem = -1;
                    at_put_response_code(&s->at_state, AT_RESPONSE_CODE_OK);
                    if (s->at_state.do_hangup)
                    {
                        at_modem_control(&s->at_state, AT_MODEM_CONTROL_HANGUP, NULL);
                        t31_set_at_rx_mode(s, AT_MODE_ONHOOK_COMMAND);
                        s->at_state.do_hangup = FALSE;
                    }
                    else
                    {
                        t31_set_at_rx_mode(s, AT_MODE_OFFHOOK_COMMAND);
                    }
                    break;
                case T31_CED_TONE:
                    /* Go directly to V.21/HDLC transmit. */
                    s->modem = -1;
                    restart_modem(s, T31_V21_TX);
                    t31_set_at_rx_mode(s, AT_MODE_HDLC);
                    break;
                case T31_V21_TX:
#if defined(ENABLE_V17)
                case T31_V17_TX:
#endif
                case T31_V27TER_TX:
                case T31_V29_TX:
                    s->modem = -1;
                    at_put_response_code(&s->at_state, AT_RESPONSE_CODE_OK);
                    t31_set_at_rx_mode(s, AT_MODE_OFFHOOK_COMMAND);
                    restart_modem(s, T31_SILENCE_TX);
                    break;
                }
            }
        }
    }
    if (s->transmit_on_idle)
    {
        /* Pad to the requested length with silence */
        memset(amp, 0, max_len*sizeof(int16_t));
        len = max_len;        
    }
    return len;
}
/*- End of function --------------------------------------------------------*/

void t31_set_transmit_on_idle(t31_state_t *s, int transmit_on_idle)
{
    s->transmit_on_idle = transmit_on_idle;
}
/*- End of function --------------------------------------------------------*/

t31_state_t *t31_init(t31_state_t *s,
                      at_tx_handler_t *at_tx_handler,
                      void *at_tx_user_data,
                      t31_modem_control_handler_t *modem_control_handler,
                      void *modem_control_user_data,
                      t38_tx_packet_handler_t *tx_t38_packet_handler,
                      void *tx_t38_packet_user_data)
{
    if (at_tx_handler == NULL  ||  modem_control_handler == NULL)
        return NULL;

    memset(s, 0, sizeof(*s));
    span_log_init(&s->logging, SPAN_LOG_NONE, NULL);
    span_log_set_protocol(&s->logging, "T.31");

    s->modem_control_handler = modem_control_handler;
    s->modem_control_user_data = modem_control_user_data;
#if defined(ENABLE_V17)
    v17_rx_init(&(s->v17rx), 14400, non_ecm_put_bit, s);
    v17_tx_init(&(s->v17tx), 14400, FALSE, non_ecm_get_bit, s);
#endif
    v29_rx_init(&(s->v29rx), 9600, non_ecm_put_bit, s);
    v29_rx_signal_cutoff(&(s->v29rx), -45.5);
    v29_tx_init(&(s->v29tx), 9600, FALSE, non_ecm_get_bit, s);
    v27ter_rx_init(&(s->v27ter_rx), 4800, non_ecm_put_bit, s);
    v27ter_tx_init(&(s->v27ter_tx), 4800, FALSE, non_ecm_get_bit, s);
    silence_gen_init(&(s->silence_gen), 0);
    power_meter_init(&(s->rx_power), 4);
    s->last_sample = 0;
    s->silence_threshold_power = power_meter_level_dbm0(-43);
    s->at_state.rx_signal_present = FALSE;
    s->at_state.rx_trained = FALSE;

    s->at_state.do_hangup = FALSE;
    s->at_state.line_ptr = 0;
    s->silence_heard = 0;
    s->silence_awaited = 0;
    s->call_samples = 0;
    s->modem = -1;
    s->at_state.transmit = TRUE;
    s->rx_handler = dummy_rx;
    s->rx_user_data = NULL;
    s->tx_handler = (span_tx_handler_t *) &silence_gen;
    s->tx_user_data = &(s->silence_gen);

    if (queue_create(&(s->rx_queue), 4096, QUEUE_WRITE_ATOMIC | QUEUE_READ_ATOMIC) < 0)
        return NULL;
    at_init(&s->at_state, at_tx_handler, at_tx_user_data, t31_modem_control_handler, s);
    at_set_class1_handler(&s->at_state, process_class1_cmd, s);
    s->at_state.dte_inactivity_timeout = DEFAULT_DTE_TIMEOUT;
    if (tx_t38_packet_handler)
    {
        t38_core_init(&s->t38, process_rx_indicator, process_rx_data, process_rx_missing, (void *) s);
        s->t38.tx_packet_handler = tx_t38_packet_handler;
        s->t38.tx_packet_user_data = tx_t38_packet_user_data;
    }
    s->t38_mode = FALSE;
    return s;
}
/*- End of function --------------------------------------------------------*/

int t31_release(t31_state_t *s)
{
    at_reset_call_info(&s->at_state);
    free(s);
    return 0;
}
/*- End of function --------------------------------------------------------*/
/*- End of file ------------------------------------------------------------*/
