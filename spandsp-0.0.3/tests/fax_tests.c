/*
 * SpanDSP - a series of DSP components for telephony
 *
 * fax_tests.c
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
 * $Id: fax_tests.c,v 1.60 2006/11/19 14:07:27 steveu Exp $
 */

/*! \page fax_tests_page FAX tests
\section fax_tests_page_sec_1 What does it do?
\section fax_tests_page_sec_2 How does it work?
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
#include <assert.h>
#include <audiofile.h>
#include <tiffio.h>

#include "spandsp.h"

#define SAMPLES_PER_CHUNK       160

#define INPUT_TIFF_FILE_NAME    "../itutests/fax/itutests.tif"

#define OUTPUT_FILE_NAME_WAVE   "fax_tests.wav"

#define FAX_MACHINES            2

struct machine_s
{
    int chan;
    int16_t amp[SAMPLES_PER_CHUNK];
    int len;
    fax_state_t fax;
    int done;
    int succeeded;
    char tag[50];
    int error_delay;
} machines[FAX_MACHINES];

int test_local_interrupt = FALSE;

static void phase_b_handler(t30_state_t *s, void *user_data, int result)
{
    int i;
    
    i = (intptr_t) user_data;
    printf("%d: Phase B handler on channel %d - (0x%X) %s\n", i, i, result, t30_frametype(result));
}
/*- End of function --------------------------------------------------------*/

static void phase_d_handler(t30_state_t *s, void *user_data, int result)
{
    int i;
    t30_stats_t t;
    char ident[21];

    i = (intptr_t) user_data;
    printf("%d: Phase D handler on channel %d - (0x%X) %s\n", i, i, result, t30_frametype(result));
    t30_get_transfer_statistics(s, &t);
    printf("%d: Phase D: bit rate %d\n", i, t.bit_rate);
    printf("%d: Phase D: ECM %s\n", i, (t.error_correcting_mode)  ?  "on"  :  "off");
    printf("%d: Phase D: pages transferred %d\n", i, t.pages_transferred);
    printf("%d: Phase D: image size %d x %d\n", i, t.width, t.length);
    printf("%d: Phase D: image resolution %d x %d\n", i, t.x_resolution, t.y_resolution);
    printf("%d: Phase D: bad rows %d\n", i, t.bad_rows);
    printf("%d: Phase D: longest bad row run %d\n", i, t.longest_bad_row_run);
    printf("%d: Phase D: compression type %d\n", i, t.encoding);
    printf("%d: Phase D: image size %d\n", i, t.image_size);
    t30_get_local_ident(s, ident);
    printf("%d: Phase D: local ident '%s'\n", i, ident);
    t30_get_far_ident(s, ident);
    printf("%d: Phase D: remote ident '%s'\n", i, ident);
    
    if (test_local_interrupt)
    {
        if (i == 0)
        {
            printf("%d: Initiating interrupt request\n", i);
            t30_local_interrupt_request(s, TRUE);
        }
        else
        {
            switch (result)
            {
            case T30_PIP:
            case T30_PRI_MPS:
            case T30_PRI_EOM:
            case T30_PRI_EOP:
                printf("%d: Accepting interrupt request\n", i);
                t30_local_interrupt_request(s, TRUE);
                break;
            case T30_PIN:
                break;
            }
        }
    }
}
/*- End of function --------------------------------------------------------*/

