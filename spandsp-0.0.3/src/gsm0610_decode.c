/*
 * SpanDSP - a series of DSP components for telephony
 *
 * gsm0610_decode.c - GSM 06.10 full rate speech codec.
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
 * This code is based on the widely used GSM 06.10 code available from
 * http://kbs.cs.tu-berlin.de/~jutta/toast.html
 *
 * $Id: gsm0610_decode.c,v 1.12 2006/11/30 15:41:47 steveu Exp $
 */

/*! \file */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>
#include <inttypes.h>
#if defined(HAVE_TGMATH_H)
#include <tgmath.h>
#endif
#if defined(HAVE_MATH_H)
#include <math.h>
#endif
#include <stdlib.h>
#include <memory.h>

#include "spandsp/telephony.h"
#include "spandsp/dc_restore.h"
#include "spandsp/bitstream.h"
#include "spandsp/gsm0610.h"

#include "gsm0610_local.h"

/* 4.3 FIXED POINT IMPLEMENTATION OF THE RPE-LTP DECODER */

static void postprocessing(gsm0610_state_t *s, int16_t amp[])
{
    int k;
    int16_t msr;
    int16_t tmp;

    msr = s->msr;
    for (k = 0;  k < GSM0610_FRAME_LEN;  k++)
    {
        tmp = gsm_mult_r(msr, 28180);
        /* De-emphasis */
        msr = gsm_add(amp[k], tmp);
        /* Truncation & upscaling */
        amp[k] = (int16_t) (gsm_add(msr, msr) & 0xFFF8);
    }
    /*endfor*/
    s->msr = msr;
}
/*- End of function --------------------------------------------------------*/

static void decode_a_frame(gsm0610_state_t *s,
                           int16_t amp[GSM0610_FRAME_LEN],
                           gsm0610_frame_t *f)
{
    int j;
    int k;
    int16_t erp[40];
    int16_t wt[GSM0610_FRAME_LEN];
    int16_t *drp;

    drp = s->dp0 + 120;
    for (j = 0;  j < 4;  j++)
    {
        gsm0610_rpe_decoding(s, f->xmaxc[j], f->Mc[j], f->xMc[j], erp);
        gsm0610_long_term_synthesis_filtering(s, f->Nc[j], f->bc[j], erp, drp);
        for (k = 0;  k < 40;  k++)
            wt[j*40 + k] = drp[k];
        /*endfor*/
    }
    /*endfor*/

    gsm0610_short_term_synthesis_filter(s, f->LARc, wt, amp);
    postprocessing(s, amp);
}
/*- End of function --------------------------------------------------------*/

int gsm0610_unpack_none(gsm0610_frame_t *s, const uint8_t c[])
{
    int i;
    int j;
    int k;
    
    i = 0;
    for (j = 0;  j < 8;  j++)
        s->LARc[j] = c[i++];
    for (j = 0;  j < 4;  j++)
    {
        s->Nc[j] = c[i++];
        s->bc[j] = c[i++];
        s->Mc[j] = c[i++];
        s->xmaxc[j] = c[i++];
        for (k = 0;  k < 13;  k++)
            s->xMc[j][k] = c[i++];
    }
    return 76;
}
/*- End of function --------------------------------------------------------*/

