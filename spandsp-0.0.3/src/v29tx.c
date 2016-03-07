/*
 * SpanDSP - a series of DSP components for telephony
 *
 * v29tx.c - ITU V.29 modem transmit part
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
 * $Id: v29tx.c,v 1.58 2006/11/28 16:59:57 steveu Exp $
 */

/*! \file */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <inttypes.h>
#include <stdlib.h>
#include <string.h>
#if defined(HAVE_TGMATH_H)
#include <tgmath.h>
#endif
#if defined(HAVE_MATH_H)
#include <math.h>
#endif

#include "spandsp/telephony.h"
#include "spandsp/logging.h"
#include "spandsp/complex.h"
#include "spandsp/vector_float.h"
#include "spandsp/complex_vector_float.h"
#include "spandsp/async.h"
#include "spandsp/dds.h"
#include "spandsp/power_meter.h"

#include "spandsp/v29tx.h"

#define CARRIER_NOMINAL_FREQ        1700.0f

/* Segments of the training sequence */
#define V29_TRAINING_SEG_TEP        0
#define V29_TRAINING_SEG_1          (V29_TRAINING_SEG_TEP + 480)
#define V29_TRAINING_SEG_2          (V29_TRAINING_SEG_1 + 48)
#define V29_TRAINING_SEG_3          (V29_TRAINING_SEG_2 + 128)
#define V29_TRAINING_SEG_4          (V29_TRAINING_SEG_3 + 384)
#define V29_TRAINING_END            (V29_TRAINING_SEG_4 + 48)
#define V29_TRAINING_SHUTDOWN_END   (V29_TRAINING_END + 32)

/* Raised root cosine pulse shaping; Beta = 0.25; 4 symbols either
   side of the centre. */
/* Created with mkshape -r 0.05 0.25 91 -l and then split up */
#define PULSESHAPER_GAIN            (9.9888356312f/10.0f)
#define PULSESHAPER_COEFF_SETS      10

static const float pulseshaper[PULSESHAPER_COEFF_SETS][V29_TX_FILTER_STEPS] =
{
    {
        -0.0029426223f,         /* Filter 0 */
        -0.0183060118f,
         0.0653192857f,
        -0.1703207714f,
         0.6218069936f,
         0.6218069936f,
        -0.1703207714f,
         0.0653192857f,
        -0.0183060118f
    },
    {
         0.0031876922f,         /* Filter 1 */
        -0.0300884145f,
         0.0832744718f,
        -0.1974255221f,
         0.7664229820f,
         0.4670580725f,
        -0.1291107519f,
         0.0424189243f,
        -0.0059810465f
    },
    {
         0.0097229236f,         /* Filter 2 */
        -0.0394811291f,
         0.0931039664f,
        -0.2043906784f,
         0.8910868760f,
         0.3122713836f,
        -0.0802880559f,
         0.0179050490f,
         0.0052057308f
    },
    {
         0.0156117223f,         /* Filter 3 */
        -0.0447125347f,
         0.0922040267f,
        -0.1862939416f,
         0.9870942864f,
         0.1669790517f,
        -0.0301581072f,
        -0.0051358510f,
         0.0139350286f
    },
    {
         0.0197702545f,         /* Filter 4 */
        -0.0443470335f,
         0.0789538534f,
        -0.1399184160f,
         1.0476130256f,
         0.0393903028f,
         0.0157339854f,
        -0.0241879599f,
         0.0193774571f
    },
    {
         0.0212455717f,         /* Filter 5 */
        -0.0375307894f,
         0.0530516472f,
        -0.0642195521f,
         1.0682849922f,
        -0.0642195521f,
         0.0530516472f,
        -0.0375307894f,
         0.0212455717f
    },
    {
         0.0193774571f,         /* Filter 6 */
        -0.0241879599f,
         0.0157339854f,
         0.0393903028f,
         1.0476130256f,
        -0.1399184160f,
         0.0789538534f,
        -0.0443470335f,
         0.0197702545f
    },
    {
         0.0139350286f,         /* Filter 7 */
        -0.0051358510f,
        -0.0301581072f,
         0.1669790517f,
         0.9870942864f,
        -0.1862939416f,
         0.0922040267f,
        -0.0447125347f,
         0.0156117223f
    },
    {
         0.0052057308f,         /* Filter 8 */
         0.0179050490f,
        -0.0802880559f,
         0.3122713836f,
         0.8910868760f,
        -0.2043906784f,
         0.0931039664f,
        -0.0394811291f,
         0.0097229236f
    },
    {
        -0.0059810465f,         /* Filter 9 */
         0.0424189243f,
        -0.1291107519f,
         0.4670580725f,
         0.7664229820f,
        -0.1974255221f,
         0.0832744718f,
        -0.0300884145f,
         0.0031876922f
    },
};

static int fake_get_bit(void *user_data)
{
    return 1;
}
/*- End of function --------------------------------------------------------*/

