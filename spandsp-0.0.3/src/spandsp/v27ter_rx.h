/*
 * SpanDSP - a series of DSP components for telephony
 *
 * v27ter_rx.h - ITU V.27ter modem receive part
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
 * $Id: v27ter_rx.h,v 1.34 2006/10/24 13:45:28 steveu Exp $
 */

/*! \file */

#if !defined(_V27TER_RX_H_)
#define _V27TER_RX_H_

/*! \page v27ter_rx_page The V.27ter receiver

\section v27ter_rx_page_sec_1 What does it do?
The V.27ter receiver implements the receive side of a V.27ter modem. This can operate
at data rates of 4800 and 2400 bits/s. The audio input is a stream of 16 bit samples,
at 8000 samples/second. The transmit and receive side of V.27ter modems operate
independantly. V.27ter is mostly used for FAX transmission, where it provides the
standard 4800 bits/s rate (the 2400 bits/s mode is not used for FAX). 

\section v27ter_rx_page_sec_2 How does it work?
V.27ter defines two modes of operation. One uses 8-PSK at 1600 baud, giving 4800bps.
The other uses 4-PSK at 1200 baud, giving 2400bps. A training sequence is specified
at the start of transmission, which makes the design of a V.27ter receiver relatively
straightforward.
*/

/* Target length for the equalizer is about 43 taps for 4800bps and 32 taps for 2400bps
   to deal with the worst stuff in V.56bis. */
#define V27TER_EQUALIZER_PRE_LEN        15  /* this much before the real event */
#define V27TER_EQUALIZER_POST_LEN       15  /* this much after the real event */
#define V27TER_EQUALIZER_MASK           63  /* one less than a power of 2 >= (V27TER_EQUALIZER_PRE_LEN + 1 + V27TER_EQUALIZER_POST_LEN) */

#define V27TER_RX_4800_FILTER_STEPS     27
#define V27TER_RX_2400_FILTER_STEPS     27

#if V27TER_RX_4800_FILTER_STEPS > V27TER_RX_2400_FILTER_STEPS
#define V27TER_RX_FILTER_STEPS V27TER_RX_4800_FILTER_STEPS
#else
#define V27TER_RX_FILTER_STEPS V27TER_RX_2400_FILTER_STEPS
#endif

/*!
    V.27ter modem receive side descriptor. This defines the working state for a
    single instance of a V.27ter modem receiver.
*/
typedef struct
{
    /*! \brief The bit rate of the modem. Valid values are 2400 and 4800. */
    int bit_rate;
    /*! \brief The callback function used to put each bit received. */
    put_bit_func_t put_bit;
    /*! \brief A user specified opaque pointer passed to the put_bit routine. */
    void *user_data;
    /*! \brief A callback function which may be enabled to report every symbol's
               constellation position. */
    qam_report_handler_t *qam_report;
    /*! \brief A user specified opaque pointer passed to the qam_report callback
               routine. */
    void *qam_user_data;

    /*! \brief The route raised cosine (RRC) pulse shaping filter buffer. */
    float rrc_filter[2*V27TER_RX_FILTER_STEPS];
    /*! \brief Current offset into the RRC pulse shaping filter buffer. */
    int rrc_filter_step;

    /*! \brief The register for the training and data scrambler. */
    unsigned int scramble_reg;
    /*! \brief A counter for the number of consecutive bits of repeating pattern through
               the scrambler. */
    int scrambler_pattern_count;
    int in_training;
    int training_bc;
    int training_count;
    float training_error;
    int carrier_present;
    int16_t last_sample;
    /*! \brief TRUE if the previous trained values are to be reused. */
    int old_train;

    /*! \brief The current phase of the carrier (i.e. the DDS parameter). */
    uint32_t carrier_phase;
    /*! \brief The update rate for the phase of the carrier (i.e. the DDS increment). */
    int32_t carrier_phase_rate;
    /*! \brief The carrier update rate saved for reuse when using short training. */
    int32_t carrier_phase_rate_save;
    float carrier_track_p;
    float carrier_track_i;

    power_meter_t power;
    int32_t carrier_on_power;
    int32_t carrier_off_power;
    float agc_scaling;
    float agc_scaling_save;

    int constellation_state;

    float eq_delta;
    /*! \brief The adaptive equalizer coefficients */
    complexf_t eq_coeff[V27TER_EQUALIZER_PRE_LEN + 1 + V27TER_EQUALIZER_POST_LEN];
    complexf_t eq_coeff_save[V27TER_EQUALIZER_PRE_LEN + 1 + V27TER_EQUALIZER_POST_LEN];
    complexf_t eq_buf[V27TER_EQUALIZER_MASK + 1];
    /*! \brief Current offset into equalizer buffer. */
    int eq_step;
    int eq_put_step;
    int eq_skip;

    /*! \brief Integration variable for damping the Gardner algorithm tests. */
    int gardner_integrate;
    /*! \brief Current step size of Gardner algorithm integration. */
    int gardner_step;
    /*! \brief The total symbol timing correction since the carrier came up.
               This is only for performance analysis purposes. */
    int total_baud_timing_correction;
    /*! \brief The current fractional phase of the baud timing. */
    int baud_phase;

    /*! \brief Starting phase angles for the coarse carrier aquisition step. */
    int32_t start_angles[2];
    /*! \brief History list of phase angles for the coarse carrier aquisition step. */
    int32_t angles[16];
    /*! \brief Error and flow logging control */
    logging_state_t logging;
} v27ter_rx_state_t;

