/*
 * SpanDSP - a series of DSP components for telephony
 *
 * schedule_tests.c
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
 * $Id: schedule_tests.c,v 1.13 2006/11/19 14:07:27 steveu Exp $
 */

/*! \page schedule_tests_page Event scheduler tests
\section schedule_tests_page_sec_1 What does it do?
???.

\section schedule_tests_page_sec_2 How does it work?
???.
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <inttypes.h>
#include <stdlib.h>
#include <string.h>
#if defined(HAVE_TGMATH_H)
#include <tgmath.h>
#endif
#if defined(HAVE_MATH_H)
#include <math.h>
#endif
#include <tiffio.h>

#include "spandsp.h"

uint64_t when1;
uint64_t when2;

static void callback1(span_sched_state_t *s, void *user_data)
{
    int id;
    uint64_t when;

    when = span_schedule_time(s);
    printf("1: Callback at %f %" PRId64 "\n", (float) when/8000.0, when - when1);
    if ((when - when1))
    {
        printf("Callback occured at the wrong time.\n");
        exit(2);
    }
    id = span_schedule_event(s, 500, callback1, NULL);
    when1 = when + 500*8;
    when = span_schedule_next(s);
    printf("1: Event %d, earliest is %" PRId64 "\n", id, when);
}

static void callback2(span_sched_state_t *s, void *user_data)
{
    int id;
    uint64_t when;

    when = span_schedule_time(s);
    printf("2: Callback at %f %" PRId64 "\n", (float) when/8000.0, when - when2);
    id = span_schedule_event(s, 550, callback2, NULL);
    if ((when - when2) != 80)
    {
        printf("Callback occured at the wrong time.\n");
        exit(2);
    }
    when2 = when + 550*8;
    when = span_schedule_next(s);
    printf("2: Event %d, earliest is %" PRId64 "\n", id, when);
}

int main(int argc, char *argv[])
{
    int i;
    int id1;
    int id2;
    span_sched_state_t sched;
    uint64_t when;
    
    span_schedule_init(&sched);

    id1 = span_schedule_event(&sched, 500, callback1, NULL);
    id2 = span_schedule_event(&sched, 550, callback2, NULL);
    when1 = span_schedule_time(&sched) + 500*8;
    when2 = span_schedule_time(&sched) + 550*8;
    //span_schedule_del(&sched, id);
    
    for (i = 0;  i < SAMPLE_RATE*100;  i += 160)
    {
        span_schedule_update(&sched, 160);
    }
    when = span_schedule_time(&sched);
    if ((when1 - when) < 0  ||  (when1 - when) > 4000  ||  (when2 - when) < 0  ||  (when2 - when) > 4400)
    {
        printf("Callback failed to occur.\n");
        exit(2);
    }
    span_schedule_release(&sched);

    printf("Tests passed.\n");
    return  0;
}
/*- End of function --------------------------------------------------------*/
/*- End of file ------------------------------------------------------------*/
