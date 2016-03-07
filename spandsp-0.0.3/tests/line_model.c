/*
 * SpanDSP - a series of DSP components for telephony
 *
 * line_model.c - Model a telephone line.
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
 * $Id: line_model.c,v 1.21 2006/11/19 14:07:27 steveu Exp $
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <unistd.h>
#include <inttypes.h>
#include <string.h>
#include <time.h>
#include <stdio.h>
#include <fcntl.h>
#include <audiofile.h>
#include <tiffio.h>
#if defined(HAVE_TGMATH_H)
#include <tgmath.h>
#endif
#if defined(HAVE_MATH_H)
#define GEN_CONST
#include <math.h>
#endif

#include "spandsp.h"
#include "spandsp/g168models.h"

#include "test_utils.h"
#include "line_model.h"
#include "line_models.h"

#if !defined(NULL)
#define NULL (void *) 0
#endif

float null_line_model[] =
{
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        1.0
};

static float *models[] =
{
    null_line_model,        /* 0 */
    proakis_line_model,
    ad_1_edd_1_model,
    ad_1_edd_2_model,
    ad_1_edd_3_model,
    ad_5_edd_1_model,       /* 5 */
    ad_5_edd_2_model,
    ad_5_edd_3_model,
    ad_6_edd_1_model,
    ad_6_edd_2_model,
    ad_6_edd_3_model,       /* 10 */
    ad_7_edd_1_model,
    ad_7_edd_2_model,
    ad_7_edd_3_model,
    ad_8_edd_1_model,
    ad_8_edd_2_model,       /* 15 */
    ad_8_edd_3_model,
    ad_9_edd_1_model,
    ad_9_edd_2_model,
    ad_9_edd_3_model
};

static float calc_near_line_filter(one_way_line_model_state_t *s, float v)
{
    float sum;
    int j;
    int p;

    /* Add the sample in the filter buffer */
    p = s->near_buf_ptr;
    s->near_buf[p] = v;
    if (++p == s->near_filter_len)
        p = 0;
    s->near_buf_ptr = p;
    
    /* Apply the filter */
    sum = 0;
    for (j = 0;  j < s->near_filter_len;  j++)
    {
        sum += s->near_filter[j]*s->near_buf[p];
        if (++p >= s->near_filter_len)
            p = 0;
    }
    
    /* Add noise */
    sum += awgn(&s->near_noise);
    
    return sum;
}
/*- End of function --------------------------------------------------------*/

static float calc_far_line_filter(one_way_line_model_state_t *s, float v)
{
    float sum;
    int j;
    int p;

    /* Add the sample in the filter buffer */
    p = s->far_buf_ptr;
    s->far_buf[p] = v;
    if (++p == s->far_filter_len)
        p = 0;
    s->far_buf_ptr = p;
    
    /* Apply the filter */
    sum = 0;
    for (j = 0;  j < s->far_filter_len;  j++)
    {
        sum += s->far_filter[j]*s->far_buf[p];
        if (++p >= s->far_filter_len)
            p = 0;
    }
    
    /* Add noise */
    sum += awgn(&s->far_noise);

    return sum;
}
/*- End of function --------------------------------------------------------*/

void one_way_line_model(one_way_line_model_state_t *s, 
                        int16_t *output,
                        const int16_t *input,
                        int samples)
{
    int i;
    float in;
    float out;
    float out1;
    int16_t amp[1];

    /* The path being modelled is:
        terminal
          | < hybrid
          |
          | < noise and filtering
          |
          | < hybrid
         CO
          |
          | < A-law distortion + bulk delay
          |
         CO
          | < hybrid
          |
          | < noise and filtering
          |
          | < hybrid
        terminal
     */
    for (i = 0;  i < samples;  i++)
    {
        in = input[i];

        /* Near end analogue section */
        
        /* Line model filters & noise */
        out = calc_near_line_filter(s, in);
    
        /* Long distance digital section */

        amp[0] = out;
        codec_munge(s->munge, amp, 1);
        out = amp[0];
        /* Introduce the bulk delay of the long distance link. */
        out1 = s->bulk_delay_buf[s->bulk_delay_ptr];
        s->bulk_delay_buf[s->bulk_delay_ptr] = out;
        out = out1;
        if (++s->bulk_delay_ptr >= s->bulk_delay)
            s->bulk_delay_ptr = 0;

        /* Far end analogue section */
        
        /* Line model filters & noise */
        out = calc_far_line_filter(s, out);
    
        output[i] = out;
    }
}
/*- End of function --------------------------------------------------------*/