static void phase_e_handler(t30_state_t *s, void *user_data, int result)
{
    int i;
    t30_stats_t t;
    const char *u;
    char ident[21];
    
    i = (intptr_t) user_data;
    printf("%d: Phase E handler on channel %d - (%d) %s\n", i, i, result, t30_completion_code_to_str(result));    
    t30_get_transfer_statistics(s, &t);
    printf("%d: Phase E: bit rate %d\n", i, t.bit_rate);
    printf("%d: Phase E: ECM %s\n", i, (t.error_correcting_mode)  ?  "on"  :  "off");
    printf("%d: Phase E: pages transferred %d\n", i, t.pages_transferred);
    printf("%d: Phase E: image size %d x %d\n", i, t.width, t.length);
    printf("%d: Phase E: image resolution %d x %d\n", i, t.x_resolution, t.y_resolution);
    printf("%d: Phase E: bad rows %d\n", i, t.bad_rows);
    printf("%d: Phase E: longest bad row run %d\n", i, t.longest_bad_row_run);
    printf("%d: Phase E: coding method %s\n", i, t4_encoding_to_str(t.encoding));
    printf("%d: Phase E: image size %d bytes\n", i, t.image_size);
    t30_get_local_ident(s, ident);
    printf("%d: Phase E: local ident '%s'\n", i, ident);
    t30_get_far_ident(s, ident);
    printf("%d: Phase E: remote ident '%s'\n", i, ident);
    if ((u = t30_get_far_country(s)))
        printf("%d: Phase E: Remote was made in '%s'\n", i, u);
    if ((u = t30_get_far_vendor(s)))
        printf("%d: Phase E: Remote was made by '%s'\n", i, u);
    if ((u = t30_get_far_model(s)))
        printf("%d: Phase E: Remote is model '%s'\n", i, u);
    machines[i].succeeded = (result == T30_ERR_OK)  &&  (t.pages_transferred == 12);
    machines[i].done = TRUE;
}
/*- End of function --------------------------------------------------------*/

static int document_handler(t30_state_t *s, void *user_data, int event)
{
    int i;
    
    i = (intptr_t) user_data;
    printf("%d: Document handler on channel %d - event %d\n", i, i, event);
    return FALSE;
}
/*- End of function --------------------------------------------------------*/

