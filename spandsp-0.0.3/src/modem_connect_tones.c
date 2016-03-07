/*
 * SpanDSP - a series of DSP components for telephony
 *
 * modem_connect_tones.c - Generation and detection of tones
 * associated with modems calling and answering calls.
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
 * $Id: modem_connect_tones.c,v 1.6 2006/11/30 15:41:47 steveu Exp $
 */
 
/*! \file */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <inttypes.h>
#include <stdlib.h>
#if defined(HAVE_TGMATH_H)
#include <tgmath.h>
#endif
#if defined(HAVE_MATH_H)
#include <math.h>
#endif

#include "spandsp/telephony.h"
#include "spandsp/dds.h"
#include "spandsp/tone_detect.h"
#include "spandsp/tone_generate.h"
#include "spandsp/super_tone_rx.h"
#include "spandsp/modem_connect_tones.h"

int modem_connect_tones_tx(modem_connect_tones_tx_state_t *s,
                           int16_t amp[],
                           int len)
{
    int mod;
    int i;

    switch (s->tone_type)
    {
    case MODEM_CONNECT_TONES_FAX_CNG:
    case MODEM_CONNECT_TONES_FAX_CED:
        len = tone_gen(&s->tone_tx, amp, len);
        break;
    case MODEM_CONNECT_TONES_EC_DISABLE:
        for (i = 0;  i < len;  i++)
        {
            if (--s->hop_timer <= 0)
            {
                s->hop_timer = ms_to_samples(450);
                s->tone_phase += 0x80000000;
            }
            amp[i] = dds_mod(&s->tone_phase, s->tone_phase_rate, s->level, 0);
        }
        break;
    case MODEM_CONNECT_TONES_EC_DISABLE_MOD:
        for (i = 0;  i < len;  i++)
        {
            mod = s->level + dds_mod(&s->mod_phase, s->mod_phase_rate, s->mod_level, 0);
            if (--s->hop_timer <= 0)
            {
                s->hop_timer = ms_to_samples(450);
                s->tone_phase += 0x80000000;
            }
            amp[i] = dds_mod(&s->tone_phase, s->tone_phase_rate, mod, 0);
        }
        break;
    }
    return len;
}
/*- End of function --------------------------------------------------------*/

modem_connect_tones_tx_state_t *modem_connect_tones_tx_init(modem_connect_tones_tx_state_t *s,
                                                            int tone_type)
{
    tone_gen_descriptor_t tone_desc;

    s->tone_type = tone_type;
    switch (s->tone_type)
    {
    case MODEM_CONNECT_TONES_FAX_CNG:
        make_tone_gen_descriptor(&tone_desc,
                                 1100,
                                 -11,
                                 0,
                                 0,
                                 500,
                                 3000,
                                 0,
                                 0,
                                 TRUE);
        tone_gen_init(&s->tone_tx, &tone_desc);
        break;
    case MODEM_CONNECT_TONES_FAX_CED:
        make_tone_gen_descriptor(&tone_desc,
                                 2100,
                                 -11,
                                 0,
                                 0,
                                 2600,
                                 0,
                                 0,
                                 0,
                                 FALSE);
        tone_gen_init(&s->tone_tx, &tone_desc);
        break;
    case MODEM_CONNECT_TONES_EC_DISABLE:
    case MODEM_CONNECT_TONES_EC_DISABLE_MOD:
        s->tone_phase_rate = dds_phase_rate(2100.0);
        s->mod_phase_rate = dds_phase_rate(15.0);
        s->tone_phase = 0;
        s->mod_phase = 0;
        s->hop_timer = ms_to_samples(450);
        s->level = dds_scaling_dbm0(-12);
        if (s->tone_type == MODEM_CONNECT_TONES_EC_DISABLE_MOD)
            s->mod_level = s->level/5;
        else
            s->mod_level = 0;
    }
    return s;
}
/*- End of function --------------------------------------------------------*/

