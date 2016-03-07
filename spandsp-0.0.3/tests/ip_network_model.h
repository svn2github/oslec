/*
 * SpanDSP - a series of DSP components for telephony
 *
 * ip_network_model.h - Model an IP networks latency, jitter and loss.
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
 * $Id: ip_network_model.h,v 1.4 2006/10/24 13:45:29 steveu Exp $
 */

/*! \file */

/*! \page ip_network_model_page IP network path model
\section ip_network_model_page_sec_1 What does it do?
The IP network modelling module provides simple modelling of latency, jitter
and packet loss for a IP network media streaming path.

Right now this is extremely crude. It needs to be extended to better represent
the behaviour of UDP packets for RTP, UDPTL, and other streaming purposes.
*/

#if !defined(_IP_NETWORK_MODEL_H_)
#define _IP_NETWORK_MODEL_H_

/*!
    IP network model descriptor. This holds the complete state of
    an IP network path model.
*/
typedef struct
{
    /*! The bulk delay of the path, in samples */
    int bulk_delay;
    /*! Jitter */
    int jitter;
    /*! Packet loss, in parts per million */
    int packet_loss;

    queue_t packet_queue;
    int current_samples;
    int delay[2];
} ip_network_model_state_t;

#ifdef __cplusplus
extern "C" {
#endif

void ip_network_model_send(ip_network_model_state_t *s,
                            int seq_no,
                            int count,
                            const uint8_t *buf,
                            int len);

int ip_network_model_get(ip_network_model_state_t *s,
                         int time_step,
                         uint8_t *msg,
                         int max_len,
                         int *seq_no);

ip_network_model_state_t *ip_network_model_init(int bulk_delay,
                                                int jitter,
                                                int loss);

int ip_network_model_release(ip_network_model_state_t *s);

#ifdef __cplusplus
}
#endif

#endif
/*- End of file ------------------------------------------------------------*/
