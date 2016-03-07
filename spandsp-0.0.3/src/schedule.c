/*
 * SpanDSP - a series of DSP components for telephony
 *
 * schedule.c
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
 * $Id: schedule.c,v 1.12 2006/10/24 13:45:26 steveu Exp $
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <inttypes.h>
#include <stdlib.h>
#include <memory.h>

#include "spandsp/telephony.h"
#include "spandsp/schedule.h"

int span_schedule_event(span_sched_state_t *s, int ms, span_sched_callback_func_t function, void *user_data)
{
    int i;

    for (i = 0;  i < s->max_to_date;  i++)
    {
        if (s->sched[i].callback == NULL)
            break;
        /*endif*/
    }
    /*endfor*/
    if (i >= s->allocated)
    {
        s->allocated += 5;
        s->sched = (span_sched_t *) realloc(s->sched, sizeof(span_sched_t)*s->allocated);
    }
    /*endif*/
    if (i >= s->max_to_date)
        s->max_to_date = i + 1;
    /*endif*/
    s->sched[i].when = s->ticker + ms*SAMPLE_RATE/1000;
    s->sched[i].callback = function;
    s->sched[i].user_data = user_data;
    return i;
}
/*- End of function --------------------------------------------------------*/

uint64_t span_schedule_next(span_sched_state_t *s)
{
    int i;
    uint64_t earliest;

    earliest = ~((uint64_t) 0);
    for (i = 0;  i < s->max_to_date;  i++)
    {
        if (s->sched[i].callback  &&  earliest > s->sched[i].when)
            earliest = s->sched[i].when;
        /*endif*/
    }
    /*endfor*/
    return earliest;
}
/*- End of function --------------------------------------------------------*/

uint64_t span_schedule_time(span_sched_state_t *s)
{
    return s->ticker;
}
/*- End of function --------------------------------------------------------*/

void span_schedule_update(span_sched_state_t *s, int samples)
{
    int i;
    span_sched_callback_func_t callback;
    void *user_data;

    s->ticker += samples;
    for (i = 0;  i < s->max_to_date;  i++)
    {
        if (s->sched[i].callback  &&  s->sched[i].when <= s->ticker)
        {
            callback = s->sched[i].callback;
            user_data = s->sched[i].user_data;
            s->sched[i].callback = NULL;
            s->sched[i].user_data = NULL;
            callback(s, user_data);
        }
        /*endif*/
    }
    /*endfor*/
}
/*- End of function --------------------------------------------------------*/

void span_schedule_del(span_sched_state_t *s, int i)
{
    if (i >= s->max_to_date
        ||
        i < 0
        ||
        s->sched[i].callback == NULL)
    {
        fprintf(stderr, "Asked to delete scheduled ID %d???\n", i);
        return;
    }
    /*endif*/
    s->sched[i].callback = NULL;
}
/*- End of function --------------------------------------------------------*/

span_sched_state_t *span_schedule_init(span_sched_state_t *s)
{
    memset(s, 0, sizeof(*s));
    return s;
}
/*- End of function --------------------------------------------------------*/

int span_schedule_release(span_sched_state_t *s)
{
    if (s->sched)
    {
        free(s->sched);
        s->sched = NULL;
    }
    return 0;
}
/*- End of function --------------------------------------------------------*/
/*- End of file ------------------------------------------------------------*/
