/*
 * SpanDSP - a series of DSP components for telephony
 *
 * schedule.h
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
 * $Id: schedule.h,v 1.8 2006/10/24 13:45:28 steveu Exp $
 */

/*! \file */

/*! \page schedule_page Scheduling
\section schedule_page_sec_1 What does it do?
???.

\section schedule_page_sec_2 How does it work?
???.
*/

#if !defined(_SCHEDULE_H_)
#define _SCHEDULE_H_

typedef struct span_sched_state_s span_sched_state_t;

typedef void (*span_sched_callback_func_t)(span_sched_state_t *s, void *user_data);

typedef struct
{
    uint64_t when;
    span_sched_callback_func_t callback;
    void *user_data;
} span_sched_t;

struct span_sched_state_s
{
    uint64_t ticker;
    int allocated;
    int max_to_date;
    span_sched_t *sched;
};

#ifdef __cplusplus
extern "C" {
#endif

uint64_t span_schedule_next(span_sched_state_t *s);
uint64_t span_schedule_time(span_sched_state_t *s);

int span_schedule_event(span_sched_state_t *s, int ms, void (*function)(span_sched_state_t *s, void *data), void *user_data);
void span_schedule_update(span_sched_state_t *s, int samples);
void span_schedule_del(span_sched_state_t *s, int id);

span_sched_state_t *span_schedule_init(span_sched_state_t *s);
int span_schedule_release(span_sched_state_t *s);

#ifdef __cplusplus
}
#endif

#endif
/*- End of file ------------------------------------------------------------*/
