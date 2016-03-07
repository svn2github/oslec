/*
 * SpanDSP - a series of DSP components for telephony
 *
 * test_utils.c - Utility routines for module tests.
 *
 * Written by Steve Underwood <steveu@coppice.org>
 *
 * Copyright (C) 2006 Steve Underwood
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
 * $Id: test_utils.c,v 1.7 2006/11/19 14:07:27 steveu Exp $
 */

/*! \file */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <inttypes.h>
#include <string.h>
#include <stdio.h>
#if defined(HAVE_TGMATH_H)
#include <tgmath.h>
#endif
#if defined(HAVE_MATH_H)
#include <math.h>
#endif
#include <time.h>
#include <fcntl.h>
#include <audiofile.h>
#include <tiffio.h>

#include "spandsp.h"
#include "test_utils.h"

struct codec_munge_state_s
{
    int munging_codec;
    g726_state_t g726_enc_state;
    g726_state_t g726_dec_state;
};

codec_munge_state_t *codec_munge_init(int codec)
{
    codec_munge_state_t *s;
    
    if ((s = (codec_munge_state_t *) malloc(sizeof(*s))))
    {
        switch (codec)
        {
        case MUNGE_CODEC_G726_40K:
            g726_init(&s->g726_enc_state, 40000, G726_ENCODING_LINEAR, G726_PACKING_NONE);
            g726_init(&s->g726_dec_state, 40000, G726_ENCODING_LINEAR, G726_PACKING_NONE);
            s->munging_codec = MUNGE_CODEC_G726_32K;
            break;
        case MUNGE_CODEC_G726_32K:
            g726_init(&s->g726_enc_state, 32000, G726_ENCODING_LINEAR, G726_PACKING_NONE);
            g726_init(&s->g726_dec_state, 32000, G726_ENCODING_LINEAR, G726_PACKING_NONE);
            s->munging_codec = MUNGE_CODEC_G726_32K;
            break;
        case MUNGE_CODEC_G726_24K:
            g726_init(&s->g726_enc_state, 24000, G726_ENCODING_LINEAR, G726_PACKING_NONE);
            g726_init(&s->g726_dec_state, 24000, G726_ENCODING_LINEAR, G726_PACKING_NONE);
            s->munging_codec = MUNGE_CODEC_G726_32K;
            break;
        case MUNGE_CODEC_G726_16K:
            g726_init(&s->g726_enc_state, 16000, G726_ENCODING_LINEAR, G726_PACKING_NONE);
            g726_init(&s->g726_dec_state, 16000, G726_ENCODING_LINEAR, G726_PACKING_NONE);
            s->munging_codec = MUNGE_CODEC_G726_32K;
            break;
        default:
            s->munging_codec = codec;
            break;
        }
    }
    return s;
}
/*- End of function --------------------------------------------------------*/

void codec_munge(codec_munge_state_t *s, int16_t amp[], int len)
{
    uint8_t law;
    uint8_t adpcmdata[160];
    int i;
    int adpcm;
    int x;

    switch (s->munging_codec)
    {
    case MUNGE_CODEC_NONE:
        /* Do nothing */
        break;
    case MUNGE_CODEC_ALAW:
        for (i = 0;  i < len;  i++)
        {
            law = linear_to_alaw(amp[i]);
            amp[i] = alaw_to_linear(law);
        }
        break;
    case MUNGE_CODEC_ULAW:
        for (i = 0;  i < len;  i++)
        {
            law = linear_to_ulaw(amp[i]);
            amp[i] = ulaw_to_linear(law);
        }
        break;
    case MUNGE_CODEC_G726_32K:
        /* This could actually be any of the G.726 rates */
        for (i = 0;  i < len;  i += x)
        {
            x = (len - i >= 160)  ?  160  :  (len - i);
            adpcm = g726_encode(&s->g726_enc_state, adpcmdata, amp + i, x);
            g726_decode(&s->g726_dec_state, amp + i, adpcmdata, adpcm);
        }
        break;
    }
}
/*- End of function --------------------------------------------------------*/
/*- End of file ------------------------------------------------------------*/
