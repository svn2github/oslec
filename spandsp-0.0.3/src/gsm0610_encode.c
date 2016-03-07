/*
 * SpanDSP - a series of DSP components for telephony
 *
 * gsm0610_encode.c - GSM 06.10 full rate speech codec.
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
 * $Id: gsm0610_encode.c,v 1.12 2006/11/30 15:41:47 steveu Exp $
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

/* 4.2 FIXED POINT IMPLEMENTATION OF THE RPE-LTP CODER */

/* The RPE-LTD coder works on a frame by frame basis.  The length of
   the frame is equal to 160 samples.  Some computations are done
   once per frame to produce at the output of the coder the
   LARc[1..8] parameters which are the coded LAR coefficients and 
   also to realize the inverse filtering operation for the entire
   frame (160 samples of signal d[0..159]).  These parts produce at
   the output of the coder:

   Procedure 4.2.11 to 4.2.18 are to be executed four times per
   frame.  That means once for each sub-segment RPE-LTP analysis of
   40 samples.  These parts produce at the output of the coder.
*/
static void encode_a_frame(gsm0610_state_t *s, const int16_t amp[], gsm0610_frame_t *f)
{
    int k;
    int16_t *dp;
    int16_t *dpp;
    int16_t so[GSM0610_FRAME_LEN];
    int i;

    dp = s->dp0 + 120;
    dpp = dp;
    gsm0610_preprocess(s, amp, so);
    gsm0610_lpc_analysis(s, so, f->LARc);
    gsm0610_short_term_analysis_filter(s, f->LARc, so);

    for (k = 0;  k < 4;  k++)
    {
        gsm0610_long_term_predictor(s,
                                    so + k*40,
                                    dp,
                                    s->e + 5,
                                    dpp,
                                    &f->Nc[k],
                                    &f->bc[k]);
        gsm0610_rpe_encoding(s, s->e + 5, &f->xmaxc[k], &f->Mc[k], f->xMc[k]);

        for (i = 0;  i < 40;  i++)
            dp[i] = gsm_add(s->e[5 + i], dpp[i]);
        /*endfor*/
        dp += 40;
        dpp += 40;
    }
    /*endfor*/
    memcpy ((char *) s->dp0,
            (char *) (s->dp0 + GSM0610_FRAME_LEN),
            120*sizeof(*s->dp0));
}
/*- End of function --------------------------------------------------------*/

gsm0610_state_t *gsm0610_init(gsm0610_state_t *s, int packing)
{
    if (s == NULL)
    {
        s = (gsm0610_state_t *) malloc(sizeof (gsm0610_state_t));
        if (s == NULL)
            return  NULL;
        /*endif*/
    }
    /*endif*/
    memset((char *) s, '\0', sizeof (gsm0610_state_t));
    s->nrp = 40;
    s->packing = packing;
    return s;
}
/*- End of function --------------------------------------------------------*/

int gsm0610_release(gsm0610_state_t *s)
{
    if (s)
        free(s);
    /*endif*/
    return 0;
}
/*- End of function --------------------------------------------------------*/

int gsm0610_pack_none(uint8_t c[], gsm0610_frame_t *s)
{
    int i;
    int j;
    int k;
    
    i = 0;
    for (j = 0;  j < 8;  j++)
        c[i++] = (uint8_t) s->LARc[j];
    for (j = 0;  j < 4;  j++)
    {
        c[i++] = (uint8_t) s->Nc[j];
        c[i++] = (uint8_t) s->bc[j];
        c[i++] = (uint8_t) s->Mc[j];
        c[i++] = (uint8_t) s->xmaxc[j];
        for (k = 0;  k < 13;  k++)
            c[i++] = (uint8_t) s->xMc[j][k];
    }
    return 76;
}
/*- End of function --------------------------------------------------------*/

int gsm0610_pack_wav49(uint8_t code[], gsm0610_frame_t *s, int half)
{
    int i;
    int j;
    uint8_t *c;
    bitstream_state_t bs;

    c = code;
    if (half)
        bitstream_init(&bs);
    bitstream_put(&bs, &c, s->LARc[0], 6);
    bitstream_put(&bs, &c, s->LARc[1], 6);
    bitstream_put(&bs, &c, s->LARc[2], 5);
    bitstream_put(&bs, &c, s->LARc[3], 5);
    bitstream_put(&bs, &c, s->LARc[4], 4);
    bitstream_put(&bs, &c, s->LARc[5], 4);
    bitstream_put(&bs, &c, s->LARc[6], 3);
    bitstream_put(&bs, &c, s->LARc[7], 3);
    for (i = 0;  i < 4;  i++)
    {
        bitstream_put(&bs, &c, s->Nc[i], 7);
        bitstream_put(&bs, &c, s->bc[i], 2);
        bitstream_put(&bs, &c, s->Mc[i], 2);
        bitstream_put(&bs, &c, s->xmaxc[i], 6);
        for (j = 0;  j < 13;  j++)
            bitstream_put(&bs, &c, s->xMc[i][j], 3);
    }
    return (half)  ?  32  :  33;
}
/*- End of function --------------------------------------------------------*/