static __inline__ int get_scrambled_bit(v29_tx_state_t *s)
{
    int bit;
    int out_bit;

    if ((bit = s->current_get_bit(s->user_data)) == PUTBIT_END_OF_DATA)
    {
        /* End of real data. Switch to the fake get_bit routine, until we
           have shut down completely. */
        s->current_get_bit = fake_get_bit;
        s->in_training = TRUE;
        bit = 1;
    }
    out_bit = (bit ^ (s->scramble_reg >> 17) ^ (s->scramble_reg >> 22)) & 1;
    s->scramble_reg = (s->scramble_reg << 1) | out_bit;
    return out_bit;
}
/*- End of function --------------------------------------------------------*/

static __inline__ complexf_t getbaud(v29_tx_state_t *s)
{
    static const int phase_steps_9600[8] =
    {
        1, 0, 2, 3, 6, 7, 5, 4
    };
    static const int phase_steps_4800[4] =
    {
        0, 2, 6, 4
    };
    static const complexf_t constellation[16] =
    {
        { 3.0,  0.0},   /*   0deg low  */
        { 1.0,  1.0},   /*  45deg low  */
        { 0.0,  3.0},   /*  90deg low  */
        {-1.0,  1.0},   /* 135deg low  */
        {-3.0,  0.0},   /* 180deg low  */
        {-1.0, -1.0},   /* 225deg low  */
        { 0.0, -3.0},   /* 270deg low  */
        { 1.0, -1.0},   /* 315deg low  */
        { 5.0,  0.0},   /*   0deg high */
        { 3.0,  3.0},   /*  45deg high */
        { 0.0,  5.0},   /*  90deg high */
        {-3.0,  3.0},   /* 135deg high */
        {-5.0,  0.0},   /* 180deg high */
        {-3.0, -3.0},   /* 225deg high */
        { 0.0, -5.0},   /* 270deg high */
        { 3.0, -3.0}    /* 315deg high */
    };
    static const complexf_t abab[6] =
    {
        { 3.0, -3.0},   /* 315deg high 9600 */
        {-3.0,  0.0},   /* 180deg low       */
        { 1.0, -1.0},   /* 315deg low 7200  */
        {-3.0,  0.0},   /* 180deg low       */
        { 0.0, -3.0},   /* 270deg low 4800  */
        {-3.0,  0.0}    /* 180deg low       */
    };
    static const complexf_t cdcd[6] =
    {
        { 3.0,  0.0},   /*   0deg low 9600  */
        {-3.0,  3.0},   /* 135deg high      */
        { 3.0,  0.0},   /*   0deg low 7200  */
        {-1.0,  1.0},   /* 135deg low       */
        { 3.0,  0.0},   /*   0deg low 4800  */
        { 0.0,  3.0}    /*  90deg low       */
    };
    int bits;
    int amp;
    int bit;

    if (s->in_training)
    {
        /* Send the training sequence */
        if (++s->training_step <= V29_TRAINING_SEG_4)
        {
            if (s->training_step <= V29_TRAINING_SEG_3)
            {
                if (s->training_step <= V29_TRAINING_SEG_1)
                {
                    /* Optional segment: Unmodulated carrier (talker echo protection) */
                    return constellation[0];
                }
                if (s->training_step <= V29_TRAINING_SEG_2)
                {
                    /* Segment 1: silence */
                    return complex_setf(0.0f, 0.0f);
                }
                /* Segment 2: ABAB... */
                return abab[(s->training_step & 1) + s->training_offset];
            }
            /* Segment 3: CDCD... */
            /* Apply the 1 + x^-6 + x^-7 training scrambler */
            bit = s->training_scramble_reg & 1;
            s->training_scramble_reg >>= 1;
            s->training_scramble_reg |= (((bit ^ s->training_scramble_reg) & 1) << 6);
            return cdcd[bit + s->training_offset];
        }
        /* We should be in the block of test ones, or shutdown ones, if we get here. */
        /* There is no graceful shutdown procedure defined for V.29. Just
           send some ones, to ensure we get the real data bits through, even
           with bad ISI. */
        if (s->training_step == V29_TRAINING_END + 1)
        {
            /* Switch from the fake get_bit routine, to the user supplied real
               one, and we are up and running. */
            s->current_get_bit = s->get_bit;
            s->in_training = FALSE;
        }
    }
    /* 9600bps uses the full constellation.
       7200bps uses only the first half of the full constellation.
       4800bps uses the smaller constellation. */
    amp = 0;
    /* We only use an amplitude bit at 9600bps */
    if (s->bit_rate == 9600  &&  get_scrambled_bit(s))
        amp = 8;
    /*endif*/
    bits = get_scrambled_bit(s);
    bits = (bits << 1) | get_scrambled_bit(s);
    if (s->bit_rate == 4800)
    {
        bits = phase_steps_4800[bits];
    }
    else
    {
        bits = (bits << 1) | get_scrambled_bit(s);
        bits = phase_steps_9600[bits];
    }
    s->constellation_state = (s->constellation_state + bits) & 7;
    return constellation[amp | s->constellation_state];
}
/*- End of function --------------------------------------------------------*/

