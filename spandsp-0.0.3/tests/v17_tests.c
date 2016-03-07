#define ENABLE_V17
/*
 * SpanDSP - a series of DSP components for telephony
 *
 * v17_tests.c
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
 * $Id: v17_tests.c,v 1.60 2006/11/19 14:07:27 steveu Exp $
 */

/*! \page v17_tests_page V.17 modem tests
\section v17_tests_page_sec_1 What does it do?
These tests test one way paths, as V.17 is a half-duplex modem. They allow either:

 - A V.17 transmit modem to feed a V.17 receive modem through a telephone line
   model. BER testing is then used to evaluate performance under various line
   conditions. This is effective for testing the basic performance of the
   receive modem. It is also the only test mode provided for evaluating the
   transmit modem.

 - A V.17 receive modem is used to decode V.17 audio, stored in a wave file.
   This is good way to evaluate performance with audio recorded from other
   models of modem, and with real world problematic telephone lines.

If the appropriate GUI environment exists, the tests are built such that a visual
display of modem status is maintained.

\section v17_tests_page_sec_2 How is it used?
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#if defined(HAVE_FL_FL_H)  &&  defined(HAVE_FL_FL_CARTESIAN_H)  &&  defined(HAVE_FL_FL_AUDIO_METER_H)
#define ENABLE_GUI
#endif

#include <inttypes.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
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
#include "test_utils.h"
#include "line_model.h"
#if defined(ENABLE_GUI)
#include "modem_monitor.h"
#include "line_model_monitor.h"
#endif

#define BLOCK_LEN       160

#define OUT_FILE_NAME   "v17.wav"

char *decode_test_file = NULL;
int use_gui = FALSE;

int symbol_no = 0;

int rx_bits = 0;
int tx_bits = 0;

int test_bps;

bert_state_t bert;
one_way_line_model_state_t *line_model;

#if defined(ENABLE_GUI)
qam_monitor_t *qam_monitor;
#endif

bert_results_t latest_results;

static void reporter(void *user_data, int reason, bert_results_t *results)
{
    switch (reason)
    {
    case BERT_REPORT_SYNCED:
        printf("BERT report synced\n");
        break;
    case BERT_REPORT_UNSYNCED:
        printf("BERT report unsync'ed\n");
        break;
    case BERT_REPORT_REGULAR:
        printf("BERT report regular - %d bits, %d bad bits, %d resyncs\n", results->total_bits, results->bad_bits, results->resyncs);
        memcpy(&latest_results, results, sizeof(latest_results));
        break;
    case BERT_REPORT_GT_10_2:
        printf("BERT report > 1 in 10^2\n");
        break;
    case BERT_REPORT_LT_10_2:
        printf("BERT report < 1 in 10^2\n");
        break;
    case BERT_REPORT_LT_10_3:
        printf("BERT report < 1 in 10^3\n");
        break;
    case BERT_REPORT_LT_10_4:
        printf("BERT report < 1 in 10^4\n");
        break;
    case BERT_REPORT_LT_10_5:
        printf("BERT report < 1 in 10^5\n");
        break;
    case BERT_REPORT_LT_10_6:
        printf("BERT report < 1 in 10^6\n");
        break;
    case BERT_REPORT_LT_10_7:
        printf("BERT report < 1 in 10^7\n");
        break;
    default:
        printf("BERT report reason %d\n", reason);
        break;
    }
}
/*- End of function --------------------------------------------------------*/

