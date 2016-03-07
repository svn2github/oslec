/*
 * SpanDSP - a series of DSP components for telephony
 *
 * echo_tests.c
 *
 * Written by Steve Underwood <steveu@coppice.org>
 *
 * Copyright (C) 2001 Steve Underwood
 *
 * Based on a bit from here, a bit from there, eye of toad,
 * ear of bat, etc - plus, of course, my own 2 cents.
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
 * $Id: echo_tests.c,v 1.27 2006/11/19 14:07:27 steveu Exp $
 */

/*! \page echo_can_tests_page Line echo cancellation for voice tests

\section echo_can_tests_page_sec_1 What does it do?
The echo cancellation tests test the echo cancellor against the G.168 spec. Not
all the tests in G.168 are fully implemented at this time.

\section echo_can_tests_page_sec_2 How does it work?

\section echo_can_tests_page_sec_2 How do I use it?

*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#if defined(HAVE_FL_FL_H)  &&  defined(HAVE_FL_FL_CARTESIAN_H)  &&  defined(HAVE_FL_FL_AUDIO_METER_H)
#define ENABLE_GUI
#endif

#define _GNU_SOURCE

#include <unistd.h>
#include <inttypes.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#if defined(HAVE_TGMATH_H)
#include <tgmath.h>
#endif
#if defined(HAVE_MATH_H)
#include <math.h>
#endif
#include <assert.h>
#include <audiofile.h>
#include <tiffio.h>

#define GEN_CONST
#include <math.h>

#include "spandsp.h"
#include "spandsp/g168models.h"
#if defined(ENABLE_GUI)
#include "echo_monitor.h"
#endif

#define TEST_EC_TAPS            256

#if !defined(NULL)
#define NULL (void *) 0
#endif

typedef struct
{
    const char *name;
    int max;
    int cur;
    AFfilehandle handle;
    int16_t signal[SAMPLE_RATE];
} signal_source_t;

typedef struct
{
    int type;
    fir_float_state_t *fir;
    float history[35*8];
    int pos;
    float factor; 
    float power;
} level_measurement_device_t;

signal_source_t local_css;
signal_source_t far_css;

fir32_state_t line_model;
float model_ki, erl;

AFfilehandle residuehandle;
int16_t residue_sound[SAMPLE_RATE];
int residue_cur = 0;
int munge;

FILE *fdump;

float clip(float x);
float clip(float x) {
    if (x > 32767.0) x = 32767.0;
    if (x < -32767.0) x = -32767.0;

    return x;
}
/*- End of function --------------------------------------------------------*/

static inline void put_residue(int16_t amp)
{
    int outframes;

    residue_sound[residue_cur++] = amp;
    if (residue_cur >= SAMPLE_RATE)    {
        outframes = afWriteFrames(residuehandle,
                                  AF_DEFAULT_TRACK,
                                  residue_sound,
                                  residue_cur);
        if (outframes != residue_cur)
        {
            fprintf(stderr, "    Error writing residue sound\n");
            exit(2);
        }
        residue_cur = 0;
    }
}
/*- End of function --------------------------------------------------------*/

static void signal_load(signal_source_t *sig, const char *name)
{
    float x;

    sig->name = name;
    if ((sig->handle = afOpenFile(sig->name, "r", 0)) == AF_NULL_FILEHANDLE)
    {
        fprintf(stderr, "    Cannot open wave file '%s'\n", sig->name);
        exit(2);
    }
    if ((x = afGetFrameSize(sig->handle, AF_DEFAULT_TRACK, 1)) != 2.0)
    {
        fprintf(stderr, "    Unexpected frame size in wave file '%s'\n", sig->name);
        exit(2);
    }
    if ((x = afGetRate(sig->handle, AF_DEFAULT_TRACK)) != (float) SAMPLE_RATE)
    {
        printf("    Unexpected sample rate in wave file '%s'\n", sig->name);
        exit(2);
    }
    if ((x = afGetChannels(sig->handle, AF_DEFAULT_TRACK)) != 1.0)
    {
        printf("    Unexpected number of channels in wave file '%s'\n", sig->name);
        exit(2);
    }
    sig->max = afReadFrames(sig->handle, AF_DEFAULT_TRACK, sig->signal, SAMPLE_RATE);
    if (sig->max < 0)
    {
        fprintf(stderr, "    Error reading sound file '%s'\n", sig->name);
        exit(2);
    }
}
/*- End of function --------------------------------------------------------*/

static AFfilehandle af_file_open_for_read(const char *name)
{
    float x;
    AFfilehandle handle;

    if ((handle = afOpenFile(name, "r", 0)) == AF_NULL_FILEHANDLE)
    {
        fprintf(stderr, "    Cannot open wave file '%s'\n", name);
        exit(2);
    }
    if ((x = afGetFrameSize(handle, AF_DEFAULT_TRACK, 1)) != 2.0)
    {
        fprintf(stderr, "    Unexpected frame size in wave file '%s'\n", name);
        exit(2);
    }
    if ((x = afGetRate(handle, AF_DEFAULT_TRACK)) != (float) SAMPLE_RATE)
    {
        printf("    Unexpected sample rate in wave file '%s'\n", name);
        exit(2);
    }
    if ((x = afGetChannels(handle, AF_DEFAULT_TRACK)) != 1.0)
    {
        printf("    Unexpected number of channels in wave file '%s'\n", name);
        exit(2);
    }

    return handle;
}
/*- End of function --------------------------------------------------------*/

static AFfilehandle af_file_open_for_write(const char *name)
{
    AFfilesetup  setup;
    AFfilehandle handle;

    setup = afNewFileSetup();
    if (setup == AF_NULL_FILESETUP)
    {
        fprintf(stderr, "    %s: Failed to create file setup\n", name);
        exit(2);
    }
    afInitSampleFormat(setup, AF_DEFAULT_TRACK, AF_SAMPFMT_TWOSCOMP, 16);
    afInitRate(setup, AF_DEFAULT_TRACK, (float) SAMPLE_RATE);
    afInitFileFormat(setup, AF_FILE_WAVE);
    afInitChannels(setup, AF_DEFAULT_TRACK, 1);
    handle = afOpenFile(name, "w", setup);

    if (handle == AF_NULL_FILEHANDLE)
    {
        fprintf(stderr, "    Failed to open result file\n");
        exit(2);
    }
    afFreeFileSetup(setup);

    return handle;
}
/*- End of function --------------------------------------------------------*/