int gsm0610_pack_voip(uint8_t code[], gsm0610_frame_t *s)
{
    int i;
    int j;
    uint8_t *c;
    bitstream_state_t bs;

    c = code;
    bitstream_init(&bs);
    bitstream_put2(&bs, &c, GSM0610_MAGIC, 4);
    bitstream_put2(&bs, &c, s->LARc[0], 6);
    bitstream_put2(&bs, &c, s->LARc[1], 6);
    bitstream_put2(&bs, &c, s->LARc[2], 5);
    bitstream_put2(&bs, &c, s->LARc[3], 5);
    bitstream_put2(&bs, &c, s->LARc[4], 4);
    bitstream_put2(&bs, &c, s->LARc[5], 4);
    bitstream_put2(&bs, &c, s->LARc[6], 3);
    bitstream_put2(&bs, &c, s->LARc[7], 3);
    for (i = 0;  i < 4;  i++)
    {
        bitstream_put2(&bs, &c, s->Nc[i], 7);
        bitstream_put2(&bs, &c, s->bc[i], 2);
        bitstream_put2(&bs, &c, s->Mc[i], 2);
        bitstream_put2(&bs, &c, s->xmaxc[i], 6);
        for (j = 0;  j < 13;  j++)
            bitstream_put2(&bs, &c, s->xMc[i][j], 3);
    }
    return 33;
}
/*- End of function --------------------------------------------------------*/

int gsm0610_encode(gsm0610_state_t *s, uint8_t code[], const int16_t amp[], int quant)
{
    gsm0610_frame_t frame;
    uint8_t *c;
    int i;

    c = code;
    if (s->packing == GSM0610_PACKING_WAV49)
        quant <<= 1;
    for (i = 0;  i < quant;  i++)
    {
        encode_a_frame(s, &amp[i*GSM0610_FRAME_LEN], &frame);
        /*  variable    size

            LARc[0]     6
            LARc[1]     6
            LARc[2]     5
            LARc[3]     5
            LARc[4]     4
            LARc[5]     4
            LARc[6]     3
            LARc[7]     3
    
            Nc[0]       7
            bc[0]       2
            Mc[0]       2
            xmaxc[0]    6
            xMc[0]      3
            xMc[1]      3
            xMc[2]      3
            xMc[3]      3
            xMc[4]      3
            xMc[5]      3
            xMc[6]      3
            xMc[7]      3
            xMc[8]      3
            xMc[9]      3
            xMc[10]     3
            xMc[11]     3
            xMc[12]     3

            Nc[1]       7
            bc[1]       2
            Mc[1]       2
            xmaxc[1]    6
            xMc[13]     3
            xMc[14]     3
            xMc[15]     3
            xMc[16]     3
            xMc[17]     3
            xMc[18]     3
            xMc[19]     3
            xMc[20]     3
            xMc[21]     3
            xMc[22]     3
            xMc[23]     3
            xMc[24]     3
            xMc[25]     3

            Nc[2]       7
            bc[2]       2
            Mc[2]       2
            xmaxc[2]    6
            xMc[26]     3
            xMc[27]     3
            xMc[28]     3
            xMc[29]     3
            xMc[30]     3
            xMc[31]     3
            xMc[32]     3
            xMc[33]     3
            xMc[34]     3
            xMc[35]     3
            xMc[36]     3
            xMc[37]     3
            xMc[38]     3

            Nc[3]       7
            bc[3]       2
            Mc[3]       2
            xmaxc[3]    6
            xMc[39]     3
            xMc[40]     3
            xMc[41]     3
            xMc[42]     3
            xMc[43]     3
            xMc[44]     3
            xMc[45]     3
            xMc[46]     3
            xMc[47]     3
            xMc[48]     3
            xMc[49]     3
            xMc[50]     3
            xMc[51]     3
        */

        switch (s->packing)
        {
        case GSM0610_PACKING_NONE:
            c += gsm0610_pack_none(c, &frame);
            break;
        case GSM0610_PACKING_WAV49:
            s->frame_index = !s->frame_index;
            c += gsm0610_pack_wav49(c, &frame, s->frame_index);
            break;
        case GSM0610_PACKING_VOIP:
            c += gsm0610_pack_voip(c, &frame);
            break;
        }
        /*endswitch*/
    }
    /*endwhile*/
    return (int) (c - code);
}
/*- End of function --------------------------------------------------------*/
/*- End of file ------------------------------------------------------------*/
