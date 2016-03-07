/*
 * SpanDSP - a series of DSP components for telephony
 *
 * modem_connect_tones_tests.c
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
 * $Id: modem_connect_tones_tests.c,v 1.5 2006/11/19 14:07:27 steveu Exp $
 */

/*! \page modem_connect_tones_tests_page Modem connect tones tests
\section modem_connect_tones_rx_tests_page_sec_1 What does it do?
These tests...
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <inttypes.h>
#include <stdio.h>
#if defined(HAVE_TGMATH_H)
#include <tgmath.h>
#endif
#if defined(HAVE_MATH_H)
#include <math.h>
#endif
#include <fcntl.h>
#include <string.h>
#include <audiofile.h>
#include <tiffio.h>

#include "spandsp.h"

#define OUTPUT_FILE_NAME    "modem_connect_tones.wav"

#define BELLCORE_DIR        "../itutests/bellcore/"

#define FALSE 0
#define TRUE (!FALSE)

const char *bellcore_files[] =
{
    BELLCORE_DIR "tr-tsy-00763-1.wav",
    BELLCORE_DIR "tr-tsy-00763-2.wav",
    BELLCORE_DIR "tr-tsy-00763-3.wav",
    BELLCORE_DIR "tr-tsy-00763-4.wav",
    BELLCORE_DIR "tr-tsy-00763-5.wav",
    BELLCORE_DIR "tr-tsy-00763-6.wav",
    ""
};

int main(int argc, char *argv[])
{
    int i;
    int j;
    int pitch;
    int level;
    int16_t amp[8000];
    modem_connect_tones_rx_state_t cng_rx;
    modem_connect_tones_rx_state_t ced_rx;
    modem_connect_tones_rx_state_t ec_dis_rx;
    modem_connect_tones_tx_state_t ec_dis_tx;
    awgn_state_t chan_noise_source;
    int hits;
    AFfilehandle inhandle;
    AFfilehandle outhandle;
    AFfilesetup filesetup;
    int outframes;
    int frames;
    int samples;
    int hit;
    int false_hit;
    int false_miss;
    float x;
    tone_gen_descriptor_t tone_desc;
    tone_gen_state_t tone_tx;

    if ((filesetup = afNewFileSetup()) == AF_NULL_FILESETUP)
    {
        fprintf(stderr, "    Failed to create file setup\n");
        exit(2);
    }
    afInitSampleFormat(filesetup, AF_DEFAULT_TRACK, AF_SAMPFMT_TWOSCOMP, 16);
    afInitRate(filesetup, AF_DEFAULT_TRACK, (float) SAMPLE_RATE);
    afInitFileFormat(filesetup, AF_FILE_WAVE);
    afInitChannels(filesetup, AF_DEFAULT_TRACK, 1);

    if ((outhandle = afOpenFile(OUTPUT_FILE_NAME, "w", filesetup)) == AF_NULL_FILEHANDLE)
    {
        fprintf(stderr, "    Cannot create wave file '%s'\n", OUTPUT_FILE_NAME);
        exit(2);
    }

    printf("Test 1a: CNG generation to a file\n");
    modem_connect_tones_tx_init(&ec_dis_tx, MODEM_CONNECT_TONES_FAX_CNG);
    for (i = 0;  i < 1000;  i++)
    {
        samples = modem_connect_tones_tx(&ec_dis_tx, amp, 160);
        outframes = afWriteFrames(outhandle,
                                  AF_DEFAULT_TRACK,
                                  amp,
                                  samples);
        if (outframes != samples)
        {
            fprintf(stderr, "    Error writing wave file\n");
            exit(2);
        }
    }

    printf("Test 1b: CED generation to a file\n");
    modem_connect_tones_tx_init(&ec_dis_tx, MODEM_CONNECT_TONES_FAX_CED);
    for (i = 0;  i < 1000;  i++)
    {
        samples = modem_connect_tones_tx(&ec_dis_tx, amp, 160);
        outframes = afWriteFrames(outhandle,
                                  AF_DEFAULT_TRACK,
                                  amp,
                                  samples);
        if (outframes != samples)
        {
            fprintf(stderr, "    Error writing wave file\n");
            exit(2);
        }
    }

    printf("Test 1c: Modulated EC-disable generation to a file\n");
    /* Some with modulation */
    modem_connect_tones_tx_init(&ec_dis_tx, MODEM_CONNECT_TONES_EC_DISABLE_MOD);
    for (i = 0;  i < 1000;  i++)
    {
        samples = modem_connect_tones_tx(&ec_dis_tx, amp, 160);
        outframes = afWriteFrames(outhandle,
                                  AF_DEFAULT_TRACK,
                                  amp,
                                  samples);
        if (outframes != samples)
        {
            fprintf(stderr, "    Error writing wave file\n");
            exit(2);
        }
    }

    printf("Test 1d: EC-disable generation to a file\n");
    /* Some without modulation */
    modem_connect_tones_tx_init(&ec_dis_tx, MODEM_CONNECT_TONES_EC_DISABLE);
    for (i = 0;  i < 1000;  i++)
    {
        samples = modem_connect_tones_tx(&ec_dis_tx, amp, 160);
        outframes = afWriteFrames(outhandle,
                                  AF_DEFAULT_TRACK,
                                  amp,
                                  samples);
        if (outframes != samples)
        {
            fprintf(stderr, "    Error writing wave file\n");
            exit(2);
        }
    }
    if (afCloseFile(outhandle) != 0)
    {
        printf("    Cannot close wave file '%s'\n", OUTPUT_FILE_NAME);
        exit(2);
    }
    
    printf("Test 2a: CNG detection with frequency\n");
    awgn_init_dbm0(&chan_noise_source, 7162534, -50.0f);
    false_hit = FALSE;
    false_miss = FALSE;
    for (pitch = 600;  pitch < 1600;  pitch++)
    {
        make_tone_gen_descriptor(&tone_desc,
                                 pitch,
                                 -11,
                                 0,
                                 0,
                                 425,
                                 3000,
                                 0,
                                 0,
                                 TRUE);
        tone_gen_init(&tone_tx, &tone_desc);

        modem_connect_tones_rx_init(&cng_rx, MODEM_CONNECT_TONES_FAX_CNG, NULL, NULL);
        for (i = 0;  i < 500;  i++)
        {
            samples = tone_gen(&tone_tx, amp, 160);
            for (j = 0;  j < samples;  j++)
                amp[j] += awgn(&chan_noise_source);
            /*endfor*/
            modem_connect_tones_rx(&cng_rx, amp, samples);
        }
        hit = modem_connect_tones_rx_get(&cng_rx);
        if (pitch < (1100 - 70)  ||  pitch > (1100 + 70))
        {
            if (hit)
                false_hit = TRUE;
        }
        else if (pitch > (1100 - 50)  &&  pitch < (1100 + 50))
        {
            if (!hit)
                false_miss = TRUE;
        }
        if (hit)
            printf("Detected at %5dHz %12d %12d %d\n", pitch, cng_rx.channel_level, cng_rx.notch_level, hit);
    }
    if (false_hit  ||  false_miss)
    {
        printf("Test failed.\n");
        exit(2);
    }

    printf("Test 2b: CED detection with frequency\n");
    awgn_init_dbm0(&chan_noise_source, 7162534, -50.0f);
    false_hit = FALSE;
    false_miss = FALSE;
    for (pitch = 1600;  pitch < 2600;  pitch++)
    {
        make_tone_gen_descriptor(&tone_desc,
                                 pitch,
                                 -11,
                                 0,
                                 0,
                                 2600,
                                 0,
                                 0,
                                 0,
                                 FALSE);
        tone_gen_init(&tone_tx, &tone_desc);

        modem_connect_tones_rx_init(&ced_rx, MODEM_CONNECT_TONES_FAX_CED, NULL, NULL);
        for (i = 0;  i < 500;  i++)
        {
            samples = tone_gen(&tone_tx, amp, 160);
            for (j = 0;  j < samples;  j++)
                amp[j] += awgn(&chan_noise_source);
            /*endfor*/
            modem_connect_tones_rx(&ced_rx, amp, samples);
        }
        hit = modem_connect_tones_rx_get(&ced_rx);
        if (pitch < (2100 - 70)  ||  pitch > (2100 + 70))
        {
            if (hit)
                false_hit = TRUE;
        }
        else if (pitch > (2100 - 50)  &&  pitch < (2100 + 50))
        {
            if (!hit)
                false_miss = TRUE;
        }
        if (hit)
            printf("Detected at %5dHz %12d %12d %d\n", pitch, ced_rx.channel_level, ced_rx.notch_level, hit);
    }
    if (false_hit  ||  false_miss)
    {
        printf("Test failed.\n");
        exit(2);
    }

    printf("Test 2c: EC disable detection with frequency\n");
    awgn_init_dbm0(&chan_noise_source, 7162534, -50.0f);
    false_hit = FALSE;
    false_miss = FALSE;
    for (pitch = 2000;  pitch < 2200;  pitch++)
    {
        /* Use the transmitter to test the receiver */
        modem_connect_tones_tx_init(&ec_dis_tx, MODEM_CONNECT_TONES_EC_DISABLE);
        /* Fudge things for the test */
        ec_dis_tx.tone_phase_rate = dds_phase_rate(pitch);
        ec_dis_tx.level = dds_scaling_dbm0(-25);
        modem_connect_tones_rx_init(&ec_dis_rx, MODEM_CONNECT_TONES_EC_DISABLE, NULL, NULL);
        for (i = 0;  i < 500;  i++)
        {
            samples = modem_connect_tones_tx(&ec_dis_tx, amp, 160);
            for (j = 0;  j < samples;  j++)
                amp[j] += awgn(&chan_noise_source);
            /*endfor*/
            modem_connect_tones_rx(&ec_dis_rx, amp, samples);
        }
        hit = modem_connect_tones_rx_get(&ec_dis_rx);
        if (pitch < (2100 - 70)  ||  pitch > (2100 + 70))
        {
            if (hit)
                false_hit = TRUE;
        }
        else if (pitch > (2100 - 50)  &&  pitch < (2100 + 50))
        {
            if (!hit)
                false_miss = TRUE;
        }
        if (hit)
            printf("Detected at %5dHz %12d %12d %d\n", pitch, ec_dis_rx.channel_level, ec_dis_rx.notch_level, hit);
    }
    if (false_hit  ||  false_miss)
    {
        printf("Test failed.\n");
        exit(2);
    }

    printf("Test 3a: CNG detection with level\n");
    awgn_init_dbm0(&chan_noise_source, 7162534, -60.0f);
    false_hit = FALSE;
    false_miss = FALSE;
    for (pitch = 1062;  pitch <= 1138;  pitch += 2*38)
    {
        for (level = 0;  level >= -43;  level--)
        {
            make_tone_gen_descriptor(&tone_desc,
                                     pitch,
                                     level,
                                     0,
                                     0,
                                     500,
                                     3000,
                                     0,
                                     0,
                                     TRUE);
            tone_gen_init(&tone_tx, &tone_desc);

            modem_connect_tones_rx_init(&cng_rx, MODEM_CONNECT_TONES_FAX_CNG, NULL, NULL);
            for (i = 0;  i < 500;  i++)
            {
                samples = tone_gen(&tone_tx, amp, 160);
                for (j = 0;  j < samples;  j++)
                    amp[j] += awgn(&chan_noise_source);
                /*endfor*/
                modem_connect_tones_rx(&cng_rx, amp, samples);
            }
            hit = modem_connect_tones_rx_get(&cng_rx);
            if (level < -43)
            {
                if (hit)
                    false_hit = TRUE;
            }
            else if (level > -43)
            {
                if (!hit)
                    false_miss = TRUE;
            }
            if (hit)
                printf("Detected at %5dHz %ddB %12d %12d %d\n", pitch, level, cng_rx.channel_level, cng_rx.notch_level, hit);
        }
    }
    if (false_hit  ||  false_miss)
    {
        printf("Test failed.\n");
        exit(2);
    }

    printf("Test 3b: CED detection with level\n");
    awgn_init_dbm0(&chan_noise_source, 7162534, -60.0f);
    false_hit = FALSE;
    false_miss = FALSE;
    for (pitch = 2062;  pitch <= 2138;  pitch += 2*38)
    {
        for (level = 0;  level >= -43;  level--)
        {
            make_tone_gen_descriptor(&tone_desc,
                                     pitch,
                                     level,
                                     0,
                                     0,
                                     2600,
                                     0,
                                     0,
                                     0,
                                     FALSE);
            tone_gen_init(&tone_tx, &tone_desc);

            modem_connect_tones_rx_init(&ced_rx, MODEM_CONNECT_TONES_FAX_CED, NULL, NULL);
            for (i = 0;  i < 500;  i++)
            {
                samples = tone_gen(&tone_tx, amp, 160);
                for (j = 0;  j < samples;  j++)
                    amp[j] += awgn(&chan_noise_source);
                /*endfor*/
                modem_connect_tones_rx(&ced_rx, amp, samples);
            }
            hit = modem_connect_tones_rx_get(&ced_rx);
            if (level < -43)
            {
                if (hit)
                    false_hit = TRUE;
            }
            else if (level > -43)
            {
                if (!hit)
                    false_miss = TRUE;
            }
            if (hit)
                printf("Detected at %5dHz %ddB %12d %12d %d\n", pitch, level, ced_rx.channel_level, ced_rx.notch_level, hit);
        }
    }
    if (false_hit  ||  false_miss)
    {
        printf("Test failed.\n");
        exit(2);
    }

    /* Talk-off test */
    /* Here we use the BellCore talk off test tapes, intended for DTMF detector
       testing. Presumably they should also have value here, but I am not sure.
       If those voice snippets were chosen to be tough on DTMF detectors, they
       might go easy on detectors looking for different pitches. However, the
       Mitel DTMF test tape is known (the hard way) to exercise 2280Hz tone
       detectors quite well. */
    printf("Test 2: Talk-off test\n");
    modem_connect_tones_rx_init(&cng_rx, MODEM_CONNECT_TONES_FAX_CNG, NULL, NULL);
    modem_connect_tones_rx_init(&ced_rx, MODEM_CONNECT_TONES_FAX_CED, NULL, NULL);
    modem_connect_tones_rx_init(&ec_dis_rx, MODEM_CONNECT_TONES_EC_DISABLE, NULL, NULL);
    hits = 0;
    for (j = 0;  bellcore_files[j][0];  j++)
    {
        if ((inhandle = afOpenFile(bellcore_files[j], "r", 0)) == AF_NULL_FILEHANDLE)
        {
            printf ("    Cannot open speech file '%s'\n", bellcore_files[j]);
            exit (2);
        }
        if ((x = afGetFrameSize(inhandle, AF_DEFAULT_TRACK, 1)) != 2.0)
        {
            printf ("    Unexpected frame size in speech file '%s'\n", bellcore_files[j]);
            exit (2);
        }
        if ((x = afGetRate(inhandle, AF_DEFAULT_TRACK)) != (float) SAMPLE_RATE)
        {
            printf("    Unexpected sample rate in speech file '%s'\n", bellcore_files[j]);
            exit(2);
        }
        if ((x = afGetChannels(inhandle, AF_DEFAULT_TRACK)) != 1.0)
        {
            printf("    Unexpected number of channels in speech file '%s'\n", bellcore_files[j]);
            exit(2);
        }

        hits = 0;
        while ((frames = afReadFrames(inhandle, AF_DEFAULT_TRACK, amp, 8000)))
        {
            modem_connect_tones_rx(&cng_rx, amp, frames);
            modem_connect_tones_rx(&ced_rx, amp, frames);
            modem_connect_tones_rx(&ec_dis_rx, amp, frames);
            if (modem_connect_tones_rx_get(&cng_rx))
            {
                /* This is not a true measure of hits, as there might be more
                   than one in a block of data. However, since the only good
                   result is no hits, this approximation is OK. */
                hits++;
                modem_connect_tones_rx_init(&cng_rx, MODEM_CONNECT_TONES_FAX_CNG, NULL, NULL);
            }
            if (modem_connect_tones_rx_get(&ced_rx))
            {
                hits++;
                modem_connect_tones_rx_init(&ced_rx, MODEM_CONNECT_TONES_FAX_CED, NULL, NULL);
            }
            if (modem_connect_tones_rx_get(&ec_dis_rx))
            {
                hits++;
                modem_connect_tones_rx_init(&ec_dis_rx, MODEM_CONNECT_TONES_EC_DISABLE, NULL, NULL);
            }
        }
        if (afCloseFile(inhandle) != 0)
        {
            printf("    Cannot close speech file '%s'\n", bellcore_files[j]);
            exit(2);
        }
        printf("    File %d gave %d false hits.\n", j + 1, hits);
    }
    if (hits)
    {
        printf("Test failed.\n");
        exit(2);
    }

    printf("Tests passed.\n");
    return  0;
}
/*- End of function --------------------------------------------------------*/
/*- End of file ------------------------------------------------------------*/
