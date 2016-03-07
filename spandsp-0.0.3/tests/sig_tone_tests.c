/*
 * SpanDSP - a series of DSP components for telephony
 *
 * test_sig_tone.c
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
 * $Id: sig_tone_tests.c,v 1.15 2006/11/19 14:07:27 steveu Exp $
 */

/*! \file */

/*! \page sig_tone_tests_page The signaling tone processor tests
\section sig_tone_tests_sec_1 What does it do?
???.

\section sig_tone_tests_sec_2 How does it work?
???.
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <stdio.h>
#include <inttypes.h>
#include <memory.h>
#if defined(HAVE_TGMATH_H)
#include <tgmath.h>
#endif
#if defined(HAVE_MATH_H)
#include <math.h>
#endif
#include <audiofile.h>
#include <tiffio.h>

#include "spandsp.h"

#define OUT_FILE_NAME   "sig_tone.wav"

static int sampleno = 0;
static int tone_1_present = 0;
static int tone_2_present = 0;
static int ping = 0;

void map_frequency_response(sig_tone_state_t *s);

static int handler(void *user_data, int what)
{
    //printf("What - %d\n", what);
    if ((what & SIG_TONE_1_CHANGE))
    {
        tone_1_present = what & SIG_TONE_1_PRESENT;
        printf("Tone 1 is %s after %d samples\n", (tone_1_present)  ?  "on"  : "off", (what >> 16) & 0xFFFF);
    }
    /*endif*/
    if ((what & SIG_TONE_2_CHANGE))
    {
        tone_2_present = what & SIG_TONE_2_PRESENT;
        printf("Tone 2 is %s after %d samples\n", (tone_2_present)  ?  "on"  : "off", (what >> 16) & 0xFFFF);
    }
    /*endif*/
    if ((what & SIG_TONE_UPDATE_REQUEST))
    {
        /* The signaling processor wants to know what to do next */
        if (sampleno < 800)
        {
            /* 100ms off-hook */
            printf("100ms off-hook - %d samples\n", 800 - sampleno);
            return 0x02 | ((800 - sampleno) << 16) | SIG_TONE_RX_PASSTHROUGH;
        }
        /*endif*/
        if (sampleno < 4800)
        {
            /* 500ms idle */
            printf("500ms idle - %d samples\n", 4800 - sampleno);
            return 0x02 | SIG_TONE_1_PRESENT | ((4800 - sampleno) << 16) | SIG_TONE_RX_PASSTHROUGH;
        }
        /*endif*/
        if (sampleno < 5600)
        {
            /* 100ms seize */
            printf("100ms seize - %d samples\n", 5600 - sampleno);
            return 0x02 | ((5600 - sampleno) << 16) | SIG_TONE_RX_PASSTHROUGH;
        }
        /*endif*/
        if (ping)
        {
            printf("33ms break - 262 samples\n");
            ping = !ping;
            return 0x02 | (262 << 16) | SIG_TONE_RX_PASSTHROUGH;
        }
        else
        {
            printf("67ms make - 528 samples\n");
            ping = !ping;
            return 0x02 |  SIG_TONE_1_PRESENT | (528 << 16) | SIG_TONE_RX_PASSTHROUGH;
        }
        /*endif*/
    }
    /*endif*/
    return 0;
}
/*- End of function --------------------------------------------------------*/

void map_frequency_response(sig_tone_state_t *s)
{
    int16_t buf[8192];
    awgn_state_t noise_source;
    int i;
    int f;
    uint32_t phase_acc;
    int32_t phase_rate;
    int32_t scaling;
    double sum;
    
    /* Things like noise don't highlight the frequency response of the high Q notch
       very well. We use a slowly swept frequency to check it. */
    awgn_init_dbm0(&noise_source, 1234567, -10.0f);
    for (f = 1;  f < 4000;  f++)
    {
        phase_rate = dds_phase_rate(f);
        scaling = dds_scaling_dbm0(-10);
        phase_acc = 0;
        for (i = 0;  i < 8192;  i++)
            buf[i] = dds_mod(&phase_acc, phase_rate, scaling, 0);
        /*endfor*/
        sig_tone_rx(s, buf, 8192);
        sum = 0.0;
        for (i = 1000;  i < 8192;  i++)
            sum += (double) buf[i]*(double) buf[i];
        /*endfor*/
        sum = sqrt(sum);
        printf("%7d %f\n", f, sum);
    }
    /*endfor*/
}
/*- End of function --------------------------------------------------------*/

