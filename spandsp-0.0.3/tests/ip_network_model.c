/*
 * SpanDSP - a series of DSP components for telephony
 *
 * ip_network_model.c - Model an IP networks latency, jitter and loss.
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
 * $Id: ip_network_model.c,v 1.4 2006/11/19 14:07:27 steveu Exp $
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <unistd.h>
#include <inttypes.h>
#include <string.h>
#include <time.h>
#include <stdio.h>
#include <fcntl.h>
#include <audiofile.h>
#include <tiffio.h>
#if defined(HAVE_TGMATH_H)
#include <tgmath.h>
#endif
#if defined(HAVE_MATH_H)
#define GEN_CONST
#include <math.h>
#endif

#include "spandsp.h"

#include "ip_network_model.h"

#if !defined(NULL)
#define NULL (void *) 0
#endif

void ip_network_model_send(ip_network_model_state_t *s,
                            int seq_no,
                            int count,
                            const uint8_t *buf,
                            int len)
{
    int delay[2];
    int i;

    for (i = 0;  i < count;  i++)
    {
        if (rand()%1000000 >= s->packet_loss)
        {
            delay[0] = s->current_samples + s->bulk_delay + (rand()%s->jitter);
            delay[1] = seq_no;
            queue_write_msg(&s->packet_queue, (uint8_t *) delay, sizeof(delay));
            queue_write_msg(&s->packet_queue, buf, len);
        }
        else
        {
            fprintf(stderr, "Dropping packet - seq %d\n", seq_no);
        }
    }
}
/*- End of function --------------------------------------------------------*/

int ip_network_model_get(ip_network_model_state_t *s,
                          int time_step,
                          uint8_t *msg,
                          int max_len,
                          int *seq_no)
{
    int len;

    s->current_samples += time_step;

    if (s->delay[0] < 0)
    {
        /* Wait for a new packet */
        if (queue_empty(&s->packet_queue))
            return -1;
        len = queue_read_msg(&s->packet_queue, (uint8_t *) s->delay, sizeof(s->delay));
        if (len != sizeof(s->delay))
            return -1;
    }
    *seq_no = s->delay[1];
    /* Wait for the appropriate time */
    if (s->delay[0] > s->current_samples)
        return -1;
    /* Wait for a message */
    if (queue_empty(&s->packet_queue))
        return -1;
    s->delay[0] = -1;
    s->delay[1] = -1;
    len = queue_read_msg(&s->packet_queue, msg, max_len);
    return len;
}
/*- End of function --------------------------------------------------------*/

ip_network_model_state_t *ip_network_model_init(int bulk_delay,
                                                int jitter,
                                                int loss)
{
    ip_network_model_state_t *s;

    if ((s = (ip_network_model_state_t *) malloc(sizeof(*s))) == NULL)
        return NULL;
    memset(s, 0, sizeof(*s));

    s->bulk_delay = bulk_delay;
    s->jitter = jitter;
    s->packet_loss = loss;

    s->delay[0] = -1;
    s->delay[1] = -1;
    
    if (queue_create(&s->packet_queue, 32768, QUEUE_WRITE_ATOMIC | QUEUE_READ_ATOMIC) < 0)
    {
        free(s);
        return NULL;
    }
    return s;
}
/*- End of function --------------------------------------------------------*/

int ip_network_model_release(ip_network_model_state_t *s)
{
    queue_delete(&s->packet_queue);
    free(s);
    return 0;
}
/*- End of function --------------------------------------------------------*/
/*- End of file ------------------------------------------------------------*/
