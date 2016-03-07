/*
 * SpanDSP - a series of DSP components for telephony
 *
 * sig_tone.h - Signalling tone processing for the 2280Hz, 2600Hz and similar
 *              signalling tone used in older protocols.
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
 * $Id: sig_tone.h,v 1.7 2006/10/24 13:22:02 steveu Exp $
 */

/*! \file */

/*! \page sig_tone_page The signaling tone processor
\section sig_tone_sec_1 What does it do?
The signaling tone processor handles the 2280Hz, 2400Hz and 2600Hz tones, used
in many analogue signaling procotols, and digital ones derived from them.

\section sig_tone_sec_2 How does it work?
TBD
*/

#if !defined(_SIG_TONE_H_)
#define _SIG_TONE_H_

typedef int (*sig_tone_func_t)(void *user_data, int what);

/* The optional tone sets */
enum
{
    SIG_TONE_2280HZ = 1,
    SIG_TONE_2600HZ,
    SIG_TONE_2400HZ_2600HZ
};

#define SIG_TONE_1_PRESENT          0x001
#define SIG_TONE_1_CHANGE           0x002
#define SIG_TONE_2_PRESENT          0x004
#define SIG_TONE_2_CHANGE           0x008
#define SIG_TONE_TX_PASSTHROUGH     0x010
#define SIG_TONE_RX_PASSTHROUGH     0x020
#define SIG_TONE_UPDATE_REQUEST     0x100

/*!
    Signaling tone descriptor. This defines the working state for a
    single instance of the transmit and receive sides of a signaling
    tone processor.
*/
typedef struct
{
    /*! \brief The tones used. */
    int tone_freq[2];
    /*! \brief The high and low tone amplitudes. */
    int tone_amp[2];

    /*! \brief The delay, in audio samples, before the high level tone drops
               to a low level tone. */
    int high_low_timeout;

    /*! \brief Some signaling tone detectors use a sharp initial filter,
               changing to a broader band filter after some delay. This
               parameter defines the delay. 0 means it never changes. */
    int sharp_flat_timeout;

    /*! \brief Parameters to control the behaviour of the notch filter, used
               to remove the tone from the voice path in some protocols. */
    int notch_lag_time;
    int notch_allowed;

    /*! \brief The tone on persistence check, in audio samples. */
    int tone_on_check_time;
    /*! \brief The tone off persistence check, in audio samples. */
    int tone_off_check_time;

    /*! \brief The coefficients for the cascaded bi-quads notch filter. */
    int32_t notch_a1[3];
    int32_t notch_b1[3];
    int32_t notch_a2[3];
    int32_t notch_b2[3];
    int notch_postscale;

    /*! \brief Flat mode bandpass bi-quad parameters */
    int32_t broad_a[3];
    int32_t broad_b[3];
    int broad_postscale;

    /*! \brief The coefficients for the post notch leaky integrator. */
    int32_t notch_slugi;
    int32_t notch_slugp;

    /*! \brief The coefficients for the post modulus leaky integrator in the
               unfiltered data path.  The prescale value incorporates the
               detection ratio. This is called the guard ratio in some
               protocols. */
    int32_t unfiltered_slugi;
    int32_t unfiltered_slugp;

    /*! \brief The coefficients for the post modulus leaky integrator in the
               bandpass filter data path. */
    int32_t broad_slugi;
    int32_t broad_slugp;

    /*! \brief Masks which effectively threshold the notched, weighted and
               bandpassed data. */
    int32_t notch_threshold;
    int32_t unfiltered_threshold;
    int32_t broad_threshold;
} sig_tone_descriptor_t;

typedef struct
{
    /*! \brief The callback function used to handle signaling changes. */
    sig_tone_func_t sig_update;
    /*! \brief A user specified opaque pointer passed to the callback function. */
    void *user_data;

    /*! \brief Transmit side parameters */
    sig_tone_descriptor_t *desc;
    int32_t phase_rate[2];
    int32_t tone_scaling[2];
    uint32_t phase_acc[2];

    int high_low_timer;

    /*! \brief The z's for the notch filter */
    int32_t notch_z1[3];
    int32_t notch_z2[3];

    /*! \brief The z's for the weighting/bandpass filter. */
    int32_t broad_z[3];

    /*! \brief The z's for the integrators. */
    int32_t notch_zl;
    int32_t broad_zl;

    /*! \brief The thresholded data. */
    int32_t mown_notch;
    int32_t mown_bandpass;

    int flat_mode;
    int tone_present;
    int notch_enabled;
    int flat_mode_timeout;
    int notch_insertion_timeout;
    int tone_persistence_timeout;
    
    int current_tx_tone;
    int current_tx_timeout;
    int signaling_state_duration;
} sig_tone_state_t;

/*! Initialise a signaling tone context.
    \brief Initialise a signaling tone context.
    \param s The signaling tone context.
    \param tone_type The type of signaling tone.
    \param sig_update Callback function to handle signaling updates.
    \param user_data An opaque pointer.
    \return A pointer to the signalling tone context, or NULL if there was a problem. */
sig_tone_state_t *sig_tone_init(sig_tone_state_t *s, int tone_type, sig_tone_func_t sig_update, void *user_data);

/*! Process a block of received audio samples.
    \brief Process a block of received audio samples.
    \param s The signaling tone context.
    \param amp The audio sample buffer.
    \param len The number of samples in the buffer.
    \return The number of samples unprocessed. */
int sig_tone_rx(sig_tone_state_t *s, int16_t amp[], int len);

/*! Generate a block of signaling tone audio samples.
    \brief Generate a block of signaling tone audio samples.
    \param s The signaling tone context.
    \param amp The audio sample buffer.
    \param len The number of samples to be generated.
    \return The number of samples actually generated. */
int sig_tone_tx(sig_tone_state_t *s, int16_t amp[], int len);

#endif
/*- End of file ------------------------------------------------------------*/