int gsm0610_unpack_wav49(gsm0610_frame_t *s, const uint8_t code[], int half)
{
    int i;
    int j;
    static bitstream_state_t bs;
    const uint8_t *c;

    c = code;
    if (half)
        bitstream_init(&bs);
    s->LARc[0] = (int16_t) bitstream_get(&bs, &c, 6);
    s->LARc[1] = (int16_t) bitstream_get(&bs, &c, 6);
    s->LARc[2] = (int16_t) bitstream_get(&bs, &c, 5);
    s->LARc[3] = (int16_t) bitstream_get(&bs, &c, 5);
    s->LARc[4] = (int16_t) bitstream_get(&bs, &c, 4);
    s->LARc[5] = (int16_t) bitstream_get(&bs, &c, 4);
    s->LARc[6] = (int16_t) bitstream_get(&bs, &c, 3);
    s->LARc[7] = (int16_t) bitstream_get(&bs, &c, 3);
    for (i = 0;  i < 4;  i++)
    {
        s->Nc[i] = (int16_t) bitstream_get(&bs, &c, 7);
        s->bc[i] = (int16_t) bitstream_get(&bs, &c, 2);
        s->Mc[i] = (int16_t) bitstream_get(&bs, &c, 2);
        s->xmaxc[i] = (int16_t) bitstream_get(&bs, &c, 6);
        for (j = 0;  j < 13;  j++)
            s->xMc[i][j] = (int16_t) bitstream_get(&bs, &c, 3);
    }
    return (half)  ?  33  :  32;
}
/*- End of function --------------------------------------------------------*/

int gsm0610_unpack_voip(gsm0610_frame_t *s, const uint8_t code[])
{
    int i;
    int j;
    const uint8_t *c;
    unsigned int magic;
    bitstream_state_t bs;

    c = code;
    bitstream_init(&bs);
    magic = bitstream_get2(&bs, &c, 4);
    if (magic != GSM0610_MAGIC)
        return -1;
    s->LARc[0] = (int16_t) bitstream_get2(&bs, &c, 6);
    s->LARc[1] = (int16_t) bitstream_get2(&bs, &c, 6);
    s->LARc[2] = (int16_t) bitstream_get2(&bs, &c, 5);
    s->LARc[3] = (int16_t) bitstream_get2(&bs, &c, 5);
    s->LARc[4] = (int16_t) bitstream_get2(&bs, &c, 4);
    s->LARc[5] = (int16_t) bitstream_get2(&bs, &c, 4);
    s->LARc[6] = (int16_t) bitstream_get2(&bs, &c, 3);
    s->LARc[7] = (int16_t) bitstream_get2(&bs, &c, 3);
    for (i = 0;  i < 4;  i++)
    {
        s->Nc[i] = (int16_t) bitstream_get2(&bs, &c, 7);
        s->bc[i] = (int16_t) bitstream_get2(&bs, &c, 2);
        s->Mc[i] = (int16_t) bitstream_get2(&bs, &c, 2);
        s->xmaxc[i] = (int16_t) bitstream_get2(&bs, &c, 6);
        for (j = 0;  j < 13;  j++)
            s->xMc[i][j] = (int16_t) bitstream_get2(&bs, &c, 3);
    }
    return 33;
}
/*- End of function --------------------------------------------------------*/

int gsm0610_decode(gsm0610_state_t *s, int16_t amp[], const uint8_t code[], int quant)
{
    gsm0610_frame_t frame;
    const uint8_t *c;
    int bytes;
    int i;

    if (s->packing == GSM0610_PACKING_WAV49)
        quant <<= 1;
    c = code;
    for (i = 0;  i < quant;  i++)
    {
        switch (s->packing)
        {
        default:
        case GSM0610_PACKING_NONE:
            bytes = gsm0610_unpack_none(&frame, c);
            break;
        case GSM0610_PACKING_WAV49:
            s->frame_index = !s->frame_index;
            bytes = gsm0610_unpack_wav49(&frame, c, s->frame_index);
            break;
        case GSM0610_PACKING_VOIP:
            bytes = gsm0610_unpack_voip(&frame, c);
            break;
        }
        /*endswitch*/
        if (bytes < 0)
            return 0;
        decode_a_frame(s, amp, &frame);
        c += bytes;
        amp += GSM0610_FRAME_LEN;
    }
    /*endwhile*/
    return quant*GSM0610_FRAME_LEN;
}
/*- End of function --------------------------------------------------------*/
/*- End of file ------------------------------------------------------------*/
