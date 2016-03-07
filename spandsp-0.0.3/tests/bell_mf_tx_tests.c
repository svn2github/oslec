/*
 * SpanDSP - a series of DSP components for telephony
 *
 * bell_mf_tx_tests.c - Test the Bell MF generator.
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
 * $Id: bell_mf_tx_tests.c,v 1.4 2006/11/19 14:07:26 steveu Exp $
 */

/*! \file */

/*! \page bell_mf_tx_tests_page Bell MF generation tests
\section bell_mf_tx_tests_page_sec_1 What does it do?
???.

\section bell_mf_tx_tests_page_sec_2 How does it work?
???.
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <inttypes.h>
#include <string.h>
#if defined(HAVE_TGMATH_H)
#include <tgmath.h>
#endif
#if defined(HAVE_MATH_H)
#include <math.h>
#endif
#include <stdio.h>
#include <time.h>
#include <fcntl.h>
#include <audiofile.h>
#include <tiffio.h>

#include "spandsp.h"

#define OUTPUT_FILE_NAME    "bell_mf.wav"

int main(int argc, char *argv[])
{
    bell_mf_tx_state_t gen;
    int16_t amp[16384];
    int len;
    AFfilehandle outhandle;
    AFfilesetup filesetup;
    int outframes;
    int add_digits;

    filesetup = afNewFileSetup();
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

    bell_mf_tx_init(&gen);
    len = bell_mf_tx(&gen, amp, 16384);
    printf("Generated %d samples\n", len);
    outframes = afWriteFrames(outhandle,
                              AF_DEFAULT_TRACK,
                              amp,
                              len);
    if (bell_mf_tx_put(&gen, "123"))
        printf("Ooops\n");
    len = bell_mf_tx(&gen, amp, 16384);
    printf("Generated %d samples\n", len);
    outframes = afWriteFrames(outhandle,
                              AF_DEFAULT_TRACK,
                              amp,
                              len);
    if (bell_mf_tx_put(&gen, "456"))
        printf("Ooops\n");
    len = bell_mf_tx(&gen, amp, 160);
    printf("Generated %d samples\n", len);
    outframes = afWriteFrames(outhandle,
                              AF_DEFAULT_TRACK,
                              amp,
                              len);
    if (bell_mf_tx_put(&gen, "789"))
        printf("Ooops\n");
    len = bell_mf_tx(&gen, amp, 160);
    printf("Generated %d samples\n", len);
    outframes = afWriteFrames(outhandle,
                              AF_DEFAULT_TRACK,
                              amp,
                              len);
    if (bell_mf_tx_put(&gen, "*#"))
        printf("Ooops\n");
    len = bell_mf_tx(&gen, amp, 160);
    printf("Generated %d samples\n", len);
    outframes = afWriteFrames(outhandle,
                              AF_DEFAULT_TRACK,
                              amp,
                              len);
    add_digits = 1;
    do
    {
        len = bell_mf_tx(&gen, amp, 160);
        printf("Generated %d samples\n", len);
        if (len > 0)
        {
            outframes = afWriteFrames(outhandle,
                                      AF_DEFAULT_TRACK,
                                      amp,
                                      len);
        }
        if (add_digits)
        {
            if (bell_mf_tx_put(&gen, "1234567890"))
            {
                printf("Digit buffer full\n");
                add_digits = 0;
            }
        }
    }
    while (len > 0);

    bell_mf_tx_init(&gen);
    len = bell_mf_tx(&gen, amp, 16384);
    printf("Generated %d samples\n", len);
    outframes = afWriteFrames(outhandle,
                              AF_DEFAULT_TRACK,
                              amp,
                              len);
    if (bell_mf_tx_put(&gen, "123"))
        printf("Ooops\n");
    len = bell_mf_tx(&gen, amp, 16384);
    printf("Generated %d samples\n", len);
    outframes = afWriteFrames(outhandle,
                              AF_DEFAULT_TRACK,
                              amp,
                              len);
    if (bell_mf_tx_put(&gen, "456"))
        printf("Ooops\n");
    len = bell_mf_tx(&gen, amp, 160);
    printf("Generated %d samples\n", len);
    outframes = afWriteFrames(outhandle,
                              AF_DEFAULT_TRACK,
                              amp,
                              len);
    if (bell_mf_tx_put(&gen, "789"))
        printf("Ooops\n");
    len = bell_mf_tx(&gen, amp, 160);
    printf("Generated %d samples\n", len);
    outframes = afWriteFrames(outhandle,
                              AF_DEFAULT_TRACK,
                              amp,
                              len);
    if (bell_mf_tx_put(&gen, "0*#"))
        printf("Ooops\n");
    len = bell_mf_tx(&gen, amp, 160);
    printf("Generated %d samples\n", len);
    outframes = afWriteFrames(outhandle,
                              AF_DEFAULT_TRACK,
                              amp,
                              len);
    if (bell_mf_tx_put(&gen, "ABC"))
        printf("Ooops\n");
    len = bell_mf_tx(&gen, amp, 160);
    printf("Generated %d samples\n", len);
    outframes = afWriteFrames(outhandle,
                              AF_DEFAULT_TRACK,
                              amp,
                              len);
    add_digits = 1;
    do
    {
        len = bell_mf_tx(&gen, amp, 160);
        printf("Generated %d samples\n", len);
        if (len > 0)
        {
            outframes = afWriteFrames(outhandle,
                                      AF_DEFAULT_TRACK,
                                      amp,
                                      len);
        }
        if (add_digits)
        {
            if (bell_mf_tx_put(&gen, "1234567890"))
            {
                printf("Digit buffer full\n");
                add_digits = 0;
            }
        }
    }
    while (len > 0);

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