#ifdef __cplusplus
extern "C" {
#endif

/*! Initialise a V.27ter modem receive context.
    \brief Initialise a V.27ter modem receive context.
    \param s The modem context.
    \param rate The bit rate of the modem. Valid values are 2400 and 4800.
    \param put_bit The callback routine used to put the received data.
    \param user_data An opaque pointer passed to the put_bit routine.
    \return A pointer to the modem context, or NULL if there was a problem. */
v27ter_rx_state_t *v27ter_rx_init(v27ter_rx_state_t *s, int rate, put_bit_func_t put_bit, void *user_data);

/*! Reinitialise an existing V.27ter modem receive context.
    \brief Reinitialise an existing V.27ter modem receive context.
    \param s The modem context.
    \param rate The bit rate of the modem. Valid values are 2400 and 4800.
    \param old_train TRUE if a previous trained values are to be reused.
    \return 0 for OK, -1 for bad parameter */
int v27ter_rx_restart(v27ter_rx_state_t *s, int rate, int old_train);

/*! Release a V.27ter modem receive context.
    \brief Release a V.27ter modem receive context.
    \param s The modem context.
    \return 0 for OK */
int v27ter_rx_release(v27ter_rx_state_t *s);

/*! Change the put_bit function associated with a V.27ter modem receive context.
    \brief Change the put_bit function associated with a V.27ter modem receive context.
    \param s The modem context.
    \param put_bit The callback routine used to handle received bits.
    \param user_data An opaque pointer. */
void v27ter_rx_set_put_bit(v27ter_rx_state_t *s, put_bit_func_t put_bit, void *user_data);

/*! Process a block of received V.27ter modem audio samples.
    \brief Process a block of received V.27ter modem audio samples.
    \param s The modem context.
    \param amp The audio sample buffer.
    \param len The number of samples in the buffer.
    \return The number of samples unprocessed.
*/
int v27ter_rx(v27ter_rx_state_t *s, const int16_t amp[], int len);

/*! Get a snapshot of the current equalizer coefficients.
    \brief Get a snapshot of the current equalizer coefficients.
    \param coeffs The vector of complex coefficients.
    \return The number of coefficients in the vector. */
int v27ter_rx_equalizer_state(v27ter_rx_state_t *s, complexf_t **coeffs);

/*! Get the current received carrier frequency.
    \param s The modem context.
    \return The frequency, in Hertz. */
float v27ter_rx_carrier_frequency(v27ter_rx_state_t *s);

/*! Get the current symbol timing correction since startup.
    \param s The modem context.
    \return The correction. */
float v27ter_rx_symbol_timing_correction(v27ter_rx_state_t *s);

/*! Get a current received signal power.
    \param s The modem context.
    \return The signal power, in dBm0. */
float v27ter_rx_signal_power(v27ter_rx_state_t *s);

/*! Set the power level at which the carrier detection will cut in
    \param s The modem context.
    \param cutoff The signal cutoff power, in dBm0. */
void v27ter_rx_signal_cutoff(v27ter_rx_state_t *s, float cutoff);

/*! Set a handler routine to process QAM status reports
    \param s The modem context.
    \param handler The handler routine.
    \param user_data An opaque pointer passed to the handler routine. */
void v27ter_rx_set_qam_report_handler(v27ter_rx_state_t *s, qam_report_handler_t *handler, void *user_data);

#ifdef __cplusplus
}
#endif

#endif
/*- End of file ------------------------------------------------------------*/