static void signal_restart(signal_source_t *sig)
{
    sig->cur = 0;
}
/*- End of function --------------------------------------------------------*/

static int16_t signal_amp(signal_source_t *sig)
{
    int16_t tx;

    tx = sig->signal[sig->cur++];
    if (sig->cur >= sig->max)
        sig->cur = 0;
    return tx;
}
/*- End of function --------------------------------------------------------*/

/* note mu-law used, alaw has big DC Offsets that causes probs with G168
   tests, due to alaw idle values being passed thru when NLP opens for
   very low level signals.  Probably need a DC blocking filter in e/c
*/
static inline int16_t codec_munge(int16_t amp)
{
    return ulaw_to_linear(linear_to_ulaw(amp));
}
/*- End of function --------------------------------------------------------*/

static int channel_model_create(int model)
{
    static const int32_t *line_models[] =
    {
        line_model_d2_coeffs,
        line_model_d3_coeffs,
        line_model_d4_coeffs,
        line_model_d5_coeffs,
        line_model_d6_coeffs,
        line_model_d7_coeffs,
        line_model_d8_coeffs,
        line_model_d9_coeffs
    };

    static int line_model_sizes[] =
    {
        sizeof(line_model_d2_coeffs)/sizeof(int32_t),
        sizeof(line_model_d3_coeffs)/sizeof(int32_t),
        sizeof(line_model_d4_coeffs)/sizeof(int32_t),
        sizeof(line_model_d5_coeffs)/sizeof(int32_t),
        sizeof(line_model_d6_coeffs)/sizeof(int32_t),
        sizeof(line_model_d7_coeffs)/sizeof(int32_t),
        sizeof(line_model_d8_coeffs)/sizeof(int32_t),
        sizeof(line_model_d9_coeffs)/sizeof(int32_t)
    };

    static float ki[] = 
    {
	1.39E-5, 1.44E-5, 1.52E-5, 1.77E-5, 9.33E-6, 1.51E-5, 2.33E-5, 1.33E-5
    };

    if (model < 1  ||  model > (int) (sizeof(line_model_sizes)/sizeof(line_model_sizes[0])))
        return -1;
    fir32_create(&line_model, line_models[model-1], line_model_sizes[model-1]);

    model_ki = ki[model-1];

    return 0;
}
/*- End of function --------------------------------------------------------*/

static int16_t channel_model(int16_t *new_local, int16_t *new_far, int16_t local, int16_t far)
{
    int16_t echo;
    int16_t rx;

    /* Channel modelling is merely simulating the effects of A-law or u-law distortion
       and using one of the echo models from G.168. Simulating the codec is very important,
       as this is usually the limiting factor in how much echo reduction is achieved. */

    /* The local tx signal will usually have gone through an A-law munging before
       it reached the line's analogue area, where the echo occurs. */
    if (munge == TRUE)
	local = codec_munge(local);
    /* Now we need to model the echo. We only model a single analogue segment, as per
       the G.168 spec. However, there will generally be near end and far end analogue/echoey
       segments in the real world, unless an end is purely digital. */
    echo = fir32(&line_model, local*erl*(32768.0*model_ki));
    /* The far end signal will have been through an A-law munging, although
       this should not affect things. */
    if (munge == TRUE)
	rx = clip(echo + codec_munge(far));
    else
	rx = clip(echo + far);
	
    /* This mixed echo and far end signal will have been through an A-law munging
       when it came back into the digital network. */
    if (munge == TRUE)
	rx = codec_munge(rx);
    if (new_far)
        *new_far = rx;
    if (new_local)
        *new_local = local;
    return  rx;
}
/*- End of function --------------------------------------------------------*/

/* 
   250Hz HP filter, designed using this excellent site:
 
   http://www-users.cs.york.ac.uk/~fisher/mkfilter/

   Included as preliminary test to see if this sort of filter will help
   hum removal from low cost X100P type cards.  Unfortunately I couldn't
   get thios to work well in fixed point, so had to leave it out of 
   treh core echo canceller.
*/

#define NZEROS 4
#define NPOLES 4

#define FIXED
#ifdef FIXED
#define GAIN   1.293080949e+00
#define QCONST32(x,bits) ((int)((x)*(1<<(bits))))

static int hp_filter(int xv[], int yv[], int x)
{
    xv[0] = xv[1]; xv[1] = xv[2]; xv[2] = xv[3]; xv[3] = xv[4]; 
    xv[4] = x * (int)((1<<5)/GAIN);
    yv[0] = yv[1]; yv[1] = yv[2]; yv[2] = yv[3]; yv[3] = yv[4]; 
    yv[4] =   (xv[0] + xv[4]) - 4 * (xv[1] + xv[3]) + 6 * xv[2];
    yv[4] +=  (QCONST32(-0.5980652616f, 10) * yv[0]) >> 10;
    yv[4] +=  (QCONST32( 2.6988843913f, 10) * yv[1]) >> 10;
    yv[4] +=  (QCONST32(-4.5892912321f, 10) * yv[2]) >> 10;
    yv[4] +=  (QCONST32( 3.4873077415f, 10) * yv[3]) >> 10;

    return yv[4] >> 5;     
}

#else
#define GAIN   1.293080949e+00

static float hp_filter(float xv[], float yv[], float x)
{
    xv[0] = xv[1]; xv[1] = xv[2]; xv[2] = xv[3]; xv[3] = xv[4]; 
    xv[4] = x / GAIN;
    yv[0] = yv[1]; yv[1] = yv[2]; yv[2] = yv[3]; yv[3] = yv[4]; 
    yv[4] =   (xv[0] + xv[4]) - 4 * (xv[1] + xv[3]) + 6 * xv[2]
	+ ( -0.5980652616 * yv[0]) + (  2.6988843913 * yv[1])
	+ ( -4.5892912321 * yv[2]) + (  3.4873077415 * yv[3]);
    return yv[4];     
}
#endif