int v29_tx(v29_tx_state_t *s, int16_t amp[], int len)
{
    complexf_t x;
    complexf_t z;
    int i;
    int sample;

    if (s->training_step >= V29_TRAINING_SHUTDOWN_END)
    {
        /* Once we have sent the shutdown symbols, we stop sending completely. */
        return 0;
    }
    for (sample = 0;  sample < len;  sample++)
    {
        if ((s->baud_phase += 3) >= 10)
        {
            s->baud_phase -= 10;
            s->rrc_filter[s->rrc_filter_step] =
            s->rrc_filter[s->rrc_filter_step + V29_TX_FILTER_STEPS] = getbaud(s);
            if (++s->rrc_filter_step >= V29_TX_FILTER_STEPS)
                s->rrc_filter_step = 0;
        }
        /* Root raised cosine pulse shaping at baseband */
        x.re = 0.0f;
        x.im = 0.0f;
        for (i = 0;  i < V29_TX_FILTER_STEPS;  i++)
        {
            x.re += pulseshaper[9 - s->baud_phase][i]*s->rrc_filter[i + s->rrc_filter_step].re;
            x.im += pulseshaper[9 - s->baud_phase][i]*s->rrc_filter[i + s->rrc_filter_step].im;
        }
        /* Now create and modulate the carrier */
        z = dds_complexf(&(s->carrier_phase), s->carrier_phase_rate);
        /* Don't bother saturating. We should never clip. */
        amp[sample] = (int16_t) lrintf((x.re*z.re - x.im*z.im)*s->gain);
    }
    return sample;
}
/*- End of function --------------------------------------------------------*/

static void set_working_gain(v29_tx_state_t *s)
{
    switch (s->bit_rate)
    {
    case 9600:
        s->gain = 0.387f*s->base_gain;
        break;
    case 7200:
        s->gain = 0.605f*s->base_gain;
        break;
    case 4800:
        s->gain = 0.470f*s->base_gain;
        break;
    default:
        break;
    }
}
/*- End of function --------------------------------------------------------*/

void v29_tx_power(v29_tx_state_t *s, float power)
{
    /* The constellation does not maintain constant average power as we change bit rates.
       We need to scale the gain we get here by a bit rate specific scaling factor each
       time we restart the modem. */
    s->base_gain = powf(10.0f, (power - DBM0_MAX_POWER)/20.0f)*32768.0f/PULSESHAPER_GAIN;
    set_working_gain(s);
}
/*- End of function --------------------------------------------------------*/

void v29_tx_set_get_bit(v29_tx_state_t *s, get_bit_func_t get_bit, void *user_data)
{
    if (s->get_bit == s->current_get_bit)
        s->current_get_bit = get_bit;
    s->get_bit = get_bit;
    s->user_data = user_data;
}
/*- End of function --------------------------------------------------------*/

int v29_tx_restart(v29_tx_state_t *s, int rate, int tep)
{
    span_log(&s->logging, SPAN_LOG_FLOW, "Restarting V.29\n");
    s->bit_rate = rate;
    set_working_gain(s);
    switch (s->bit_rate)
    {
    case 9600:
        s->training_offset = 0;
        break;
    case 7200:
        s->training_offset = 2;
        break;
    case 4800:
        s->training_offset = 4;
        break;
    default:
        return -1;
    }
    cvec_zerof(s->rrc_filter, sizeof(s->rrc_filter)/sizeof(s->rrc_filter[0]));
    s->rrc_filter_step = 0;
    s->scramble_reg = 0;
    s->training_scramble_reg = 0x2A;
    s->in_training = TRUE;
    s->training_step = (tep)  ?  V29_TRAINING_SEG_TEP  :  V29_TRAINING_SEG_1;
    s->carrier_phase = 0;
    s->baud_phase = 0;
    s->constellation_state = 0;
    s->current_get_bit = fake_get_bit;
    return 0;
}
/*- End of function --------------------------------------------------------*/

v29_tx_state_t *v29_tx_init(v29_tx_state_t *s, int rate, int tep, get_bit_func_t get_bit, void *user_data)
{
    if (s == NULL)
    {
        if ((s = (v29_tx_state_t *) malloc(sizeof(*s))) == NULL)
            return NULL;
    }
    memset(s, 0, sizeof(*s));
    s->get_bit = get_bit;
    s->user_data = user_data;
    s->carrier_phase_rate = dds_phase_ratef(CARRIER_NOMINAL_FREQ);
    v29_tx_power(s, -14.0f);
    v29_tx_restart(s, rate, tep);
    return s;
}
/*- End of function --------------------------------------------------------*/

int v29_tx_release(v29_tx_state_t *s)
{
    free(s);
    return 0;
}
/*- End of function --------------------------------------------------------*/
/*- End of file ------------------------------------------------------------*/
