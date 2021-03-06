/*
 * SpanDSP - a series of DSP components for telephony
 *
 * v27ter_tx.h - ITU V.27ter modem transmit part
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
 * $Id: v27ter_tx.h,v 1.25 2006/10/24 13:45:28 steveu Exp $
 */

/*! \file */

#if !defined(_V27TER_TX_H_)
#define _V27TER_TX_H_

/*! \page v27ter_tx_page The V.27ter transmitter
\section v27ter_tx_page_sec_1 What does it do?
The V.27ter transmitter implements the transmit side of a V.27ter modem. This
can operate at data rates of 4800 and 2400 bits/s. The audio output is a stream
of 16 bit samples, at 8000 samples/second. The transmit and receive side of
V.27ter modems operate independantly. V.27ter is used for FAX transmission,
where it provides the standard 4800 and 2400 bits/s rates. 

\section v27ter_tx_page_sec_2 How does it work?
V.27ter uses DPSK modulation. A common method of producing a DPSK modulated
signal is to use a sampling rate which is a multiple of the baud rate. The raw
signal is then a series of complex pulses, each an integer number of samples
long. These can be shaped, using a suitable complex filter, and multiplied by a
complex carrier signal to produce the final DPSK signal for transmission. 

The pulse shaping filter for V.27ter is defined in the spec. It is a root raised
cosine filter with 50% excess bandwidth. 

The sampling rate for our transmitter is defined by the channel - 8000 samples/s.
This is a multiple of the baud rate at 4800 bits/s (8-PSK at 1600 baud, 5 samples per
symbol), but not at 2400 bits/s (4-PSK at 1200 baud, 20/3 samples per symbol). The baud
interval is actually 20/3 sample periods at 2400bis/s. A symmetric FIR is used to
apply root raised cosine filtering in the 4800bits/s mode. In the 2400bits/s mode
a polyphase FIR filter is used. This consists of 20 sets of coefficients, offering
zero to 19/20ths of a baud phase shift as well as root raised cosine filtering.
The appropriate coefficient set is chosen for each signal sample generated.

The carrier is generated using the DDS method. Using 2 second order resonators,
started in quadrature, might be more efficient, as it would have less impact on
the processor cache than a table lookup approach. However, the DDS approach
suits the receiver better, so then same signal generator is also used for the
transmitter.
*/

#define V27TER_TX_FILTER_STEPS      9

/*!
    V.27ter modem transmit side descriptor. This defines the working state for a
    single instance of a V.27ter modem transmitter.
*/
typedef struct
{
    /*! \brief The bit rate of the modem. Valid values are 2400 and 4800. */
    int bit_rate;
    /*! \brief The callback function used to get the next bit to be transmitted. */
    get_bit_func_t get_bit;
    /*! \brief A user specified opaque pointer passed to the callback function. */
    void *user_data;

    /*! \brief The gain factor needed to achieve the specified output power at 2400bps. */
    float gain_2400;
    /*! \brief The gain factor needed to achieve the specified output power at 4800bps. */
    float gain_4800;

    /*! \brief The route raised cosine (RRC) pulse shaping filter buffer. */
    complexf_t rrc_filter[2*V27TER_TX_FILTER_STEPS];
    /*! \brief Current offset into the RRC pulse shaping filter buffer. */
    int rrc_filter_step;
    
    /*! \brief The register for the training and data scrambler. */
    unsigned int scramble_reg;
    /*! \brief A counter for the number of consecutive bits of repeating pattern through
               the scrambler. */
    int scrambler_pattern_count;
    /*! \brief TRUE if transmitting the training sequence, or shutting down transmission.
               FALSE if transmitting user data. */
    int in_training;
    /*! \brief A counter used to track progress through sending the training sequence. */
    int training_step;

    /*! \brief The current phase of the carrier (i.e. the DDS parameter). */
    uint32_t carrier_phase;
    /*! \brief The update rate for the phase of the carrier (i.e. the DDS increment). */
    int32_t carrier_phase_rate;
    /*! \brief The current fractional phase of the baud timing. */
    int baud_phase;
    /*! \brief The code number for the current position in the constellation. */
    int constellation_state;
    /*! \brief The get_bit function in use at any instant. */
    get_bit_func_t current_get_bit;
    /*! \brief Error and flow logging control */
    logging_state_t logging;
} v27ter_tx_state_t;

#ifdef __cplusplus
extern "C" {
#endif

/*! Adjust a V.27ter modem transmit context's power output.
    \brief Adjust a V.27ter modem transmit context's output power.
    \param s The modem context.
    \param power The power level, in dBm0 */
void v27ter_tx_power(v27ter_tx_state_t *s, float power);

/*! Initialise a V.27ter modem transmit context.
    \brief Initialise a V.27ter modem transmit context.
    \param s The modem context.
    \param rate The bit rate of the modem. Valid values are 2400 and 4800.
    \param tep TRUE is the optional TEP tone is to be transmitted.
    \param get_bit The callback routine used to get the data to be transmitted.
    \param user_data An opaque pointer.
    \return A pointer to the modem context, or NULL if there was a problem. */
v27ter_tx_state_t *v27ter_tx_init(v27ter_tx_state_t *s, int rate, int tep, get_bit_func_t get_bit, void *user_data);

/*! Reinitialise an existing V.27ter modem transmit context, so it may be reused.
    \brief Reinitialise an existing V.27ter modem transmit context.
    \param s The modem context.
    \param rate The bit rate of the modem. Valid values are 2400 and 4800.
    \param tep TRUE is the optional TEP tone is to be transmitted.
    \return 0 for OK, -1 for bad parameter */
int v27ter_tx_restart(v27ter_tx_state_t *s, int rate, int tep);

/*! Release a V.27ter modem transmit context.
    \brief Release a V.27ter modem transmit context.
    \param s The modem context.
    \return 0 for OK */
int v27ter_tx_release(v27ter_tx_state_t *s);

/*! Change the get_bit function associated with a V.27ter modem transmit context.
    \brief Change the get_bit function associated with a V.27ter modem transmit context.
    \param s The modem context.
    \param get_bit The callback routine used to get the data to be transmitted.
    \param user_data An opaque pointer. */
void v27ter_tx_set_get_bit(v27ter_tx_state_t *s, get_bit_func_t get_bit, void *user_data);

/*! Generate a block of V.27ter modem audio samples.
    \brief Generate a block of V.27ter modem audio samples.
    \param s The modem context.
    \param amp The audio sample buffer.
    \param len The number of samples to be generated.
    \return The number of samples actually generated.
*/
int v27ter_tx(v27ter_tx_state_t *s, int16_t amp[], int len);

#ifdef __cplusplus
}
#endif

#endif
/*- End of file ------------------------------------------------------------*/
