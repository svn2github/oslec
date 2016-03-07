/*
 * SpanDSP - a series of DSP components for telephony
 *
 * g168tests.c - Tests of the "test equipment" (filters, etc.) specified
 *               in G.168. This code is only for checking out the tools,
 *               not for testing an echo cancellor.
 *
 * Written by Steve Underwood <steveu@coppice.org>
 *
 * Copyright (C) 2001 Steve Underwood
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
 * $Id: g168_tests.c,v 1.10 2006/11/23 15:48:09 steveu Exp $
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <inttypes.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#if defined(HAVE_TGMATH_H)
#include <tgmath.h>
#endif
#if defined(HAVE_MATH_H)
#include <math.h>
#endif
#include <tiffio.h>

#include "spandsp.h"
#include "spandsp/g168models.h"

#define FALSE 0
#define TRUE (!FALSE)

int main (int argc, char *argv[])
{
    int f;
    int i;
    int16_t amp[8000];
    int signal;
    int power[10];
    tone_gen_descriptor_t tone_desc;
    tone_gen_state_t tone_state;
    fir32_state_t line_model_d2;
    fir32_state_t line_model_d3;
    fir32_state_t line_model_d4;
    fir32_state_t line_model_d5;
    fir32_state_t line_model_d6;
    fir32_state_t line_model_d7;
    fir32_state_t line_model_d8;
    fir_float_state_t level_measurement_bp;

    fir32_create(&line_model_d2,
                 line_model_d2_coeffs,
                 sizeof(line_model_d2_coeffs)/sizeof(int32_t));
    fir32_create(&line_model_d3,
                 line_model_d3_coeffs,
                 sizeof(line_model_d3_coeffs)/sizeof(int32_t));
    fir32_create(&line_model_d4,
                 line_model_d4_coeffs,
                 sizeof(line_model_d4_coeffs)/sizeof(int32_t));
    fir32_create(&line_model_d5,
                 line_model_d5_coeffs,
                 sizeof(line_model_d5_coeffs)/sizeof(int32_t));
    fir32_create(&line_model_d6,
                 line_model_d6_coeffs,
                 sizeof(line_model_d6_coeffs)/sizeof(int32_t));
    fir32_create(&line_model_d7,
                 line_model_d7_coeffs,
                 sizeof(line_model_d7_coeffs)/sizeof(int32_t));
    fir32_create(&line_model_d8,
                 line_model_d8_coeffs,
                 sizeof(line_model_d8_coeffs)/sizeof(int32_t));
    fir_float_create(&level_measurement_bp,
                     level_measurement_bp_coeffs,
                     sizeof(level_measurement_bp_coeffs)/sizeof(float));

    for (f = 10;  f < 4000;  f++)
    {
         make_tone_gen_descriptor(&tone_desc,
                                  f,
                                  -10,
                                  0,
                                  0,
                                  1,
                                  0,
                                  0,
                                  0,
                                  TRUE);
        tone_gen_init(&tone_state, &tone_desc);
        tone_gen(&tone_state, amp, 8000);
        memset(power, '\0', sizeof (power));
        for (i = 0;  i < 800;  i++)
        {
            signal = fir32(&line_model_d2, amp[i]);
            power[0] += ((abs(signal) - power[0]) >> 5);
            signal = fir32(&line_model_d3, amp[i]);
            power[1] += ((abs(signal) - power[1]) >> 5);
            signal = fir32(&line_model_d4, amp[i]);
            power[2] += ((abs(signal) - power[2]) >> 5);
            signal = fir32(&line_model_d5, amp[i]);
            power[3] += ((abs(signal) - power[3]) >> 5);
            signal = fir32(&line_model_d6, amp[i]);
            power[4] += ((abs(signal) - power[4]) >> 5);
            signal = fir32(&line_model_d7, amp[i]);
            power[5] += ((abs(signal) - power[5]) >> 5);
            signal = fir32(&line_model_d8, amp[i]);
            power[6] += ((abs(signal) - power[6]) >> 5);
            signal = fir_float(&level_measurement_bp, amp[i]);
            power[7] += ((abs(signal) - power[7]) >> 5);
        }
        printf("%d %d %d %d %d %d %d %d %d\n",
               f,
               power[0],
               power[1],
               power[2],
               power[3],
               power[4],
               power[5],
               power[6],
               power[7]);
    }
    
    for (i = 0;  i < (int) (sizeof(css_c1)/sizeof(css_c1[0]));  i++)
        printf("%d\n", css_c1[i]);
    printf("\n");
    for (i = 0;  i < (int) (sizeof(css_c1)/sizeof(css_c3[0]));  i++)
        printf("%d\n", css_c3[i]);
    return  0;
}
/*- End of function --------------------------------------------------------*/
/*- End of file ------------------------------------------------------------*/
