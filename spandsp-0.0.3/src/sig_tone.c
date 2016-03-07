/*
 * SpanDSP - a series of DSP components for telephony
 *
 * sig_tone.c - Signalling tone processing for the 2280Hz, 2600Hz and similar
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
 * $Id: sig_tone.c,v 1.15 2006/11/19 14:07:25 steveu Exp $
 */

/*! \file */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdlib.h>
#include <stdio.h>
#include <inttypes.h>
#if defined(HAVE_TGMATH_H)
#include <tgmath.h>
#endif
#if defined(HAVE_MATH_H)
#include <math.h>
#endif
#include <memory.h>
#include <string.h>

#include "spandsp/telephony.h"
#include "spandsp/dc_restore.h"
#include "spandsp/dds.h"
#include "spandsp/sig_tone.h"

#define PI 3.14159265358979323

/* The coefficients for the data notch filter. This filter is also the
   guard filter for tone detection. */

sig_tone_descriptor_t sig_tones[4] =
{
    {
        /* 2280Hz (e.g. AC15, and many other European protocols) */
        {2280,  0},
        {-10, -20},
        400*8,
    
        225*8,
    
        225*8,
        TRUE,
    
        24,
        64,
    
        {  3600,  14397,  32767},
        {     0,  -9425, -28954},
        {     0,  14196,  32767},
        {     0, -17393, -28954},
        12,
/*
    0.1098633,  0.4393615,   1.0,
    0,         -0.2876274,  -0.8836059,
    0,          0.4332275,   1.0,
    0,         -0.5307922,  -0.8836059,
*/
        { 12900, -16384, -16384}, 
        {     0,  -8578, -11796},
        15,

        31744,
        1024,
    
        31744,
        187,
    
        31744,
        187,
    
        -1,
        -32,
    
        57
    },
    {
        /* 2600Hz (e.g. many US protocols) */
        {2600, 0},
        {-8, -8},
        400*8,
    
        225*8,
    
        225*8,
        FALSE,
    
        24,
        64,
    
        {  3539,  29569,  32767},
        {     0, -24010, -28341},
        {     0,  29844,  32767},
        {     0, -31208, -28341},
        12,
/*
    0.1080017,  0.9023743,   1.0,
    0,         -0.7327271,  -0.86489868,
    0,          0.9107666,   1.0,
    0,         -0.9523925,  -0.86489868,
*/
        { 32768,      0,      0},
        {     0,      0,      0},
        15,
    
        31744,
        1024,
    
        31744,
        170,
    
        31744,
        170,
    
        -1,
        -32,
    
        52
    },
    {
        /* 2400Hz / 2600Hz (e.g. SS5 and SS5bis) */
        {2400, 2600},
        {-8, -8},
        400*8,
    
        225*8,
    
        225*8,
        FALSE,
    
        24,
        64,
    
        {  3539,  20349,  32767},
        {     0, -22075, -31856},
        {     0,  20174,  32767},
        {     0, -17832, -31836},
        12,
/*
    0.1080017,  0.6210065,   1.0,
    0,         -0.6736673,  -0.9721678,
    0,          0.6156693,   1.0,
    0,         -0.5441804,  -0.9715460,
*/
    
        { 32768,      0,      0},
        {     0,      0,      0},
        15,
    
        31744,
        1024,
    
        31744,
        170,
    
        31744,
        170,
    
        -1,
        -32,
    
        52
    }
};