static void v17putbit(void *user_data, int bit)
{
    v17_rx_state_t *rx;
    int i;
    int len;
    complexf_t *coeffs;
    
    rx = (v17_rx_state_t *) user_data;
    if (bit < 0)
    {
        /* Special conditions */
        switch (bit)
        {
        case PUTBIT_TRAINING_FAILED:
            printf("Training failed\n");
            break;
        case PUTBIT_TRAINING_SUCCEEDED:
            printf("Training succeeded\n");
            len = v17_rx_equalizer_state(rx, &coeffs);
            printf("Equalizer:\n");
            for (i = 0;  i < len;  i++)
                printf("%3d (%15.5f, %15.5f) -> %15.5f\n", i, coeffs[i].re, coeffs[i].im, powerf(&coeffs[i]));
            break;
        case PUTBIT_CARRIER_UP:
            printf("Carrier up\n");
            break;
        case PUTBIT_CARRIER_DOWN:
            printf("Carrier down\n");
            break;
        default:
            printf("Eh!\n");
            break;
        }
        return;
    }

#if 0
    printf("Rx bit %d - %d - %d\n", rx_bits++, bit, tx_bits);
#else
    if (decode_test_file)
        printf("Rx bit %d - %d\n", rx_bits++, bit);
    else
        bert_put_bit(&bert, bit);
#endif
}
/*- End of function --------------------------------------------------------*/

static int v17getbit(void *user_data)
{
#if 0
    if (++tx_bits >= 50000)
    {
        tx_bits = 0;
        return PUTBIT_END_OF_DATA;
    }
//    if (tx_bits < 200)
//        return (tx_bits & 1);
    if (tx_bits == 1)
        return 0;
    return 1;
#else
    return bert_get_bit(&bert);
#endif
}
/*- End of function --------------------------------------------------------*/

static void qam_report(void *user_data, const complexf_t *constel, const complexf_t *target, int symbol)
{
    int i;
    int len;
    complexf_t *coeffs;
    float fpower;
    v17_rx_state_t *rx;
    static float smooth_power = 0.0;
    static int update_interval = 100;

    rx = (v17_rx_state_t *) user_data;
    if (constel)
    {
#if defined(ENABLE_GUI)
        if (use_gui)
        {
            qam_monitor_update_constel(qam_monitor, constel);
            qam_monitor_update_carrier_tracking(qam_monitor, v17_rx_carrier_frequency(rx));
            qam_monitor_update_symbol_tracking(qam_monitor, v17_rx_symbol_timing_correction(rx));
        }
#endif
        fpower = (constel->re - target->re)*(constel->re - target->re)
               + (constel->im - target->im)*(constel->im - target->im);
        smooth_power = 0.95*smooth_power + 0.05*fpower;
        printf("%8d [%8.4f, %8.4f] [%8.4f, %8.4f] %2x %8.4f %8.4f %9.4f %7.3f\n",
               symbol_no,
               constel->re,
               constel->im,
               target->re,
               target->im,
               symbol,
               fpower,
               smooth_power,
               v17_rx_carrier_frequency(rx),
               v17_rx_signal_power(rx));
        printf("Carrier %d %f %f\n", symbol_no, v17_rx_carrier_frequency(rx), v17_rx_symbol_timing_correction(rx));
        symbol_no++;
        if (--update_interval <= 0)
        {
            len = v17_rx_equalizer_state(rx, &coeffs);
            printf("Equalizer A:\n");
            for (i = 0;  i < len;  i++)
                printf("%3d (%15.5f, %15.5f) -> %15.5f\n", i, coeffs[i].re, coeffs[i].im, powerf(&coeffs[i]));
#if defined(ENABLE_GUI)
            if (use_gui)
                qam_monitor_update_equalizer(qam_monitor, coeffs, len);
#endif
            update_interval = 100;
        }
    }
}
/*- End of function --------------------------------------------------------*/