static level_measurement_device_t *level_measurement_device_create(int type)
{
    level_measurement_device_t *dev;
    int i;

    dev = (level_measurement_device_t *) malloc(sizeof(level_measurement_device_t));
    dev->fir = (fir_float_state_t *) malloc(sizeof(fir_float_state_t));
    fir_float_create(dev->fir,
                     level_measurement_bp_coeffs,
                     sizeof(level_measurement_bp_coeffs)/sizeof(float));
    for (i = 0;  i < 35*8;  i++)
        dev->history[i] = 0.0;
    dev->pos = 0;
    dev->factor = exp(-1.0/((float) SAMPLE_RATE*0.035));
    dev->power = 0;
    dev->type = type;
    return  dev;
}
/*- End of function --------------------------------------------------------*/

static void level_measurement_device_reset(level_measurement_device_t *dev)
{
    int i;

    for (i = 0;  i < 35*8;  i++)
        dev->history[i] = 0.0;
    dev->pos = 0;
    dev->power = 0;
}
/*- End of function --------------------------------------------------------*/

static int level_measurement_device_release(level_measurement_device_t *s)
{
    fir_float_free(s->fir);
    free(s->fir);
    free(s);
    return 0;
}
/*- End of function --------------------------------------------------------*/

static float level_measurement_device(level_measurement_device_t *dev, int16_t amp)
{
    float signal;

    signal = fir_float(dev->fir, amp);
    signal *= signal;
    if (dev->type == 0)
    {
        dev->power = dev->power*dev->factor + signal*(1.0 - dev->factor);
        signal = sqrt(dev->power);
    }
    else
    {
        dev->power -= dev->history[dev->pos];
        dev->power += signal;
        dev->history[dev->pos++] = signal;
        signal = sqrt(dev->power/(35.8*8.0));
    }
    if (signal > 0.0) 
	return DBM0_MAX_POWER + 20.0*log10(signal/32767.0);
    else
	return -1000.0;
}
/*- End of function --------------------------------------------------------*/

/* Globals used for performing tests */

echo_can_state_t *ctx;
awgn_state_t      sgen_noise_source;
awgn_state_t      rin_noise_source;

level_measurement_device_t *Rin_power_meter;
level_measurement_device_t *Sgen_power_meter;
level_measurement_device_t *Sin_power_meter;
level_measurement_device_t *Sout_power_meter;

float LRin, maxLRin;
float LSgen, maxLSgen;
float LSin, maxLSin;
float LSout, maxLSout;
float Lres, maxLres;
float test_clock;
float maxHoth;

float Rin_level, Sgen_level;
FILE *flevel;
int Rin_type, Sgen_type;
int failed;
int verbose, quiet;
float threshold;
int model_number;
char test_name[80];

tone_gen_state_t rin_tone_state;
tone_gen_state_t sgen_tone_state;

/*
   Test callback functions are called one for every processed sample
   during run_test().  They are user supplied, and return TRUE if the
   test is passing or FALSE if a combination of variables mean that
   the test has failed (for example Lres exceeding some threshold).

   Different test callback functions are required for each G168 test.
*/
int (*test_callback)(void);

/* macros to convert units for run_test */

#define MSEC  (SAMPLE_RATE/1000)
#define SEC   SAMPLE_RATE

/* Sgen signal generator types */

#define NONE 0
#define CSS  1
#define HOTH 2
#define TONE 3

/* Experimentally generated constants to normalise levels to dBm0 */

#define HOTH_SCALE 2.40
#define CSS_SCALE  5.60

static void reset_all(void) {
    echo_can_flush(ctx);
    maxLRin = maxLSgen = maxLSin = maxLSout = maxLres = -100.0;
    signal_restart(&local_css);
    signal_restart(&far_css);
    test_callback = NULL;
    Rin_type = CSS;
    Sgen_type = NONE;
    failed = FALSE;
    test_clock = 0.0;
}

static void reset_meter_peaks(void) {
    maxLRin = maxLSgen = maxLSin = maxLSout = maxLres = -100.0;
}

static void install_test_callback(int (*f)(void)) {
    test_callback = f;
}

/* note: maybe we should use absolute levels rather than gain?  Need to
   normalise levels from various signal types to do this */

static void set_Sgen(int source_type, float gain) {
    Sgen_type = source_type;
    Sgen_level = pow(10.0, gain/20.0);
}

static void set_Rin(int source_type, float gain) {
    Rin_type = source_type;
    Rin_level = pow(10.0, gain/20.0);
}

static void mute_Rin(void) {
    Rin_type = NONE;
}

static void unmute_Rin(void) {
    Rin_type = CSS;
}

static void update_levels(int16_t rin, int16_t sin, int16_t sout, int16_t sgen)
{
    LRin = level_measurement_device(Rin_power_meter, rin);
    LSin = level_measurement_device(Sin_power_meter, sin);
    LSout = level_measurement_device(Sout_power_meter, sout);
    LSgen = level_measurement_device(Sgen_power_meter, sgen);
    if (LRin > maxLRin) maxLRin = LRin;
    if (LSin > maxLSin) maxLSin = LSin;
    if (LSout > maxLSout) maxLSout = LSout;
    if (LSgen > maxLSgen) maxLSgen = LSgen;
}

static void write_log_files(int16_t rout, int16_t sin)
{
    fprintf(flevel, "%f\t%f\t%f\t%f\n",LRin, LSin, LSout, LSgen);
    fprintf(fdump, "%d %d %d", ctx->tx, ctx->rx, ctx->clean);
    fprintf(fdump, " %d %d %d %d %d %d %d %d %d %d\n", ctx->clean_nlp, ctx->Ltx, 
	    ctx->Lrx, ctx->Lclean, 
	    (ctx->nonupdate_dwell > 0), ctx->adapt,  ctx->Lclean_bg, ctx->Pstates, 
	    ctx->Lbgn_upper, ctx->Lbgn);
}

