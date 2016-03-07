/*
 * SpanDSP - a series of DSP components for telephony
 *
 * timing.h - Provide access to the Pentium/Athlon TSC timer register
 *
 * Written by Steve Underwood <steveu@coppice.org>
 *
 * Copyright (C) 2001 Steve Underwood
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
 * $Id: timing.h,v 1.6 2006/10/24 13:45:28 steveu Exp $
 */

#if !defined(_TIMING_H_)
#define _TIMING_H_

#ifdef __cplusplus
extern "C" {
#endif

#if defined(__i386__)
static __inline__ uint64_t rdtscll(void)
{
    uint64_t now;

    __asm__ __volatile__(" rdtsc\n" : "=A" (now));
    return now;
}
/*- End of function --------------------------------------------------------*/
#elif defined(__x86_64__)
static __inline__ uint64_t rdtscll(void)
{
    unsigned int __a;
    unsigned int __d;

    __asm__ __volatile__(" rdtsc\n" : "=a" (__a), "=d" (__d));
    return ((unsigned long) __a) | (((unsigned long) __d) << 32);
}
/*- End of function --------------------------------------------------------*/
#endif

#ifdef __cplusplus
}
#endif

#endif
/*- End of file ------------------------------------------------------------*/