int main(int argc, char *argv[])
{
    v17_rx_state_t rx;
    v17_tx_state_t tx;
    bert_results_t bert_results;
    int16_t gen_amp[BLOCK_LEN];
    int16_t amp[BLOCK_LEN];
    AFfilehandle inhandle;
    AFfilehandle outhandle;
    AFfilesetup filesetup;
    int outframes;
    int samples;
    int i;
    int tep;
    int block_no;
    int noise_level;
    int signal_level;
    int bits_per_test;
    int line_model_no;
    int log_audio;
    int channel_codec;

    channel_codec = MUNGE_CODEC_NONE;
    test_bps = 14400;
    tep = FALSE;
    line_model_no = 0;
    decode_test_file = NULL;
    noise_level = -70;
    signal_level = -13;
    bits_per_test = 50000;
    log_audio = FALSE;
    for (i = 1;  i < argc;  i++)
    {
        if (strcmp(argv[i], "-b") == 0)
        {
            bits_per_test = atoi(argv[++i]);
            continue;
        }
        if (strcmp(argv[i], "-c") == 0)
        {
            channel_codec = atoi(argv[++i]);
            continue;
        }
        if (strcmp(argv[i], "-d") == 0)
        {
            decode_test_file = argv[++i];
            continue;
        }
        if (strcmp(argv[i], "-t") == 0)
        {
            tep = TRUE;
            continue;
        }
        if (strcmp(argv[i], "-g") == 0)
        {
            use_gui = TRUE;
            continue;
        }
        if (strcmp(argv[i], "-l") == 0)
        {
            log_audio = TRUE;
            continue;
        }
        if (strcmp(argv[i], "-m") == 0)
        {
            line_model_no = atoi(argv[++i]);
            continue;
        }
        if (strcmp(argv[i], "-n") == 0)
        {
            noise_level = atoi(argv[++i]);
            continue;
        }
        if (strcmp(argv[i], "-s") == 0)
        {
            signal_level = atoi(argv[++i]);
            continue;
        }
        if (strcmp(argv[i], "14400") == 0)
            test_bps = 14400;
        else if (strcmp(argv[i], "12000") == 0)
            test_bps = 12000;
        else if (strcmp(argv[i], "9600") == 0)
            test_bps = 9600;
        else if (strcmp(argv[i], "7200") == 0)
            test_bps = 7200;
        else
        {
            fprintf(stderr, "Invalid bit rate\n");
            exit(2);
        }
    }
    inhandle = NULL;
    outhandle = NULL;

    filesetup = AF_NULL_FILESETUP;
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
        afInitChannels(filesetup, AF_DEFAULT_TRACK, 1);
        if ((outhandle = afOpenFile(OUT_FILE_NAME, "w", filesetup)) == AF_NULL_FILEHANDLE)
        {
            fprintf(stderr, "    Cannot create wave file '%s'\n", OUT_FILE_NAME);
            exit(2);
        }
    }

    if (decode_test_file)
    {
        /* We will decode the audio from a wave file. */
        if ((inhandle = afOpenFile(decode_test_file, "r", NULL)) == AF_NULL_FILEHANDLE)
        {
            fprintf(stderr, "    Cannot open wave file '%s'\n", decode_test_file);
            exit(2);
        }
    }
    else
    {
        /* We will generate V.17 audio, and add some noise to it. */
        v17_tx_init(&tx, test_bps, tep, v17getbit, NULL);
        v17_tx_power(&tx, signal_level);
        /* Move the carrier off a bit */
        tx.carrier_phase_rate = dds_phase_ratef(1792.0);
        tx.carrier_phase = 0x40000000;

        bert_init(&bert, bits_per_test, BERT_PATTERN_ITU_O152_11, test_bps, 20);
        bert_set_report(&bert, 10000, reporter, NULL);

        if ((line_model = one_way_line_model_init(line_model_no, (float) noise_level, channel_codec)) == NULL)
        {
            fprintf(stderr, "    Failed to create line model\n");
            exit(2);
        }
    }

    v17_rx_init(&rx, test_bps, v17putbit, &rx);
    v17_rx_set_qam_report_handler(&rx, qam_report, (void *) &rx);
    span_log_set_level(&rx.logging, SPAN_LOG_SHOW_SEVERITY | SPAN_LOG_SHOW_PROTOCOL | SPAN_LOG_FLOW);
    span_log_set_tag(&rx.logging, "V.17 rx");

#if defined(ENABLE_GUI)
    if (use_gui)
    {
        qam_monitor = qam_monitor_init(10.0, NULL);
        if (!decode_test_file)
        {
            start_line_model_monitor(129);
            line_model_monitor_line_model_update(line_model->near_filter, line_model->near_filter_len);
        }
    }
