/*
 * SpanDSP - a series of DSP components for telephony
 *
 * arctan2.h - A quick rough approximate arc tan
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
 * $Id: arctan2.h,v 1.7 2006/10/24 13:22:01 steveu Exp $
 */

/*! \file */

#if !defined(_ARCTAN2_H_)
#define _ARCTAN2_H_

/*! \page arctan2_page Fast approximate four quadrant arc-tangent
\section arctan2_page_sec_1 What does it do?
This module provides a fast approximate 4-quadrant arc tangent function,
based on something at dspguru.com. The worst case error is about 4.07 degrees.
This is fine for many "where am I" type evaluations in comms. work.

\section arctan2_page_sec_2 How does it work?
???.
*/

#ifdef __cplusplus
extern "C" {
#endif

/* This returns its answer as a signed 32 bit integer phase value. */
static __inline__ int32_t arctan2(float y, float x)
{
    float abs_y;
    float angle;

    if (x == 0.0  ||  y == 0.0)
        return 0;
    
    abs_y = fabsf(y);

    /* If we are in quadrant II or III, flip things around */
    if (x < 0.0)
        angle = 3.0f - (x + abs_y)/(abs_y - x);
    else
        angle = 1.0f - (x - abs_y)/(abs_y + x);
    angle *= 536870912.0;

    /* If we are in quadrant III or IV, negate to return an
       answer in the range +-pi */
    if (y < 0.0)
        angle = -angle;
    return (int32_t) angle;
}
/*- End of function --------------------------------------------------------*/

#ifdef __cplusplus
}
#endif

#endif
/*- End of file ------------------------------------------------------------*/
