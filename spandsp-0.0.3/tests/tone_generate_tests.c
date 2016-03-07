/*
 * SpanDSP - a series of DSP components for telephony
 *
 * tone_generate_tests.c
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
 * $Id: tone_generate_tests.c,v 1.13 2006/11/23 15:32:23 steveu Exp $
 */

/*! \page tone_generate_tests_page Tone generation tests
\section tone_generate_tests_page_sec_1 What does it do?
*/

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

#define OUTPUT_FILE_NAME    "tone_generate.wav"

int main(int argc, char *argv[])
{
    tone_gen_descriptor_t tone_desc;
    tone_gen_state_t tone_state;
    int i;
    int16_t amp[16384];
    int len;
    AFfilehandle outhandle;
    AFfilesetup filesetup;
    int outframes;

    filesetup = afNewFileSetup ();
    if (filesetup == AF_NULL_FILESETUP)
    {
    	fprintf(stderr, "    Failed to create file setup\n");
        exit(2);
    }
    afInitSampleFormat(filesetup, AF_DEFAULT_TRACK, AF_SAMPFMT_TWOSCOMP, 16);
    afInitRate(filesetup, AF_DEFAULT_TRACK, 8000.0);
    //afInitCompression(filesetup, AF_DEFAULT_TRACK, AF_COMPRESSION_G711_ALAW);
    afInitFileFormat(filesetup, AF_FILE_WAVE);
    afInitChannels(filesetup, AF_DEFAULT_TRACK, 1);

    outhandle = afOpenFile(OUTPUT_FILE_NAME, "w", filesetup);
    if (outhandle == AF_NULL_FILEHANDLE)
    {
        fprintf(stderr, "    Cannot open wave file '%s'\n", OUTPUT_FILE_NAME);
        exit(2);
    }
    
    make_tone_gen_descriptor(&tone_desc,
                             440,
                             -10,
                             620,
                             -15,
                             100,
                             200,
                             300,
                             400,
                             FALSE);
    tone_gen_init(&tone_state, &tone_desc);

    for (i = 0;  i < 1000;  i++)
    {
        len = tone_gen(&tone_state, amp, 160);
        printf("Generated %d samples\n", len);
        if (len <= 0)
            break;
        outframes = afWriteFrames(outhandle,
                                  AF_DEFAULT_TRACK,
                                  amp,
                                  len);
    }
    
    make_tone_gen_descriptor(&tone_desc,
                             350,
                             -10,
                             440,
                             -15,
                             100,
                             200,
                             300,
                             400,
                             TRUE);
    tone_gen_init(&tone_state, &tone_desc);

    for (i = 0;  i < 1000;  i++)
    {
        len = tone_gen(&tone_state, amp, 160);
        printf("Generated %d samples\n", len);
        if (len <= 0)
            break;
        outframes = afWriteFrames(outhandle,
                                  AF_DEFAULT_TRACK,
                                  amp,
                                  len);
    }
    
    make_tone_gen_descriptor(&tone_desc,
                             400,
                             -10,
                             450,
                             -10,
                             100,
                             200,
                             300,
                             400,
                             TRUE);
    tone_gen_init(&tone_state, &tone_desc);

    for (i = 0;  i < 1000;  i++)
    {
        len = tone_gen(&tone_state, amp, 160);
        printf("Generated %d samples\n", len);
        if (len <= 0)
            break;
        outframes = afWriteFrames(outhandle,
                                  AF_DEFAULT_TRACK,
                                  amp,
                                  len);
    }
    
    make_tone_gen_descriptor(&tone_desc,
                             425,
                             -10,
                             -50,
                             100,
                             100,
                             200,
                             300,
                             400,
                             TRUE);
    tone_gen_init(&tone_state, &tone_desc);

    for (i = 0;  i < 1000;  i++)
    {
        len = tone_gen(&tone_state, amp, 160);
        printf("Generated %d samples\n", len);
        if (len <= 0)
            break;
        outframes = afWriteFrames(outhandle,
                                  AF_DEFAULT_TRACK,
                                  amp,
                                  len);
    }
    
    if (afCloseFile(outhandle) != 0)
    {
        fprintf(stderr, "    Cannot close wave file '%s'\n", OUTPUT_FILE_NAME);
        exit (2);
    }
    afFreeFileSetup(filesetup);

    return  0;
}
/*- End of function --------------------------------------------------------*/
/*- End of file ------------------------------------------------------------*/