#endif

    memset(&latest_results, 0, sizeof(latest_results));
    for (block_no = 0;  block_no < 100000000;  block_no++)
    {
        if (decode_test_file)
        {
            samples = afReadFrames(inhandle,
                                   AF_DEFAULT_TRACK,
                                   amp,
                                   BLOCK_LEN);
            if (samples == 0)
                break;
#if defined(ENABLE_GUI)
            if (use_gui)
                qam_monitor_update_audio_level(qam_monitor, amp, samples);
#endif
        }
        else
        {
            samples = v17_tx(&tx, gen_amp, BLOCK_LEN);
#if defined(ENABLE_GUI)
            if (use_gui)
                qam_monitor_update_audio_level(qam_monitor, gen_amp, samples);
#endif
            if (samples == 0)
            {
                printf("Restarting on zero output\n");

                /* Push a little silence through, to ensure all the data bits get out of the buffers */
                memset(amp, 0, BLOCK_LEN*sizeof(int16_t));
                v17_rx(&rx, amp, BLOCK_LEN);

                /* Note that we might get a few bad bits as the carrier shuts down. */
                bert_result(&bert, &bert_results);
                fprintf(stderr, "Final result %ddBm0, %d bits, %d bad bits, %d resyncs\n", signal_level, bert_results.total_bits, bert_results.bad_bits, bert_results.resyncs);
                fprintf(stderr, "Last report  %ddBm0, %d bits, %d bad bits, %d resyncs\n", signal_level, latest_results.total_bits, latest_results.bad_bits, latest_results.resyncs);
                /* See if bit errors are appearing yet. Also check we are getting enough bits out of the receiver. The last regular report
                   should be error free, though the final report will generally contain bits errors as the carrier was dying. The total
                   number of bits out of the receiver should be at least the number we sent. Also, since BERT sync should have occurred
                   rapidly at the start of transmission, the last report should have occurred at not much less than the total number of
                   bits we sent. */
#if 0
                if (bert_results.total_bits < bits_per_test
                    ||
                    latest_results.total_bits < bits_per_test - 100
                    ||
                    latest_results.bad_bits != 0)
                {
                    break;
                }
#endif
                memset(&latest_results, 0, sizeof(latest_results));
                v17_tx_restart(&tx, test_bps, tep, TRUE);
                v17_tx_power(&tx, signal_level);
                v17_rx_restart(&rx, test_bps, TRUE);
                rx.eq_put_step = rand()%(192*10/3);
                bert_init(&bert, bits_per_test, BERT_PATTERN_ITU_O152_11, test_bps, 20);
                bert_set_report(&bert, 10000, reporter, NULL);
            }
            if (log_audio)
            {
                outframes = afWriteFrames(outhandle,
                                          AF_DEFAULT_TRACK,
                                          gen_amp,
                                          samples);
                if (outframes != samples)
                {
                    fprintf(stderr, "    Error writing wave file\n");
                    exit(2);
                }
            }
            one_way_line_model(line_model, amp, gen_amp, samples);
        }
#if defined(ENABLE_GUI)
        if (use_gui  &&  !decode_test_file)
            line_model_monitor_line_spectrum_update(amp, samples);
#endif
        v17_rx(&rx, amp, samples);
    }
    if (decode_test_file)
    {
#if defined(ENABLE_GUI)
        if (use_gui)
            qam_wait_to_end(qam_monitor);
#endif
    }
    else
    {
        bert_result(&bert, &bert_results);
        fprintf(stderr, "At completion:\n");
        fprintf(stderr, "Final result %ddBm0, %d bits, %d bad bits, %d resyncs\n", signal_level, bert_results.total_bits, bert_results.bad_bits, bert_results.resyncs);
        fprintf(stderr, "Last report  %ddBm0, %d bits, %d bad bits, %d resyncs\n", signal_level, latest_results.total_bits, latest_results.bad_bits, latest_results.resyncs);
        one_way_line_model_release(line_model);
    }
    if (log_audio)
    {
        if (afCloseFile(outhandle))
        {
            fprintf(stderr, "    Cannot close wave file '%s'\n", OUT_FILE_NAME);
            exit(2);
        }
        afFreeFileSetup(filesetup);
    }
    return  0;
}
/*- End of function --------------------------------------------------------*/
/*- End of file ------------------------------------------------------------*/