int sig_tone_rx(sig_tone_state_t *s, int16_t amp[], int len)
{
    int32_t x;
    int32_t notched_signal;
    int32_t bandpass_signal;
    int i;

    for (i = 0;  i < len;  i++)
    {
        if (s->signaling_state_duration < 0xFFFF)
            s->signaling_state_duration++;
        /*endif*/
        /* The notch filter is two cascaded biquads. */
        notched_signal = amp[i];
        
        notched_signal *= s->desc->notch_a1[0];
        notched_signal += s->notch_z1[1]*s->desc->notch_b1[1];
        notched_signal += s->notch_z1[2]*s->desc->notch_b1[2];
        x = notched_signal;
        notched_signal += s->notch_z1[1]*s->desc->notch_a1[1];
        notched_signal += s->notch_z1[2]*s->desc->notch_a1[2];
        s->notch_z1[2] = s->notch_z1[1];
        s->notch_z1[1] = x >> 15;
        
        notched_signal += s->notch_z2[1]*s->desc->notch_b2[1];
        notched_signal += s->notch_z2[2]*s->desc->notch_b2[2];
        x = notched_signal;
        notched_signal += s->notch_z2[1]*s->desc->notch_a2[1];
        notched_signal += s->notch_z2[2]*s->desc->notch_a2[2];
        s->notch_z2[2] = s->notch_z2[1];
        s->notch_z2[1] = x >> 15;
        
        notched_signal >>= s->desc->notch_postscale;

        /* Modulus and leaky integrate the notched data. The result of
           this isn't used in low tone detect mode, but we must keep notch_zl
           rolling along. */
        s->notch_zl = ((s->notch_zl*s->desc->notch_slugi) >> 15)
                    + ((abs(notched_signal)*s->desc->notch_slugp) >> 15);

        /* Mow the grass to weed out the noise! */
        s->mown_notch = s->notch_zl & s->desc->notch_threshold;

        if (s->tone_present)
        {
            if (s->flat_mode_timeout <= 0)
                s->flat_mode = TRUE;
            else
                s->flat_mode_timeout--;
            /*endif*/
        }
        else
        {
            s->flat_mode_timeout = s->desc->sharp_flat_timeout;
            s->flat_mode = FALSE;
        }
        /*endif*/

        if (s->flat_mode)
        {
            /* Flat mode */
    
            /* The bandpass filter is a single bi-quad stage */
            bandpass_signal = amp[i];
            bandpass_signal *= s->desc->broad_a[0];
            bandpass_signal += s->broad_z[1]*s->desc->broad_b[1];
            bandpass_signal += s->broad_z[2]*s->desc->broad_b[2];
            x = bandpass_signal;
            bandpass_signal += s->broad_z[1]*s->desc->broad_a[1];
            bandpass_signal += s->broad_z[2]*s->desc->broad_a[2];
            s->broad_z[2] = s->broad_z[1];
            s->broad_z[1] = x >> 15;
            bandpass_signal >>= s->desc->broad_postscale;
    
            /* Leaky integrate the bandpassed data */
            s->broad_zl = ((s->broad_zl*s->desc->broad_slugi) >> 15)
                        + ((abs(bandpass_signal)*s->desc->broad_slugp) >> 15);
    
            /* For the broad band receiver we use a simple linear threshold! */
            if (s->tone_present)
            {
                s->tone_present = (s->broad_zl > s->desc->broad_threshold);
                if (!s->tone_present)
                {
                    if (s->sig_update)
                        s->sig_update(s->user_data, SIG_TONE_1_CHANGE | (s->signaling_state_duration << 16));
                    /*endif*/
                    s->signaling_state_duration = 0;
                }
                /*endif*/
            }
            else
            {
                s->tone_present = (s->broad_zl > s->desc->broad_threshold);
                if (s->tone_present)
                {
                    if (s->sig_update)
                        s->sig_update(s->user_data, SIG_TONE_1_CHANGE | SIG_TONE_1_PRESENT | (s->signaling_state_duration << 16));
                    /*endif*/
                    s->signaling_state_duration = 0;
                }
                /*endif*/
            }
            /*endif*/
            /* Notch insertion logic */
    
            /* tone_present and tone_on are equivalent in flat mode */
            if (s->tone_present)
            {
                s->notch_enabled = s->desc->notch_allowed;
                s->notch_insertion_timeout = s->desc->notch_lag_time;
            }
            else
            {
                if (s->notch_insertion_timeout > 0)
                    s->notch_insertion_timeout--;
                else
                    s->notch_enabled = FALSE;
                /*endif*/
            }
            /*endif*/
        }
        else
        {
            /* Sharp mode */

            /* Modulus and leaky integrate the data */
            s->broad_zl = ((s->broad_zl*s->desc->unfiltered_slugi) >> 15)
                        + ((abs(amp[i])*s->desc->unfiltered_slugp) >> 15);
     
            /* Mow the grass to weed out the noise! */
            s->mown_bandpass = s->broad_zl & s->desc->unfiltered_threshold;
    
            /* Persistence checking and notch insertion logic */
            if (!s->tone_present)
            {
                if (s->mown_notch < s->mown_bandpass)
                {
                    /* Tone is detected this sample */
                    if (s->tone_persistence_timeout <= 0)
                    {
                        s->tone_present = TRUE;
                        s->notch_enabled = s->desc->notch_allowed;
                        s->tone_persistence_timeout = s->desc->tone_off_check_time;
                        s->notch_insertion_timeout = s->desc->notch_lag_time;
                        if (s->sig_update)
                            s->sig_update(s->user_data, SIG_TONE_1_CHANGE | SIG_TONE_1_PRESENT | (s->signaling_state_duration << 16));
                        /*endif*/
                        s->signaling_state_duration = 0;
                    }
                    else
                    {
                        s->tone_persistence_timeout--;
                        if (s->notch_insertion_timeout > 0)
                            s->notch_insertion_timeout--;
                        else
                            s->notch_enabled = FALSE;
                        /*endif*/
                    }
                    /*endif*/
                }
                else
                {
                    s->tone_persistence_timeout = s->desc->tone_on_check_time;
                    if (s->notch_insertion_timeout > 0)
                        s->notch_insertion_timeout--;
                    else
                        s->notch_enabled = FALSE;
                    /*endif*/
                }
                /*endif*/
            }
            else
            {
                if (s->mown_notch > s->mown_bandpass)
                {
                    /* Tone is not detected this sample */
                    if (s->tone_persistence_timeout <= 0)
                    {
                        s->tone_present = FALSE;
                        s->tone_persistence_timeout = s->desc->tone_on_check_time;
                        if (s->sig_update)
                            s->sig_update(s->user_data, SIG_TONE_1_CHANGE | (s->signaling_state_duration << 16));
                        /*endif*/
                        s->signaling_state_duration = 0;
                    }
                    else
                    {
                        s->tone_persistence_timeout--;
                    }
                    /*endif*/
                }
                else
                {
                    s->tone_persistence_timeout = s->desc->tone_off_check_time;
                }
                /*endif*/
            }
            /*endif*/
        }
        /*endif*/

        if ((s->current_tx_tone & SIG_TONE_RX_PASSTHROUGH))
        {
            //if (s->notch_enabled)
                amp[i] = (int16_t) notched_signal;
            /*endif*/
        }
        else
        {
            amp[i] = 0;
        }
        /*endif*/
    }
    /*endfor*/
    return  len;
}
/*- End of function --------------------------------------------------------*/