int main(int argc, char *argv[])
{
    AFfilesetup filesetup;
    AFfilehandle wave_handle;
    AFfilehandle input_wave_handle;
    int i;
    int j;
    int k;
    struct machine_s *mc;
    int outframes;
    char buf[128 + 1];
    int16_t silence[SAMPLES_PER_CHUNK];
    int16_t out_amp[2*SAMPLES_PER_CHUNK];
    int alldone;
    const char *input_tiff_file_name;
    const char *input_audio_file_name;
    int log_audio;
    int use_ecm;
    int use_tep;
    int use_line_hits;
    int polled_mode;
    char *page_header_info;

    log_audio = FALSE;
    input_tiff_file_name = INPUT_TIFF_FILE_NAME;
    input_audio_file_name = NULL;
    use_ecm = FALSE;
    use_line_hits = FALSE;
    use_tep = FALSE;
    polled_mode = FALSE;
    page_header_info = NULL;
    for (i = 1;  i < argc;  i++)
    {
        if (strcmp(argv[i], "-e") == 0)
        {
            use_ecm = TRUE;
            continue;
        }
        if (strcmp(argv[i], "-h") == 0)
        {
            use_line_hits = TRUE;
            continue;
        }
        if (strcmp(argv[i], "-H") == 0)
        {
            page_header_info = argv[++i];
            continue;
        }
        if (strcmp(argv[i], "-i") == 0)
        {
            input_tiff_file_name = argv[++i];
            continue;
        }
        if (strcmp(argv[i], "-I") == 0)
        {
            input_audio_file_name = argv[++i];
            continue;
        }
        if (strcmp(argv[i], "-l") == 0)
        {
            log_audio = TRUE;
            continue;
        }
        if (strcmp(argv[i], "-p") == 0)
        {
            polled_mode = TRUE;
            continue;
        }
        if (strcmp(argv[i], "-t") == 0)
        {
            use_tep = TRUE;
            continue;
        }
    }

    input_wave_handle = AF_NULL_FILEHANDLE;
    if (input_audio_file_name)
    {
        if ((input_wave_handle = afOpenFile(input_audio_file_name, "r", NULL)) == AF_NULL_FILEHANDLE)
        {
            fprintf(stderr, "    Cannot open wave file '%s'\n", input_audio_file_name);
            exit(2);
        }
    }

    filesetup = AF_NULL_FILESETUP;
    wave_handle = AF_NULL_FILEHANDLE;
    if (log_audio)
    {
        if ((filesetup = afNewFileSetup()) == AF_NULL_FILESETUP)
        {
            fprintf(stderr, "    Failed to create file setup\n");
            exit(2);
        }
        afInitSampleFormat(filesetup, AF_DEFAULT_TRACK, AF_SAMPFMT_TWOSCOMP, 16);
        afInitRate(filesetup, AF_DEFAULT_TRACK, (float) SAMPLE_RATE);
        afInitFileFormat(filesetup, AF_FILE_WAVE);
        afInitChannels(filesetup, AF_DEFAULT_TRACK, 2);

        if ((wave_handle = afOpenFile(OUTPUT_FILE_NAME_WAVE, "w", filesetup)) == AF_NULL_FILEHANDLE)
        {
            fprintf(stderr, "    Cannot create wave file '%s'\n", OUTPUT_FILE_NAME_WAVE);
            exit(2);
        }
    }

    memset(silence, 0, sizeof(silence));
    for (j = 0;  j < FAX_MACHINES;  j++)
    {
        machines[j].chan = j;
        mc = &machines[j];

        i = mc->chan + 1;
        sprintf(buf, "%d%d%d%d%d%d%d%d", i, i, i, i, i, i, i, i);
        fax_init(&mc->fax, (mc->chan & 1)  ?  FALSE  :  TRUE);
        fax_set_tep_mode(&mc->fax, use_tep);
        t30_set_local_ident(&mc->fax.t30_state, buf);
        t30_set_header_info(&mc->fax.t30_state, page_header_info);
        t30_set_local_nsf(&mc->fax.t30_state, (const uint8_t *) "\x50\x00\x00\x00Spandsp\x00", 12);
        t30_set_ecm_capability(&mc->fax.t30_state, use_ecm);
        t30_set_supported_image_sizes(&mc->fax.t30_state, T30_SUPPORT_US_LETTER_LENGTH | T30_SUPPORT_US_LEGAL_LENGTH | T30_SUPPORT_UNLIMITED_LENGTH
                                                        | T30_SUPPORT_215MM_WIDTH | T30_SUPPORT_255MM_WIDTH | T30_SUPPORT_303MM_WIDTH);
        t30_set_supported_resolutions(&mc->fax.t30_state, T30_SUPPORT_STANDARD_RESOLUTION | T30_SUPPORT_FINE_RESOLUTION | T30_SUPPORT_SUPERFINE_RESOLUTION
                                                        | T30_SUPPORT_R8_RESOLUTION | T30_SUPPORT_R16_RESOLUTION);
        if (use_ecm)
            t30_set_supported_compressions(&mc->fax.t30_state, T30_SUPPORT_T4_1D_COMPRESSION | T30_SUPPORT_T4_2D_COMPRESSION | T30_SUPPORT_T6_COMPRESSION);
        if ((mc->chan & 1))
        {
            if (polled_mode)
            {
                t30_set_tx_file(&mc->fax.t30_state, input_tiff_file_name, -1, -1);
            }
            else
            {
                sprintf(buf, "fax_tests_%d.tif", (mc->chan + 1)/2);
                t30_set_rx_file(&mc->fax.t30_state, buf, -1);
            }
        }
        else
        {
            if (polled_mode)
            {
                sprintf(buf, "fax_tests_%d.tif", (mc->chan + 1)/2);
                t30_set_rx_file(&mc->fax.t30_state, buf, -1);
            }
            else
            {
                t30_set_tx_file(&mc->fax.t30_state, input_tiff_file_name, -1, -1);
            }
        }
        t30_set_phase_b_handler(&mc->fax.t30_state, phase_b_handler, (void *) (intptr_t) mc->chan);
        t30_set_phase_d_handler(&mc->fax.t30_state, phase_d_handler, (void *) (intptr_t) mc->chan);
        t30_set_phase_e_handler(&mc->fax.t30_state, phase_e_handler, (void *) (intptr_t) mc->chan);
        t30_set_document_handler(&mc->fax.t30_state, document_handler, (void *) (intptr_t) mc->chan);
        sprintf(mc->tag, "FAX-%d", j + 1);
        span_log_set_level(&mc->fax.t30_state.logging, SPAN_LOG_SHOW_SEVERITY | SPAN_LOG_SHOW_PROTOCOL | SPAN_LOG_SHOW_TAG | SPAN_LOG_SHOW_SAMPLE_TIME | SPAN_LOG_FLOW);
        span_log_set_tag(&mc->fax.t30_state.logging, mc->tag);
        span_log_set_level(&mc->fax.v29rx.logging, SPAN_LOG_SHOW_SEVERITY | SPAN_LOG_SHOW_PROTOCOL | SPAN_LOG_SHOW_TAG | SPAN_LOG_SHOW_SAMPLE_TIME | SPAN_LOG_FLOW);
        span_log_set_tag(&mc->fax.v29rx.logging, mc->tag);
        span_log_set_level(&mc->fax.logging, SPAN_LOG_SHOW_SEVERITY | SPAN_LOG_SHOW_PROTOCOL | SPAN_LOG_SHOW_TAG | SPAN_LOG_SHOW_SAMPLE_TIME | SPAN_LOG_FLOW);
        span_log_set_tag(&mc->fax.logging, mc->tag);
        memset(mc->amp, 0, sizeof(mc->amp));
        mc->done = FALSE;
    }
    for (;;)
    {
        alldone = TRUE;
        for (j = 0;  j < FAX_MACHINES;  j++)
        {
            mc = &machines[j];

            if ((j & 1) == 0  &&  input_audio_file_name)
            {
                mc->len = afReadFrames(input_wave_handle, AF_DEFAULT_TRACK, mc->amp, SAMPLES_PER_CHUNK);
                if (mc->len == 0)
                    break;
            }
            else
            {
                mc->len = fax_tx(&mc->fax, mc->amp, SAMPLES_PER_CHUNK);
            }
            /* The receive side always expects a full block of samples, but the
               transmit side may not be sending any when it doesn't need to. We
               may need to pad with some silence. */
            if (mc->len < SAMPLES_PER_CHUNK)
            {
                memset(mc->amp + mc->len, 0, sizeof(int16_t)*(SAMPLES_PER_CHUNK - mc->len));
                mc->len = SAMPLES_PER_CHUNK;
            }

            span_log_bump_samples(&mc->fax.t30_state.logging, mc->len);
            span_log_bump_samples(&mc->fax.v29rx.logging, mc->len);
            span_log_bump_samples(&mc->fax.logging, mc->len);

            if (log_audio)
            {
                for (k = 0;  k < mc->len;  k++)
                    out_amp[2*k + j] = mc->amp[k];
            }
            if (machines[j ^ 1].len < SAMPLES_PER_CHUNK)
                memset(machines[j ^ 1].amp + machines[j ^ 1].len, 0, sizeof(int16_t)*(SAMPLES_PER_CHUNK - machines[j ^ 1].len));
            if (use_line_hits)
            {
                /* TODO: This applied very crude line hits. improve it */
                if (mc->fax.t30_state.state == 22)
                {
                    if (++mc->error_delay == 100)
                    {
                        fprintf(stderr, "HIT %d!\n", j);
                        mc->error_delay = 0;
                        for (k = 0;  k < 5;  k++)
                            mc->amp[k] = 0;
                    }
                }    
            }
            if (fax_rx(&mc->fax, machines[j ^ 1].amp, SAMPLES_PER_CHUNK))
                break;
            if (!mc->done)
                alldone = FALSE;
        }

        if (log_audio)
        {
            outframes = afWriteFrames(wave_handle, AF_DEFAULT_TRACK, out_amp, SAMPLES_PER_CHUNK);
            if (outframes != SAMPLES_PER_CHUNK)
                break;
        }

        if (alldone  ||  j < FAX_MACHINES)
            break;
    }
    for (j = 0;  j < FAX_MACHINES;  j++)
    {
        mc = &machines[j];
        fax_release(&mc->fax);
    }
    if (log_audio)
    {
        if (afCloseFile(wave_handle))
        {
            fprintf(stderr, "    Cannot close wave file '%s'\n", OUTPUT_FILE_NAME_WAVE);
            exit(2);
        }
        afFreeFileSetup(filesetup);
    }
    if (input_audio_file_name)
    {
        if (afCloseFile(input_wave_handle))
        {
            fprintf(stderr, "    Cannot close wave file '%s'\n", input_audio_file_name);
            exit(2);
        }
        afFreeFileSetup(filesetup);
    }
    return  0;
}
/*- End of function --------------------------------------------------------*/
/*- End of file ------------------------------------------------------------*/
