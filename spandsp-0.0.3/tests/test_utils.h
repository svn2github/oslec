/*
 * SpanDSP - a series of DSP components for telephony
 *
 * test_utils.h - Utility routines for module tests.
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
 * $Id: test_utils.h,v 1.6 2006/10/24 13:45:29 steveu Exp $
 */

/*! \file */

#if !defined(_TEST_UTILS_H_)
#define _TEST_UTILS_H_

enum
{
    MUNGE_CODEC_NONE = 0,
    MUNGE_CODEC_ALAW,
    MUNGE_CODEC_ULAW,
    MUNGE_CODEC_G726_40K,
    MUNGE_CODEC_G726_32K,
    MUNGE_CODEC_G726_24K,
    MUNGE_CODEC_G726_16K,
};

typedef struct codec_munge_state_s codec_munge_state_t;

#ifdef __cplusplus
extern "C" {
#endif

codec_munge_state_t *codec_munge_init(int codec);

void codec_munge(codec_munge_state_t *s, int16_t amp[], int len);

#ifdef __cplusplus
}
#endif

#endif
/*- End of file ------------------------------------------------------------*/