static void run_test(float time, float units) {
    int     i;
    int     samples;
    int16_t rout, rin=0, sin;
    int16_t sgen=0, sout;
    float   rin_hoth_noise = 0;	
    float   sgen_hoth_noise = 0;	

    samples = time * units;

    for (i = 0;  i < samples;  i++) {

	switch(Rin_type) {
	case NONE:
	    rin = 0;
	    break;
	case CSS:
	    rin = clip(Rin_level*signal_amp(&local_css)*CSS_SCALE);
	    break;
	case HOTH:
	    rin_hoth_noise = rin_hoth_noise*0.625 + awgn(&rin_noise_source)*0.375;
	    rin = clip(Rin_level*rin_hoth_noise*HOTH_SCALE); 
	    break;
	case TONE:
            tone_gen(&rin_tone_state, &rin, 1);
	    break;
	}

	switch(Sgen_type) {
	case NONE:
	    sgen = 0;
	    break;
	case CSS:
	    sgen = clip(Sgen_level*signal_amp(&far_css)*CSS_SCALE);
	    break;
	case HOTH:
	    sgen_hoth_noise = sgen_hoth_noise*0.625 + awgn(&sgen_noise_source)*0.375;
	    sgen = clip(Sgen_level*sgen_hoth_noise*HOTH_SCALE); 
	    break;
	case TONE:
            tone_gen(&sgen_tone_state, &sgen, 1);
	    break;
	}

        rout = echo_can_hpf_tx(ctx, rin);
	channel_model(&rout, &sin, rin, sgen);
	sout = echo_can_update(ctx, rout, sin);
	update_levels(rin, sin, sout, sgen);
	write_log_files(rout, sin);
	
	/* now test for fail condition */
	if (test_callback != NULL) {
	    if ( (failed == FALSE) && (test_callback() == FALSE)) {
		/* test has failed */
		failed = TRUE;
	    }
	}
	    
	/* stop clock on fail - points to time of failure in test */

	if (failed == FALSE)
	    test_clock += (float)1/SAMPLE_RATE;
    }
}

static void print_title(const char *title) {
    if (quiet == FALSE) 
	printf(title);
}

static void print_results(void) {

    if (quiet == FALSE) 
	printf("test  model  ERL   time     Max Rin  Max Sin  Max Sgen  Max Sout  Result\n");
    printf("%-4s  %-1d      %-5.1f%6.2fs%9.2f%9.2f%10.2f%10.2f   ", 
	   test_name, model_number, 20.0*log10(erl), 
	   test_clock, maxLRin, maxLSin, maxLSgen, maxLSout);
    if (failed == TRUE)
	printf("FAIL\n");
    else
	printf("PASS\n");
}

static int test_2a(void) {
    if (LSout > -65.0) 
	return FALSE;
    else
	return TRUE;
}

static int test_2c(void) {
    if (LSout > maxHoth) 
	return FALSE;
    else
	return TRUE;
}

static int test_3a(void) {
    if (LSout > maxLSgen) 
	return FALSE;
    else
	return TRUE;
}

static int test_3b(void) {
    if (LSout > threshold) 
	return FALSE;
    else
	return TRUE;
}

static int test_3c_t2(void) {
    if (LSout > maxLSgen) 
	return FALSE;
    else
	return TRUE;
}

static int test_3c_t4t5(void) {
    if (LSout > (maxLSgen+6.0)) 
	return FALSE;
    else
	return TRUE;
}

static int test_9(void) {
    if (fabs(LSout - LSgen) > 2.0) 
	return FALSE;
    else
	return TRUE;
}

#define N_TESTS 9
static const char *supported_tests[] = {"ut1", "2aa", "2ca", "3a", "3ba", 
					"3bb", "3c", "6", "9"};

static int is_test_supported(char *test) {
    int i;
    for(i=0; i<N_TESTS; i++) {
	if (!strcasecmp(test, supported_tests[i]))
	    return TRUE;
    }

    return FALSE;
}

/* dump estimate echo response */
static void dump_h(void) {
    int i;
    FILE *f = fopen("h.txt","wt");
    for(i=0; i<TEST_EC_TAPS; i++) {
	fprintf(f, "%f\n", (float)ctx->fir_taps16[0][i]/(1<<15));
    }
    fclose(f);
}
       