void both_ways_line_model(both_ways_line_model_state_t *s, 
                          int16_t *output1,
                          const int16_t *input1,
                          int16_t *output2,
                          const int16_t *input2,
                          int samples)
{
    int i;
    float in1;
    float in2;
    float out1;
    float out2;
    float tmp1;
    float tmp2;
    int16_t amp[1];

    /* The path being modelled is:
        terminal
          | < hybrid echo
          |
          | < noise and filtering
          |
          | < hybrid echo
         CO
          |
          | < A-law distortion + bulk delay
          |
         CO
          | < hybrid echo
          |
          | < noise and filtering
          |
          | < hybrid echo
        terminal
     */
    for (i = 0;  i < samples;  i++)
    {
        in1 = input1[i];
        in2 = input2[i];

        /* Near end analogue sections */
        /* Echo from each terminal's CO hybrid */
        tmp1 = in1 + s->fout2*s->line1.near_co_hybrid_echo;
        tmp2 = in2 + s->fout1*s->line2.near_co_hybrid_echo;

        /* Line model filters & noise */
        s->fout1 = calc_near_line_filter(&s->line1, tmp1);
        s->fout2 = calc_near_line_filter(&s->line2, tmp2);

        /* Long distance digital section */

        /* Introduce distortion due to A-law or u-law munging. */
        amp[0] = s->fout1;
        codec_munge(s->line1.munge, amp, 1);
        s->fout1 = amp[0];

        amp[0] = s->fout2;
        codec_munge(s->line2.munge, amp, 1);
        s->fout2 = amp[0];

        /* Introduce the bulk delay of the long distance digital link. */
        out1 = s->line1.bulk_delay_buf[s->line1.bulk_delay_ptr];
        s->line1.bulk_delay_buf[s->line1.bulk_delay_ptr] = s->fout1;
        s->fout1 = out1;
        if (++s->line1.bulk_delay_ptr >= s->line1.bulk_delay)
            s->line1.bulk_delay_ptr = 0;

        out2 = s->line2.bulk_delay_buf[s->line2.bulk_delay_ptr];
        s->line2.bulk_delay_buf[s->line2.bulk_delay_ptr] = s->fout2;
        s->fout2 = out2;
        if (++s->line2.bulk_delay_ptr >= s->line2.bulk_delay)
            s->line2.bulk_delay_ptr = 0;

        /* Far end analogue sections */

        /* Echo from each terminal's own hybrid */
        out1 += in2*s->line1.far_cpe_hybrid_echo;
        out2 += in1*s->line2.far_cpe_hybrid_echo;

        /* Line model filters & noise */
        out1 = calc_far_line_filter(&s->line1, out1);
        out2 = calc_far_line_filter(&s->line2, out2);

        output1[i] = fsaturate(out1);
        output2[i] = fsaturate(out2);
    }
}
/*- End of function --------------------------------------------------------*/

one_way_line_model_state_t *one_way_line_model_init(int model, float noise, int codec)
{
    one_way_line_model_state_t *s;

    if ((s = (one_way_line_model_state_t *) malloc(sizeof(*s))) == NULL)
        return NULL;
    memset(s, 0, sizeof(*s));

    s->bulk_delay = 8;
    s->bulk_delay_ptr = 0;

    s->munge = codec_munge_init(codec);

    s->near_filter = models[model];
    s->near_filter_len = 129;

    s->far_filter = models[model];
    s->far_filter_len = 129;

    awgn_init_dbm0(&s->near_noise, 1234567, noise);
    awgn_init_dbm0(&s->far_noise, 1234567, noise);
    return s;
}
/*- End of function --------------------------------------------------------*/

int one_way_line_model_release(one_way_line_model_state_t *s)
{
    free(s);
    return 0;
}
/*- End of function --------------------------------------------------------*/

both_ways_line_model_state_t *both_ways_line_model_init(int model1,
                                                        float noise1,
                                                        int model2,
                                                        float noise2,
                                                        int codec)
{
    float echo_level;
    both_ways_line_model_state_t *s;

    if ((s = (both_ways_line_model_state_t *) malloc(sizeof(*s))) == NULL)
        return NULL;
    memset(s, 0, sizeof(*s));

    s->line1.munge = codec_munge_init(codec);
    s->line2.munge = codec_munge_init(codec);

    s->line1.bulk_delay = 8;
    s->line2.bulk_delay = 8;

    s->line1.bulk_delay_ptr = 0;
    s->line2.bulk_delay_ptr = 0;

    s->line1.near_filter = models[model1];
    s->line1.near_filter_len = 129;
    s->line2.near_filter = models[model2];
    s->line2.near_filter_len = 129;

    s->line1.far_filter = models[model1];
    s->line1.far_filter_len = 129;
    s->line2.far_filter = models[model2];
    s->line2.far_filter_len = 129;

    awgn_init_dbm0(&s->line1.near_noise, 1234567, noise1);
    awgn_init_dbm0(&s->line2.near_noise, 7654321, noise2);

    awgn_init_dbm0(&s->line1.far_noise, 1234567, noise1);
    awgn_init_dbm0(&s->line2.far_noise, 7654321, noise2);

    /* Echos */
    echo_level = -15; /* in dB */
    s->line1.near_co_hybrid_echo = pow(10, echo_level/20.0);
    s->line2.near_co_hybrid_echo = pow(10, echo_level/20.0);
    s->line1.near_cpe_hybrid_echo = pow(10, echo_level/20.0);
    s->line2.near_cpe_hybrid_echo = pow(10, echo_level/20.0);
    
    return s;
}
/*- End of function --------------------------------------------------------*/

int both_ways_line_model_release(both_ways_line_model_state_t *s)
{
    free(s);
    return 0;
}
/*- End of function --------------------------------------------------------*/
/*- End of file ------------------------------------------------------------*/