int modem_connect_tones_rx(modem_connect_tones_rx_state_t *s, const int16_t amp[], int len)
{
    int i;
    int16_t notched;
    float v1;
    float famp;
    
    switch (s->tone_type)
    {
    case MODEM_CONNECT_TONES_FAX_CNG:
        for (i = 0;  i < len;  i++)
        {
            /* A Cauer notch at 1100Hz, spread just wide enough to meet our detection bandwidth
               criteria. */
            famp = amp[i];
            v1 = 0.792928f*famp + 1.0018744927985f*s->z1 - 0.54196833412465f*s->z2;
            famp = v1 - 1.2994747954630f*s->z1 + s->z2;
            s->z2 = s->z1;
            s->z1 = v1;
            notched = rintf(famp);

            /* Estimate the overall energy in the channel, and the energy in
               the notch (i.e. overall channel energy - tone energy => noise).
               Use abs instead of multiply for speed (is it really faster?). */
            s->channel_level += ((abs(amp[i]) - s->channel_level) >> 5);
            s->notch_level += ((abs(notched) - s->notch_level) >> 5);
            if (s->channel_level > 70  &&  s->notch_level*6 < s->channel_level)
            {
                /* There is adequate energy in the channel, and it is mostly at 1100Hz. */
                if (!s->tone_present)
                {
                    if (++s->tone_cycle_duration >= ms_to_samples(415))
                    {
                        if (s->tone_callback)
                            s->tone_callback(s->callback_data, TRUE);
                        else
                            s->hit = TRUE;
                        s->tone_present = TRUE;
                    }
                }
            }
            else
            {
                s->tone_cycle_duration = 0;
            }
        }
        break;
    case MODEM_CONNECT_TONES_FAX_CED:
        for (i = 0;  i < len;  i++)
        {
            /* A Cauer notch at 2100Hz, spread just wide enough to meet our detection bandwidth
               criteria. */
            famp = amp[i];
            v1 = 0.76000f*famp - 0.1183852f*s->z1 - 0.5104039f*s->z2;
            famp = v1 + 0.1567596f*s->z1 + s->z2;
            s->z2 = s->z1;
            s->z1 = v1;
            notched = rintf(famp);
            /* Estimate the overall energy in the channel, and the energy in
               the notch (i.e. overall channel energy - tone energy => noise).
               Use abs instead of multiply for speed (is it really faster?). */
            s->channel_level += ((abs(amp[i]) - s->channel_level) >> 5);
            s->notch_level += ((abs(notched) - s->notch_level) >> 5);
            if (s->channel_level > 70  &&  s->notch_level*6 < s->channel_level)
            {
                /* There is adequate energy in the channel, and it is mostly at 2100Hz. */
                if (!s->tone_present)
                {
                    if (++s->tone_cycle_duration >= ms_to_samples(500))
                    {
                        if (s->tone_callback)
                            s->tone_callback(s->callback_data, TRUE);
                        else
                            s->hit = TRUE;
                        s->tone_present = TRUE;
                    }
                }
            }
            else
            {
                s->tone_cycle_duration = 0;
            }
        }
        break;
    case MODEM_CONNECT_TONES_EC_DISABLE:
    case MODEM_CONNECT_TONES_EC_DISABLE_MOD:
        for (i = 0;  i < len;  i++)
        {
            /* A Cauer notch at 2100Hz, spread just wide enough to meet our detection bandwidth
               criteria. */
            /* This is actually centred at 2095Hz, but gets the balance we want, due
               to the asymmetric walls of the notch */
            famp = amp[i];
            v1 = 0.76000f*famp - 0.1183852f*s->z1 - 0.5104039f*s->z2;
            famp = v1 + 0.1567596f*s->z1 + s->z2;
            s->z2 = s->z1;
            s->z1 = v1;
            notched = rintf(famp);
            /* Estimate the overall energy in the channel, and the energy in
               the notch (i.e. overall channel energy - tone energy => noise).
               Use abs instead of multiply for speed (is it really faster?).
               Damp the overall energy a little more for a stable result.
               Damp the notch energy a little less, so we don't damp out the
               blip every time the phase reverses */
            s->channel_level += ((abs(amp[i]) - s->channel_level) >> 5);
            s->notch_level += ((abs(notched) - s->notch_level) >> 4);
            if (s->channel_level > 280)
            {
                /* There is adequate energy in the channel. Is it mostly at 2100Hz? */
                if (s->notch_level*6 < s->channel_level)
                {
                    /* The notch says yes, so we have the tone. */
                    if (!s->tone_present)
                    {
                        /* Do we get a kick every 450+-25ms? */
                        if (s->tone_cycle_duration >= ms_to_samples(425)
                            &&
                            s->tone_cycle_duration <= ms_to_samples(475))
                        {
                            if (++s->good_cycles > 2)
                            {
                                if (s->tone_callback)
                                    s->tone_callback(s->callback_data, TRUE);
                                else
                                    s->hit = TRUE;
                            }
                        }
                        s->tone_cycle_duration = 0;
                        s->tone_present = TRUE;
                    }
                }
                else
                {
                    s->tone_present = FALSE;
                }
                s->tone_cycle_duration++;
            }
            else
            {
                s->tone_present = FALSE;
                s->tone_cycle_duration = 0;
                s->good_cycles = 0;
            }
        }
    }
    return 0;
}
/*- End of function --------------------------------------------------------*/

int modem_connect_tones_rx_get(modem_connect_tones_rx_state_t *s)
{
    int x;
    
    x = s->hit;
    s->hit = FALSE;
    return x;
}
/*- End of function --------------------------------------------------------*/

modem_connect_tones_rx_state_t *modem_connect_tones_rx_init(modem_connect_tones_rx_state_t *s,
                                                            int tone_type,
                                                            tone_report_func_t tone_callback,
                                                            void *user_data)
{
    s->tone_type = tone_type;
    s->channel_level = 0;
    s->notch_level = 0;    
    s->tone_present = FALSE;
    s->tone_cycle_duration = 0;
    s->good_cycles = 0;
    s->hit = FALSE;
    s->tone_callback = tone_callback;
    s->callback_data = user_data;
    s->z1 = 0.0f;
    s->z2 = 0.0f;
    return s;
}
/*- End of function --------------------------------------------------------*/
/*- End of file ------------------------------------------------------------*/