int main(int argc, char *argv[])
{
    //awgn_state_t local_noise_source;
    int i;
    //int j;
    //int k;
    //tone_gen_descriptor_t tone_desc;
    //tone_gen_state_t tone_state;
    //int16_t local_sound[40000];
    //int local_max;
    //int local_cur;
    int far_cur;
    int result_cur;
    AFfilehandle txfile, rxfile, ecfile;
    time_t now;
    int tone_burst_step;
    float X_level, Sgen_leveldB;
#ifdef FIXED
    int xvrx[NZEROS+1], yvrx[NPOLES+1];
    int xvtx[NZEROS+1], yvtx[NPOLES+1];
#else
    float xvrx[NZEROS+1], yvrx[NPOLES+1];
    float xvtx[NZEROS+1], yvtx[NPOLES+1];
#endif

    int file_mode;
    float tmp;
    int   cng;
    int   hpf;

    /* default config ------------------------------------------------*/

    model_number = 1;
    erl = pow(10.0, -10.0/20.0);
    verbose = quiet = FALSE;
    file_mode = FALSE;
    Rin_level = pow(10.0, -15.0/20.0);
    Sgen_leveldB = -15.0;
    Sgen_level = pow(10.0, Sgen_leveldB/20.0);
    X_level = pow(10.0, 6.0/20.0);
    tone_burst_step = 0;
    txfile = rxfile = ecfile = NULL;
    munge = TRUE;
    cng = FALSE;
    hpf = TRUE;
    for(i=0; i<NPOLES+1; i++) {
      xvtx[i] = yvtx[i] = xvrx[i] = yvrx[i] = 0.0;
    }
    
    /* Check which tests we should run ----------------------------------------*/

    if (argc < 2) {
        fprintf(stderr, "Usage: echo [2aa] [2ca] [3a] [3ba] [3bb] [3c] [6] [9]\n"
		        "[-m ChannelModelNumber]\n"
		        "[-erl ERL_in_dB\n"
		        "[-file RinInputFile.wav SinInputFile.wav SoutOutputFile.wav\n"
		        "[-r RinLeveldBm0] [-s SgenLeveldBm0] [-x XLeveldB]\n"
		        "[-nomunge]\n"
		        "[-cng]\n"
		        "[-nohpf] Disable DC block HPF (-file mode)\n");

	exit(1);
    }

    for (i = 1;  i < argc;  i++)
    {
	if (is_test_supported(argv[i])) {
	}
	else if (strcmp(argv[i], "-m") == 0)
        {
            if (++i < argc)
                model_number = atoi(argv[i]);
        }
        else if (strcmp(argv[i], "-r") == 0)
        {
            if (++i < argc)
                Rin_level = pow(10.0, atof(argv[i])/20.0);
        }
        else if (strcmp(argv[i], "-s") == 0)
        {
            if (++i < argc) {
                Sgen_leveldB = atof(argv[i]);
                Sgen_level = pow(10.0, Sgen_leveldB/20.0);
	    }
        }
        else if (strcmp(argv[i], "-x") == 0)
        {
            if (++i < argc)
                X_level = pow(10.0, atof(argv[i])/20.0);
        }
        else if (strcmp(argv[i], "-erl") == 0)
        {
            if (++i < argc) {
                erl = atof(argv[i]);
		if (erl < 0.0) {
		    printf("ERL must be >= 0.0 dB\n");
		    exit(1);
		}
		erl = pow(10.0, -erl/20.0);
	    }
        }
        else if (strcmp(argv[i], "-v") == 0)
        {
            verbose = TRUE;
        }
        else if (strcmp(argv[i], "-q") == 0)
        {
            quiet = TRUE;
        }
        else if (strcmp(argv[i], "-file") == 0)
        {
	    file_mode = TRUE;
	    if (argc < (i+3)) {
		printf("not enough arguments for --file\n");
		exit(2);
	    }
	    txfile = af_file_open_for_read(argv[i+1]);	    
 	    rxfile = af_file_open_for_read(argv[i+2]);	    
 	    ecfile = af_file_open_for_write(argv[i+3]);
	    i += 3;
        }
        else if (strcmp(argv[i], "-nomunge") == 0)
        {
            munge = FALSE;
        }
        else if (strcmp(argv[i], "-cng") == 0)
        {
            cng = TRUE;
        }
        else if (strcmp(argv[i], "-nohpf") == 0)
        {
            hpf = FALSE;
        }
        else
        {
            fprintf(stderr, "Unknown test/option '%s' specified\n", argv[i]);
            exit(2);
        }
    }

    /* initialise a bunch of modules we need ------------------------------*/

    time(&now);

    ctx = echo_can_create(TEST_EC_TAPS, 0);
    awgn_init_dbm0(&rin_noise_source, 7162534, 0.0f);
    awgn_init_dbm0(&sgen_noise_source, 7162534, 0.0f);
    Rin_power_meter = level_measurement_device_create(0);
    Sgen_power_meter = level_measurement_device_create(0);
    Sin_power_meter = level_measurement_device_create(0);
    Sout_power_meter = level_measurement_device_create(0);
    if (channel_model_create(model_number))
    {
        fprintf(stderr, "    Failed to create line model\n");
        exit(2);
    }

    far_cur = 0;
    result_cur = 0;

    if (verbose == TRUE) {
	printf("ERL (linear)......: %6.2f (%5.2f)\n"
	       "Rin level (linear).: %6.2f (%5.2f)\n"
	       "Sgen level (linear): %6.2f (%5.2f)\n",
	       20.0*log10(erl), erl, 
	       20.0*log10(Rin_level), Rin_level,
	       20.0*log10(Sgen_level), Sgen_level);
    }

    fdump = fopen("dump.txt","wt");
    assert(fdump != NULL);
    flevel = fopen("level.txt","wt");
    assert(flevel != NULL);

    if (file_mode == TRUE) {
	/* process wave files instead of running tests, useful for
	   testing real world signals */
	int ntx, nrx, nec;
	int16_t rin, rout, sin, sout;
	int mode;

	mode =  ECHO_CAN_USE_ADAPTION | ECHO_CAN_USE_NLP;
	if (cng) 
	    mode |= ECHO_CAN_USE_CNG;
	else
	    mode |= ECHO_CAN_USE_CLIP;
	if (hpf) {
	    mode |= ECHO_CAN_USE_TX_HPF;
	    mode |= ECHO_CAN_USE_RX_HPF;
	}
	echo_can_adaption_mode(ctx, mode);
	do {
	    ntx = afReadFrames(txfile, AF_DEFAULT_TRACK, &rin, 1);
	    if (ntx < 0) {	   
		fprintf(stderr, "    Error reading tx sound file\n");
		exit(2);
	    }
	    nrx = afReadFrames(rxfile, AF_DEFAULT_TRACK, &sin, 1);
	    if (nrx < 0) {	   
		fprintf(stderr, "    Error reading rx sound file\n");
		exit(2);
	    }

	    rout = echo_can_hpf_tx(ctx, rin);
	    sout = echo_can_update(ctx, rout, sin);

	    nec = afWriteFrames(ecfile, AF_DEFAULT_TRACK, &sout, 1);
	    if (nec != 1) {
		fprintf(stderr, "    Error writing ec sound file\n");
		exit(2);
	    }

	    update_levels(rin, sin, sout, 0);
	    write_log_files(rin, sin);
	    
	} while (ntx && nrx);

	dump_h();

	afCloseFile(txfile);
	afCloseFile(rxfile);
	afCloseFile(ecfile);	
	exit(0);
    }

    signal_load(&local_css, "sound_c1_8k.wav");
    signal_load(&far_css, "sound_c3_8k.wav");

    strcpy(test_name, argv[1]);

    /* basic unit test used in e/c dvelopment */

    if (!strcasecmp(argv[1], "ut1")) {
	int16_t rin, sin, rout, sout, sgen;

	print_title("Performing Unit Test 1 - DC inputs\n");
	reset_all();
	echo_can_adaption_mode(ctx, ECHO_CAN_USE_ADAPTION);

	rout = rin = 2000;
	sin = 1000;
	sgen = 0;
	for(i=0; i<10; i++) {
	    rout = 2000+2*i;
	    sout = echo_can_update(ctx, rout, sin);
	    update_levels(rin, sin, sout, sgen);
	    write_log_files(rout, sin);
	}
	dump_h();
    }

    /* Test 1 - Steady state residual and returned echo level test */
    /* This functionality has been merged with test 2 in newer versions of G.168,
       so test 1 no longer exists. */

    /* Test 2 - Convergence and steady state residual and returned echo level test */

    /*
      NOTE: This test is only partially implemented, only the conidtion after
      1s is tested and I am still not sure if LSout == Lres in the part
      of the test after 1s.  
    */

    if (!strcasecmp(argv[1], "2aa")) {

	print_title("Performing test 2A(a) - Convergence with NLP enabled\n");
	reset_all();
	echo_can_adaption_mode(ctx, ECHO_CAN_USE_ADAPTION | ECHO_CAN_USE_NLP);

	/* initial zero input as reqd by G168 */

	mute_Rin();
	run_test(200, MSEC);
	unmute_Rin();

	/* Now test convergence */

	run_test(1, SEC);
	reset_meter_peaks();
	install_test_callback(test_2a);
	run_test(10, SEC);
	
	print_results();
    }

#ifdef OTHER_TESTS
    if ((test_list & PERFORM_TEST_2B))
    {
        printf("Performing test 2B - Re-convergence with NLP disabled\n");

        /* Test 2B - Re-convergence with NLP disabled */

        echo_can_flush(ctx);
        echo_can_adaption_mode(ctx, ECHO_CAN_USE_ADAPTION);

        /* Converge a canceller */

        signal_restart(&local_css);
        for (i = 0;  i < 800*2;  i++)
        {
            clean = echo_can_update(ctx, 0, 0);
            put_residue(clean);
        }

        for (i = 0;  i < SAMPLE_RATE*5;  i++)
        {
            tx = signal_amp(&local_css);
            channel_model(&tx, &rx, tx, 0);
            clean = echo_can_update(ctx, tx, rx);
            put_residue(clean);
#if defined(ENABLE_GUI)
            if (use_gui)
                echo_can_monitor_can_update(ctx->fir_taps16[ctx->tap_set], TEST_EC_TAPS);
#endif
        }
#if defined(ENABLE_GUI)
        if (use_gui)
            echo_can_monitor_can_update(ctx->fir_taps16[ctx->tap_set], TEST_EC_TAPS);
#endif
    }
#endif

    if (!strcasecmp(argv[1], "2ca")) {
	float SgenLeveldB;

	print_title("Performing test 2C(a) - Convergence with background noise present\n");
	reset_all();
	echo_can_adaption_mode(ctx, ECHO_CAN_USE_ADAPTION | ECHO_CAN_USE_NLP);

	/* Converge canceller with background noise */

	mute_Rin();
	run_test(200, MSEC);
	unmute_Rin();

	SgenLeveldB = 20.0*log10(Rin_level) - 15.0;
	if (SgenLeveldB > -30.0) SgenLeveldB = -30.0;
	set_Sgen(HOTH, SgenLeveldB);
	run_test(1, SEC);
	maxHoth = maxLSgen;

	/* After 1 second freeze adaption, switch off noise. */

	mute_Rin();
	run_test(150, MSEC);

	echo_can_adaption_mode(ctx, ECHO_CAN_USE_NLP);
	run_test(1, SEC);

	unmute_Rin();
	set_Sgen(NONE, 0.0);
	run_test(500, MSEC);

	/* now measure the echo */

	reset_meter_peaks();
	maxLSgen = maxHoth; /* keep this peak for print out but reset the rest */
	install_test_callback(test_2c);
	set_Sgen(NONE, 0.0);
	run_test(5, SEC);

	print_results();
    }

    /* Test 3 - Performance under double talk conditions */

    if (!strcasecmp(argv[1], "3a")) {
	print_title("Performing test 3A - Double talk test with low cancelled-end levels\n");
	reset_all();

	echo_can_adaption_mode(ctx, ECHO_CAN_USE_ADAPTION);
	set_Sgen(CSS, -15.0 + 20.0*log10(Rin_level));
	run_test(5, SEC);
	tmp = maxLSgen;

	/* now freeze adaption */

	echo_can_adaption_mode(ctx, 0);
	set_Sgen(NONE, 0.0);
	run_test(500, MSEC);

	/* Now measure the echo */

	reset_meter_peaks();
	maxLSgen = tmp;
	install_test_callback(test_3a);
	run_test(5, SEC);

	print_results();
    }

    if (!strcasecmp(argv[1], "3ba")) {
	float fig11;

	print_title("Performing test 3B(a) - Double talk stability test with high cancelled-end levels\n");
	reset_all();

	echo_can_adaption_mode(ctx, ECHO_CAN_USE_ADAPTION);
	run_test(5, SEC);
		
	/* Apply double talk */

	set_Sgen(CSS, 20.0*log10(Sgen_level));
	run_test(5, SEC);
	tmp = maxLSgen;

	/* freeze adaption and measure echo */

	mute_Rin();
	run_test(150, MSEC);

	echo_can_adaption_mode(ctx, 0);
	run_test(1, SEC);

	unmute_Rin();
	set_Sgen(NONE, 0.0);
	run_test(500, MSEC);

	/* Now measure the echo */

	fig11 = (25.0/30.0)*maxLRin - 30.0; /* pass/fail based on clean level @ tx peak */
	threshold = fig11 + 10.0;
	reset_meter_peaks();
	maxLSgen = tmp;
	install_test_callback(test_3b);
	run_test(5, SEC);

	print_results();
    }

    if (!strcasecmp(argv[1], "3bb")) {
	float fig11;

	print_title("Performing test 3B(b) - Double talk stability test with low cancelled-end levels\n");
	reset_all();

	echo_can_adaption_mode(ctx, ECHO_CAN_USE_ADAPTION);
	run_test(5, SEC);

	/* Apply double talk */

	set_Sgen(CSS,  20.0*log10(Rin_level) - 20.0*log10(X_level));
	run_test(5, SEC);
	tmp = maxLSgen;

	/* freeze adaption and measure echo */

	mute_Rin();
	run_test(150, MSEC);

	echo_can_adaption_mode(ctx, 0);
	run_test(1, SEC);

	unmute_Rin();
	set_Sgen(NONE, 0.0);
	run_test(500, MSEC);

	/* Now measure the echo */

	fig11 = (25.0/30.0)*maxLRin - 30.0; /* pass/fail based on clean level @ tx peak */
	threshold = fig11 + 3.0;
	reset_meter_peaks();
	maxLSgen = tmp;
	install_test_callback(test_3b);
	run_test(5, SEC);

	print_results();
    }

    if (!strcasecmp(argv[1], "3c")) {
	print_title("Performing test 3C - Double talk test under simulated conversation\n");
	reset_all();

	/* t1 (5.6s) - double talk */

	echo_can_adaption_mode(ctx, ECHO_CAN_USE_ADAPTION | ECHO_CAN_USE_NLP);
	set_Sgen(CSS,  Sgen_leveldB);
	run_test(5600, MSEC);

	/* t2 (1.4s) - to pass Sout <= Sgen */

	set_Sgen(NONE, 0.0);
	install_test_callback(test_3c_t2);
	run_test(1400, MSEC);

	/* t3 - (5s) - single talk to converge e/c */

	run_test(5000, MSEC);

	/* t4 - (5.6s) - double talk again */

	install_test_callback(test_3c_t4t5);
	set_Sgen(CSS,  Sgen_leveldB);
	run_test(5600, MSEC);

	/* t5 - (5.6s) - near end single talk  */

	mute_Rin();
	run_test(5600, MSEC);

	print_results();
    }

#ifdef OTHER_TESTS
    if ((test_list & PERFORM_TEST_4))
    {
        printf("Performing test 4 - Leak rate test\n");
        /* Test 4 - Leak rate test */
        echo_can_flush(ctx);
        echo_can_adaption_mode(ctx, ECHO_CAN_USE_ADAPTION);
        /* Converge a canceller */
        signal_restart(&local_css);
        for (i = 0;  i < SAMPLE_RATE*5;  i++)
        {
            tx = signal_amp(&local_css);
            channel_model(&tx, &rx, tx, 0);
            clean = echo_can_update(ctx, tx, rx);
            put_residue(clean);
        }
        /* Put 2 minutes of silence through it */
        for (i = 0;  i < SAMPLE_RATE*120;  i++)
        {
            clean = echo_can_update(ctx, 0, 0);
            put_residue(clean);
        }
        /* Now freeze it, and check if it is still well adapted. */
        echo_can_adaption_mode(ctx, 0);
        for (i = 0;  i < SAMPLE_RATE*5;  i++)
        {
            tx = signal_amp(&local_css);
            channel_model(&tx, &rx, tx, 0);
            clean = echo_can_update(ctx, tx, rx);
            put_residue(clean);
        }
        echo_can_adaption_mode(ctx, ECHO_CAN_USE_ADAPTION);
#if defined(ENABLE_GUI)
        if (use_gui)
            echo_can_monitor_can_update(ctx->fir_taps16[ctx->tap_set], TEST_EC_TAPS);
#endif
    }

    if ((test_list & PERFORM_TEST_5))
    {
        printf("Performing test 5 - Infinite return loss convergence test\n");
        /* Test 5 - Infinite return loss convergence test */
        echo_can_flush(ctx);
        echo_can_adaption_mode(ctx, ECHO_CAN_USE_ADAPTION);
        /* Converge the canceller */
        signal_restart(&local_css);
        for (i = 0;  i < SAMPLE_RATE*5;  i++)
        {
            tx = signal_amp(&local_css);
            channel_model(&tx, &rx, tx, 0);
            clean = echo_can_update(ctx, tx, rx);
            put_residue(clean);
        }
        /* Now stop echoing, and see we don't do anything unpleasant as the
           echo path is open looped. */
        for (i = 0;  i < SAMPLE_RATE*5;  i++)
        {
            tx = signal_amp(&local_css);
            rx = 0;
            tx = codec_munge(tx);
            clean = echo_can_update(ctx, tx, rx);
            put_residue(clean);
        }
#if defined(ENABLE_GUI)
        if (use_gui)
            echo_can_monitor_can_update(ctx->fir_taps16[ctx->tap_set], TEST_EC_TAPS);
#endif
    }

#endif

    if (!strcasecmp(argv[1], "6"))
    {
	int   k;
	float fig11;

        printf("Performing test 6 - Non-divergence on narrow-band signals\n");

	reset_all();
	echo_can_adaption_mode(ctx, ECHO_CAN_USE_ADAPTION);
	run_test(5, SEC);

        /* Now put 5s bursts of a list of tones through the converged canceller, and check
           that nothing unpleasant happens. */

        for (k = 0;  tones_6_4_2_7[k][0];  k++)
        {
	    tone_gen_descriptor_t tone_desc;
	    
	    /* 5 secs of each tone */

	    echo_can_adaption_mode(ctx, ECHO_CAN_USE_ADAPTION);
	    set_Rin(TONE, 20.0*log10(Rin_level)); /* level actually set by next func */
            make_tone_gen_descriptor(&tone_desc,
                                     tones_6_4_2_7[k][0],
                                     -11,
                                     tones_6_4_2_7[k][1],
                                     -9,
                                     1,
                                     0,
                                     0,
                                     0,
                                     1);
            tone_gen_init(&rin_tone_state, &tone_desc);
	    run_test(5, SEC);
	}

	/* disable adaption, back to speech */

	echo_can_adaption_mode(ctx, 0);
	set_Rin(CSS, 20.0*log10(Rin_level)); 
	run_test(1, SEC);

	/* now test convergence as per test 2 fig 11 */

	fig11 = (25.0/30.0)*maxLRin - 30.0; /* pass/fail based on clean level @ tx peak */
	threshold = fig11 + 10.0;
	reset_meter_peaks();
	install_test_callback(test_3b);
	run_test(5, SEC);	

	print_results();
    }

#ifdef OTHER_TESTS

    if ((test_list & PERFORM_TEST_7))
    {
        printf("Performing test 7 - Stability\n");
        /* Test 7 - Stability */
        /* Put tones through an unconverged canceller, and check nothing unpleasant
           happens. */
        echo_can_flush(ctx);
        echo_can_adaption_mode(ctx, ECHO_CAN_USE_ADAPTION);
        make_tone_gen_descriptor(&tone_desc,
                                 tones_6_4_2_7[0][0],
                                 -11,
                                 tones_6_4_2_7[0][1],
                                 -9,
                                 1,
                                 0,
                                 0,
                                 0,
                                 1);
        tone_gen_init(&tone_state, &tone_desc);
        j = 0;
        for (i = 0;  i < 120;  i++)
        {
            local_max = tone_gen(&tone_state, local_sound, SAMPLE_RATE);
            for (j = 0;  j < SAMPLE_RATE;  j++)
            {
                tx = local_sound[j];
                channel_model(&tx, &rx, tx, 0);
                clean = echo_can_update(ctx, tx, rx);
                put_residue(clean);
            }
#if defined(ENABLE_GUI)
            if (use_gui)
            {
                echo_can_monitor_can_update(ctx->fir_taps16[ctx->tap_set], TEST_EC_TAPS);
                echo_can_monitor_update_display();
                usleep(100000);
            }
#endif
        }
#if defined(ENABLE_GUI)
        if (use_gui)
            echo_can_monitor_can_update(ctx->fir_taps16[ctx->tap_set], TEST_EC_TAPS);
#endif
    }

    if ((test_list & PERFORM_TEST_8))
    {
        printf("Performing test 8 - Non-convergence on No 5, 6, and 7 in-band signalling\n");
        /* Test 8 - Non-convergence on No 5, 6, and 7 in-band signalling */
        fprintf(stderr, "Test 8 not yet implemented\n");
    }
#endif

    if (!strcasecmp(argv[1], "9"))
    {
        printf("Performing test 9 - Comfort noise test\n");

        echo_can_flush(ctx);
        echo_can_adaption_mode(ctx,   ECHO_CAN_USE_ADAPTION 
			            | ECHO_CAN_USE_NLP 
			            | ECHO_CAN_USE_CNG);

        /* Test 9 Part 1 - matching */

	set_Sgen(HOTH, -45.0);
	mute_Rin();
	run_test(5, SEC); /* should be 30s but I wanted to speed up sim */
	set_Rin(HOTH, -10.0);
	run_test(2, SEC); 

	reset_meter_peaks();
	install_test_callback(test_9);
	run_test(700, MSEC); 

        /* Test 9 Part 2 - adjustment down */

	install_test_callback(NULL);
	set_Sgen(HOTH, -55.0);
	mute_Rin();
	run_test(5, SEC); /* should be 10s but I wanted to speed up sim */
	set_Rin(HOTH, -10.0);
	run_test(2, SEC); 

	reset_meter_peaks();
	install_test_callback(test_9);
	run_test(700, MSEC); 

        /* Test 9 Part 3 - adjustment up */

	install_test_callback(NULL);
	set_Sgen(HOTH, -45.0);
	mute_Rin();
	run_test(5, SEC); /* should be 10s but I wanted to speed up sim */
	set_Rin(HOTH, -10.0);
	run_test(2, SEC); 

	reset_meter_peaks();
	install_test_callback(test_9);
	run_test(700, MSEC); 

 	print_results();
    }

#ifdef OTHER_TESTS
    /* Test 10 - FAX test during call establishment phase */
    if ((test_list & PERFORM_TEST_10A))
    {
        printf("Performing test 10A - Canceller operation on the calling station side\n");
        /* Test 10A - Canceller operation on the calling station side */
        fprintf(stderr, "Test 10A not yet implemented\n");
    }

    if ((test_list & PERFORM_TEST_10B))
    {
        printf("Performing test 10B - Canceller operation on the called station side\n");
        /* Test 10B - Canceller operation on the called station side */
        fprintf(stderr, "Test 10B not yet implemented\n");
    }

    if ((test_list & PERFORM_TEST_10C))
    {
        printf("Performing test 10C - Canceller operation on the calling station side during page\n"
               "transmission and page breaks (for further study)\n");
        /* Test 10C - Canceller operation on the calling station side during page
                      transmission and page breaks (for further study) */
        fprintf(stderr, "Test 10C not yet implemented\n");
    }

    if ((test_list & PERFORM_TEST_11))
    {
        printf("Performing test 11 - Tandem echo canceller test (for further study)\n");
        /* Test 11 - Tandem echo canceller test (for further study) */
        fprintf(stderr, "Test 11 not yet implemented\n");
    }

    if ((test_list & PERFORM_TEST_12))
    {
        printf("Performing test 12 - Residual acoustic echo test (for further study)\n");
        /* Test 12 - Residual acoustic echo test (for further study) */
        fprintf(stderr, "Test 12 not yet implemented\n");
    }

    if ((test_list & PERFORM_TEST_13))
    {
        printf("Performing test 13 - Performance with ITU-T low-bit rate coders in echo path (Optional, under study)\n");
        /* Test 13 - Performance with ITU-T low-bit rate coders in echo path
                     (Optional, under study) */
        fprintf(stderr, "Test 13 not yet implemented\n");
    }

    if ((test_list & PERFORM_TEST_14))
    {
        printf("Performing test 14 - Performance with V-series low-speed data modems\n");
        /* Test 14 - Performance with V-series low-speed data modems */
        fprintf(stderr, "Test 14 not yet implemented\n");
    }

    if ((test_list & PERFORM_TEST_15))
    {
        printf("Performing test 15 - PCM offset test (Optional)\n");
        /* Test 15 - PCM offset test (Optional) */
        fprintf(stderr, "Test 15 not yet implemented\n");
    }

    echo_can_free(ctx);

    signal_free(&local_css);
    signal_free(&far_css);

    if (afCloseFile(resulthandle) != 0)
    {
        fprintf(stderr, "    Cannot close speech file '%s'\n", "result_sound.wav");
        exit(2);
    }
    if (afCloseFile(residuehandle) != 0)
    {
        fprintf(stderr, "    Cannot close speech file '%s'\n", "residue_sound.wav");
        exit(2);
    }
    afFreeFileSetup(filesetup);
    afFreeFileSetup(filesetup2);

#if defined(XYZZY)
    for (j = 0;  j < ctx->taps;  j++)
    {
        for (i = 0;  i < coeff_index;  i++)
            fprintf(stderr, "%d ", coeffs[i][j]);
        fprintf(stderr, "\n");
    }
#endif
#endif
    if (verbose == TRUE)
	printf("Run time %lds\n", time(NULL) - now);
    
#if defined(ENABLE_GUI)
    if (use_gui)
        echo_can_monitor_wait_to_end();
#endif


    fclose(fdump);
    fclose(flevel);

    return  0;
}
/*- End of function --------------------------------------------------------*/
/*- End of file ------------------------------------------------------------*/