int main(int argc, char *argv[])
{
    int16_t amp[160];
    int16_t out_amp[2*160];
    AFfilehandle outhandle;
    AFfilesetup filesetup;
    int outframes;
    int i;
    int rx_samples;
    int tx_samples;
    sig_tone_state_t state;
    awgn_state_t noise_source;
    
    filesetup = afNewFileSetup();
    if (filesetup == AF_NULL_FILESETUP)
    {
        fprintf(stderr, "    Failed to create file setup\n");
        exit(2);
    }
    /*endif*/
    afInitSampleFormat(filesetup, AF_DEFAULT_TRACK, AF_SAMPFMT_TWOSCOMP, 16);
    afInitRate(filesetup, AF_DEFAULT_TRACK, (float) SAMPLE_RATE);
    afInitFileFormat(filesetup, AF_FILE_WAVE);
    afInitChannels(filesetup, AF_DEFAULT_TRACK, 2);

    outhandle = afOpenFile(OUT_FILE_NAME, "w", filesetup);
    if (outhandle == AF_NULL_FILEHANDLE)
    {
        fprintf(stderr, "    Cannot create wave file '%s'\n", OUT_FILE_NAME);
        exit(2);
    }
    /*endif*/

    awgn_init_dbm0(&noise_source, 1234567, -30.0f);

    printf("2400Hz/26000Hz tests.\n");
    sig_tone_init(&state, SIG_TONE_2400HZ_2600HZ, handler, NULL);
    state.current_tx_tone |= SIG_TONE_RX_PASSTHROUGH;
    
    map_frequency_response(&state);

    for (sampleno = 0;  sampleno < 20000;  sampleno += 160)
    {
        tx_samples = 160;
        for (i = 0;  i < tx_samples;  i++)
            amp[i] = alaw_to_linear(linear_to_alaw(awgn(&noise_source)));
        /*endfor*/
        for (i = 0;  i < tx_samples;  i++)
            out_amp[2*i] = amp[i];
        /*endfor*/
        rx_samples = sig_tone_rx(&state, amp, tx_samples);
        for (i = 0;  i < rx_samples;  i++)
            out_amp[2*i + 1] = amp[i];
        /*endfor*/
        outframes = afWriteFrames(outhandle,
                                  AF_DEFAULT_TRACK,
                                  out_amp,
                                  rx_samples);
        if (outframes != rx_samples)
        {
            fprintf(stderr, "    Error writing wave file\n");
            exit(2);
        }
        /*endif*/
    }

    printf("2280Hz tests.\n");
    sig_tone_init(&state, SIG_TONE_2280HZ, handler, NULL);
    state.current_tx_tone |= SIG_TONE_RX_PASSTHROUGH;
    
    map_frequency_response(&state);

    for (sampleno = 0;  sampleno < 20000;  sampleno += 160)
    {
        memset(amp, 0, sizeof(int16_t)*160);
        tx_samples = sig_tone_tx(&state, amp, 160);
        for (i = 0;  i < tx_samples;  i++)
            out_amp[2*i] = amp[i];
        /*endfor*/
        rx_samples = sig_tone_rx(&state, amp, tx_samples);
        for (i = 0;  i < rx_samples;  i++)
            out_amp[2*i + 1] = amp[i];
        /*endfor*/
        outframes = afWriteFrames(outhandle,
                                  AF_DEFAULT_TRACK,
                                  out_amp,
                                  rx_samples);
        if (outframes != rx_samples)
        {
            fprintf(stderr, "    Error writing wave file\n");
            exit(2);
        }
        /*endif*/
    }
    /*endfor*/
    if (afCloseFile(outhandle) != 0)
    {
        fprintf(stderr, "    Cannot close wave file '%s'\n", OUT_FILE_NAME);
        exit(2);
    }
    /*endif*/
    afFreeFileSetup(filesetup);
    
    printf("Tests completed.\n");
    return  0;
}
/*- End of function --------------------------------------------------------*/
/*- End of file ------------------------------------------------------------*/
