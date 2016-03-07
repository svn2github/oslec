/*
 * SpanDSP - a series of DSP components for telephony
 *
 * gsm0610.h - GSM 06.10 full rate speech codec.
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
 * $Id: gsm0610.h,v 1.7 2006/10/24 13:45:28 steveu Exp $
 */

#if !defined(_GSM0610_H_)
#define _GSM0610_H_

/*! \page gsm0610_page GSM 06.10 encoding and decoding
\section gsm0610_page_sec_1 What does it do?

The GSM 06.10 module is an version of the widely used GSM FR codec software
available from http://kbs.cs.tu-berlin.de/~jutta/toast.html. This version
was produced since some versions of this codec are not bit exact, or not
very efficient on modern processors. This implementation can use MMX instructions
on Pentium class processors, or alternative methods on other processors. It
passes all the ETSI test vectors. That is, it is a tested bit exact implementation.

This implementation supports encoded data in one of three packing formats:
    - Unpacked, with the 76 parameters of a GSM 06.10 code frame each occupying a
      separate byte. (note that none of the parameters exceed 8 bits).
    - Packed the the 33 byte per frame, used for VoIP, where 4 bits per frame are wasted.
    - Packed in WAV49 format, where 2 frames are packed into 65 bytes.

\section gsm0610_page_sec_2 How does it work?
???.
*/

enum
{
    GSM0610_PACKING_NONE,
    GSM0610_PACKING_WAV49,
    GSM0610_PACKING_VOIP
};

/*!
    GSM 06.10 FR codec unpacked frame.
*/
typedef struct
{
    int16_t LARc[8];
    int16_t Nc[4];
    int16_t bc[4];
    int16_t Mc[4];
    int16_t xmaxc[4];
    int16_t xMc[4][13];
} gsm0610_frame_t;

/*!
    GSM 06.10 FR codec state descriptor. This defines the state of
    a single working instance of the GSM 06.10 FR encoder or decoder.
*/
typedef struct
{
    /*! \brief One of the packing modes */
    int packing;

    int16_t dp0[280];

    int16_t z1;         /* preprocessing,   Offset_com. */
    int32_t L_z2;       /*                  Offset_com. */
    int16_t mp;         /*                  Preemphasis */

    int16_t u[8];       /* short_term_delay_filter */
    int16_t LARpp[2][8];
    int16_t j;

    int16_t nrp;        /* long term synthesis */
    int16_t v[9];       /* short term synthesis */
    int16_t msr;        /* decoder postprocessing */

    int16_t e[50];      /* encoder */
    uint8_t	frame_index;
    uint8_t	frame_chain;
} gsm0610_state_t;

#ifdef __cplusplus
extern "C" {
#endif

/*! Initialise a GSM 06.10 encode or decode context.
    \param s The GSM 06.10 context
    \param packing One of the GSM0610_PACKING_xxx options.
    \return A pointer to the GSM 06.10 context, or NULL for error. */
gsm0610_state_t *gsm0610_init(gsm0610_state_t *s, int packing);

int gsm0610_release(gsm0610_state_t *s);

/*! Encode a buffer of linear PCM data to GSM 06.10.
    \param s The GSM 06.10 context.
    \param ima_data The GSM 06.10 data produced.
    \param amp The audio sample buffer.
    \param len The number of samples in the buffer.
    \return The number of bytes of GSM 06.10 data produced. */
int gsm0610_encode(gsm0610_state_t *s, uint8_t code[], const int16_t amp[], int quant);

/*! Decode a buffer of GSM 06.10 data to linear PCM.
    \param s The GSM 06.10 context.
    \param amp The audio sample buffer.
    \param code The GSM 06.10 data.
    \param quant The number of frames of GSM 06.10 data to be decoded.
    \return The number of samples returned. */
int gsm0610_decode(gsm0610_state_t *s, int16_t amp[], const uint8_t code[], int quant);

int gsm0610_pack_none(uint8_t c[], gsm0610_frame_t *s);

int gsm0610_pack_wav49(uint8_t c[], gsm0610_frame_t *s, int half);

int gsm0610_pack_voip(uint8_t c[], gsm0610_frame_t *s);

int gsm0610_unpack_none(gsm0610_frame_t *s, const uint8_t c[]);

int gsm0610_unpack_wav49(gsm0610_frame_t *s, const uint8_t c[], int half);

int gsm0610_unpack_voip(gsm0610_frame_t *s, const uint8_t c[]);

#ifdef __cplusplus
}
#endif

#endif
/*- End of include ---------------------------------------------------------*/