int sig_tone_tx(sig_tone_state_t *s, int16_t amp[], int len)
{
    int i;
    int16_t tone;
    int update;
    int high_low;

    if (s->current_tx_tone & SIG_TONE_1_PRESENT)
    {
        for (i = 0;  i < len;  i++)
        {
            if (s->high_low_timer > 0  &&  --s->high_low_timer > 0)
                high_low = 1;
            else
                high_low = 0;
            /*endif*/
            tone = dds_mod(&(s->phase_acc[0]), s->phase_rate[0], s->tone_scaling[high_low], 0);
            if (s->current_tx_tone & SIG_TONE_TX_PASSTHROUGH)
                amp[i] = saturate(amp[i] + tone);
            else
                amp[i] = tone;
            /*endif*/
            if (--s->current_tx_timeout <= 0)
            {
                if (s->sig_update)
                {
                    update = s->sig_update(s->user_data, SIG_TONE_UPDATE_REQUEST);
                    if ((update & 0x03) == 0x03  &&  !(s->current_tx_tone & SIG_TONE_1_PRESENT))
                        s->high_low_timer = s->desc->high_low_timeout;
                    /*endif*/
                    s->current_tx_tone = update & 0xFFFF;
                    s->current_tx_timeout = (update >> 16) & 0xFFFF;
                }
                /*endif*/
            }
            /*endif*/
        }
        /*endfor*/
    }
    else
    {
        for (i = 0;  i < len;  )
        {
            if (s->current_tx_timeout < len - i)
            {
                if (!(s->current_tx_tone & SIG_TONE_TX_PASSTHROUGH))
                {
                    /* Zap any audio in the buffer */
                    memset(amp + i, 0, sizeof(int16_t)*s->current_tx_timeout);
                }
                /*endif*/
                i += s->current_tx_timeout;
                if (s->sig_update)
                {
                    update = s->sig_update(s->user_data, SIG_TONE_UPDATE_REQUEST);
                    if ((update & 0x03) == 0x03)
                        s->high_low_timer = s->desc->high_low_timeout;
                    /*endif*/
                    s->current_tx_tone = update & 0xFFFF;
                    s->current_tx_timeout = (update >> 16) & 0xFFFF;
                }
                /*endif*/
            }
            else
            {
                s->current_tx_timeout -= (len - i);
                if (!(s->current_tx_tone & SIG_TONE_TX_PASSTHROUGH))
                {
                    /* Zap any audio in the buffer */
                    memset(amp + i, 0, sizeof(int16_t)*(len - i));
                    i = len;
                }
                /*endif*/
            }
            /*endif*/
        }
        /*endfor*/
    }
    /*endif*/
    return len;
}
/*- End of function --------------------------------------------------------*/

sig_tone_state_t *sig_tone_init(sig_tone_state_t *s, int tone_type, sig_tone_func_t sig_update, void *user_data)
{
    if (tone_type <= 0  ||  tone_type > 3)
        return NULL;
    /*endif*/

    memset(s, 0, sizeof(*s));

    s->sig_update = sig_update;
    s->user_data = user_data;

    s->desc = &sig_tones[tone_type - 1];

    /* Set up the transmit side */
    s->phase_rate[0] = dds_phase_rate((float) s->desc->tone_freq[0]);
    if (s->desc->tone_freq[1])
        s->phase_rate[1] = dds_phase_rate((float) s->desc->tone_freq[1]);
    else
        s->phase_rate[1] = 0;
    /*endif*/
    s->tone_scaling[0] = dds_scaling_dbm0((float) s->desc->tone_amp[0]);
    s->tone_scaling[1] = dds_scaling_dbm0((float) s->desc->tone_amp[1]);

    /* Set up the receive side */
    s->flat_mode_timeout = 0;
    s->notch_insertion_timeout = 0;
    s->tone_persistence_timeout = 0;
    s->signaling_state_duration = 0;
    return s;
}
/*- End of function --------------------------------------------------------*/
/*- End of file ------------------------------------------------------------*/
