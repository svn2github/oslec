/*
 * SpanDSP - a series of DSP components for telephony
 *
 * time_scale.h - Time scaling for linear speech data
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
 * $Id: time_scale.h,v 1.7 2006/10/24 13:45:28 steveu Exp $
 */

#if !defined(_TIME_SCALE_H_)
#define _TIME_SCALE_H_

/*! \page time_scale_page Time scaling speech
\section time_scale_page_sec_1 What does it do?
The time scaling module allows speech files to be played back at a
different speed, from the speed at which they were recorded. If this
were done by simply speeding up or slowing down replay, the pitch of
the voice would change, and sound very odd. This modules keeps the pitch
of the voice normal.

\section time_scale_page_sec_2 How does it work?
The time scaling module is based on the Pointer Interval Controlled
OverLap and Add (PICOLA) method, developed by Morita Naotaka.
Mikio Ikeda has an excellent web page on this subject at
http://keizai.yokkaichi-u.ac.jp/~ikeda/research/picola.html
There is also working code there. This implementation uses
exactly the same algorithms, but the code is a complete rewrite.
Mikio's code batch processes files. This version works incrementally
on streams, and allows multiple streams to be processed concurrently.
*/

#define TIME_SCALE_MIN_PITCH    60
#define TIME_SCALE_MAX_PITCH    250
#define TIME_SCALE_BUF_LEN      (2*SAMPLE_RATE/TIME_SCALE_MIN_PITCH)

typedef struct
{
    double rate;
    double rcomp;
    double rate_nudge;
    int fill;
    int lcp;
    int16_t buf[TIME_SCALE_BUF_LEN];
} time_scale_t;

#ifdef __cplusplus
extern "C" {
#endif

/*! Initialise a time scale context. This must be called before the first
    use of the context, to initialise its contents.
    \brief Initialise a time scale context.
    \param s The time scale context.
    \param rate The ratio between the output speed and the input speed.
    \return 0 if initialised OK, else -1. */
int time_scale_init(time_scale_t *s, float rate);

/*! Change the time scale rate.
    \brief Change the time scale rate.
    \param s The time scale context.
    \param rate The ratio between the output speed and the input speed.
    \return 0 if changed OK, else -1. */
int time_scale_rate(time_scale_t *s, float rate);

/*! Time scale a chunk of audio samples.
    \brief Time scale a chunk of audio samples.
    \param s The time sclae context.
    \param out The output audio sample buffer.
    \param in The input audio sample buffer.
    \param len The number of input samples.
    \return The number of output samples.
*/
int time_scale(time_scale_t *s, int16_t out[], int16_t in[], int len);

#ifdef __cplusplus
}
#endif

#endif
/*- End of file ------------------------------------------------------------*/
