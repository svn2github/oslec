/*
 * SpanDSP - a series of DSP components for telephony
 *
 * v27ter_rx.c - ITU V.27ter modem receive part
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
 * $Id: v27ter_rx.c,v 1.75 2006/11/28 16:59:57 steveu Exp $
 */

/*! \file */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <inttypes.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#if defined(HAVE_TGMATH_H)
#include <tgmath.h>
#endif
#if defined(HAVE_MATH_H)
#include <math.h>
#endif

#include "spandsp/telephony.h"
#include "spandsp/logging.h"
#include "spandsp/complex.h"
#include "spandsp/vector_float.h"
#include "spandsp/complex_vector_float.h"
#include "spandsp/async.h"
#include "spandsp/power_meter.h"
#include "spandsp/arctan2.h"
#include "spandsp/dds.h"
#include "spandsp/complex_filters.h"

#include "spandsp/v29rx.h"
#include "spandsp/v27ter_rx.h"

/* V.27ter is a DPSK modem, but this code treats it like QAM. It nails down the
   signal to a static constellation, even though dealing with differences is all
   that is necessary. */

#define CARRIER_NOMINAL_FREQ        1800.0f
#define EQUALIZER_DELTA             0.25f

/* Segments of the training sequence */
/* V.27ter defines a long and a short sequence. FAX doesn't use the
   short sequence, so it is not implemented here. */
#define V27TER_TRAINING_SEG_3_LEN   50
#define V27TER_TRAINING_SEG_5_LEN   1074
#define V27TER_TRAINING_SEG_6_LEN   8

enum
{
    TRAINING_STAGE_NORMAL_OPERATION = 0,
    TRAINING_STAGE_SYMBOL_ACQUISITION,
    TRAINING_STAGE_LOG_PHASE,
    TRAINING_STAGE_WAIT_FOR_HOP,
    TRAINING_STAGE_TRAIN_ON_ABAB,
    TRAINING_STAGE_TEST_ONES,
    TRAINING_STAGE_PARKED
};

static const complexf_t v27ter_constellation[8] =
{
    { 1.414f,  0.0f},       /*   0deg */
    { 1.0f,    1.0f},       /*  45deg */
    { 0.0f,    1.414f},     /*  90deg */
    {-1.0f,    1.0f},       /* 135deg */
    {-1.414f,  0.0f},       /* 180deg */
    {-1.0f,   -1.0f},       /* 225deg */
    { 0.0f,   -1.414f},     /* 270deg */
    { 1.0f,   -1.0f}        /* 315deg */
};

/* Raised root cosine pulse shaping filter set; beta 0.5; sample rate 8000; 8 phase steps;
   baud rate 1600; shifted to centre at 1800Hz; complex. */
#define PULSESHAPER_4800_GAIN           (2.4975f*2.0f)
#define PULSESHAPER_4800_COEFF_SETS     8
static const complexf_t pulseshaper_4800[PULSESHAPER_4800_COEFF_SETS][V27TER_RX_4800_FILTER_STEPS] =
{
    {
        {-0.0050334423f, -0.0025646669f},   /* Filter 0 */
        { 0.0001996320f, -0.0006144041f},
        {-0.0064914716f, -0.0010281481f},
        {-0.0000000000f,  0.0057152766f},
        {-0.0060638961f,  0.0009604268f},
        { 0.0046534477f,  0.0143218395f},
        {-0.0027026909f,  0.0013770898f},
        { 0.0114324470f,  0.0157354133f},
        { 0.0161335660f, -0.0161335660f},
        { 0.0216170550f,  0.0157057098f},
        { 0.0523957134f, -0.1028323775f},
        { 0.1009107956f,  0.0327879051f},
        { 0.0626681574f, -0.3956711736f},
        { 1.0309650898f,  0.0000000000f},
        { 0.1612784723f,  1.0182721987f},
        {-0.3809963452f,  0.1237932168f},
        { 0.0481701579f,  0.0945392579f},
        {-0.0933698449f,  0.0678371631f},
        { 0.0188939989f,  0.0188939989f},
        {-0.0134110893f,  0.0184587808f},
        { 0.0173301130f,  0.0088301336f},
        { 0.0009373415f, -0.0028848406f},
        { 0.0148734735f,  0.0023557268f},
        {-0.0000000000f, -0.0061394833f},
        { 0.0056449120f, -0.0008940662f},
        {-0.0020309798f, -0.0062507130f},
        {-0.0005756104f,  0.0002932882f},
    },
    {
        {-0.0018682578f, -0.0009519249f},   /* Filter 1 */
        {-0.0002684621f,  0.0008262413f},
        {-0.0059141931f, -0.0009367162f},
        {-0.0000000000f,  0.0073941285f},
        {-0.0037772132f,  0.0005982518f},
        { 0.0050394423f,  0.0155098087f},
        { 0.0010806327f, -0.0005506098f},
        { 0.0105277637f,  0.0144902237f},
        { 0.0209691082f, -0.0209691082f},
        { 0.0125153543f,  0.0090929371f},
        { 0.0603186345f, -0.1183819857f},
        { 0.0675630592f,  0.0219525687f},
        { 0.0765237582f, -0.4831519944f},
        { 1.0763458014f,  0.0000000000f},
        { 0.1524445751f,  0.9624971666f},
        {-0.2992580667f,  0.0972348401f},
        { 0.0600222537f,  0.1178003057f},
        {-0.0774892752f,  0.0562992540f},
        { 0.0247376160f,  0.0247376159f},
        {-0.0090916622f,  0.0125135995f},
        { 0.0175076452f,  0.0089205908f},
        { 0.0021568809f, -0.0066381970f},
        { 0.0129897446f,  0.0020573734f},
        {-0.0000000000f, -0.0079766726f},
        { 0.0037729191f, -0.0005975717f},
        {-0.0020837980f, -0.0064132707f},
        {-0.0018682578f,  0.0009519249f},
    },
    {
        {-0.0030355143f, -0.0015466718f},   /* Filter 2 */
        {-0.0007306011f,  0.0022485590f},
        {-0.0049435003f, -0.0007829735f},
        {-0.0000000000f,  0.0087472824f},
        {-0.0011144870f,  0.0001765174f},
        { 0.0051901643f,  0.0159736834f},
        { 0.0049297142f, -0.0025118148f},
        { 0.0088213528f,  0.0121415505f},
        { 0.0251126307f, -0.0251126307f},
        { 0.0011182680f,  0.0008124692f},
        { 0.0667555589f, -0.1310151612f},
        { 0.0256033627f,  0.0083190368f},
        { 0.0905183226f, -0.5715101964f},
        { 1.1095595360f,  0.0000000000f},
        { 0.1420835849f,  0.8970804494f},
        {-0.2215345589f,  0.0719809416f},
        { 0.0679608493f,  0.1333806768f},
        {-0.0606982839f,  0.0440998847f},
        { 0.0284660210f,  0.0284660210f},
        {-0.0047348689f,  0.0065169880f},
        { 0.0165731197f,  0.0084444263f},
        { 0.0032233168f, -0.0099203492f},
        { 0.0105861265f,  0.0016766777f},
        {-0.0000000000f, -0.0092685623f},
        { 0.0018009090f, -0.0002852360f},
        {-0.0020112222f, -0.0061899056f},
        {-0.0030355143f,  0.0015466718f},
    },
    {
        {-0.0040182937f, -0.0020474229f},   /* Filter 3 */
        {-0.0011603659f,  0.0035712391f},
        {-0.0036173562f, -0.0005729329f},
        {-0.0000000000f,  0.0096778115f},
        { 0.0018022529f, -0.0002854488f},
        { 0.0050847711f,  0.0156493164f},
        { 0.0086257291f, -0.0043950285f},
        { 0.0063429899f,  0.0087303766f},
        { 0.0282322904f, -0.0282322904f},
        {-0.0123306868f, -0.0089587683f},
        { 0.0712060603f, -0.1397497620f},
        {-0.0248170325f, -0.0080635427f},
        { 0.1043647251f, -0.6589329411f},
        { 1.1298123598f,  0.0000000000f},
        { 0.1304361227f,  0.8235412673f},
        {-0.1491678531f,  0.0484675735f},
        { 0.0722366382f,  0.1417723850f},
        {-0.0437871917f,  0.0318132570f},
        { 0.0301678844f,  0.0301678844f},
        {-0.0005794433f,  0.0007975353f},
        { 0.0146599874f,  0.0074696367f},
        { 0.0040878789f, -0.0125811975f},
        { 0.0078126085f,  0.0012373956f},
        {-0.0000000000f, -0.0099797659f},
        {-0.0001576582f,  0.0000249706f},
        {-0.0018223262f, -0.0056085432f},
        {-0.0040182937f,  0.0020474229f},
    },
    {
        {-0.0047695783f, -0.0024302215f},   /* Filter 4 */
        {-0.0015320920f,  0.0047152944f},
        {-0.0019955989f, -0.0003160718f},
        {-0.0000000000f,  0.0101070339f},
        { 0.0048302421f, -0.0007650352f},
        { 0.0047152968f,  0.0145121913f},
        { 0.0119428503f, -0.0060851862f},
        { 0.0031686377f,  0.0043612557f},
        { 0.0300119095f, -0.0300119094f},
        {-0.0274628457f, -0.0199529254f},
        { 0.0731841827f, -0.1436320457f},
        {-0.0832936387f, -0.0270637438f},
        { 0.1177684882f, -0.7435609707f},
        { 1.1366178989f,  0.0000000000f},
        { 0.1177684882f,  0.7435609707f},
        {-0.0832936387f,  0.0270637438f},
        { 0.0731841827f,  0.1436320457f},
        {-0.0274628457f,  0.0199529254f},
        { 0.0300119095f,  0.0300119094f},
        { 0.0031686377f, -0.0043612557f},
        { 0.0119428503f,  0.0060851862f},
        { 0.0047152968f, -0.0145121913f},
        { 0.0048302421f,  0.0007650352f},
        {-0.0000000000f, -0.0101070339f},
        {-0.0019955989f,  0.0003160718f},
        {-0.0015320920f, -0.0047152944f},
        {-0.0047695783f,  0.0024302215f},
    },
    {
        {-0.0052564711f, -0.0026783058f},   /* Filter 5 */
        {-0.0018223262f,  0.0056085432f},
        {-0.0001576582f, -0.0000249706f},
        {-0.0000000000f,  0.0099797659f},
        { 0.0078126085f, -0.0012373956f},
        { 0.0040878789f,  0.0125811975f},
        { 0.0146599874f, -0.0074696367f},
        {-0.0005794433f, -0.0007975353f},
        { 0.0301678844f, -0.0301678844f},
        {-0.0437871917f, -0.0318132570f},
        { 0.0722366382f, -0.1417723850f},
        {-0.1491678531f, -0.0484675735f},
        { 0.1304361227f, -0.8235412673f},
        { 1.1298123598f,  0.0000000000f},
        { 0.1043647251f,  0.6589329411f},
        {-0.0248170325f,  0.0080635427f},
        { 0.0712060603f,  0.1397497620f},
        {-0.0123306868f,  0.0089587683f},
        { 0.0282322904f,  0.0282322904f},
        { 0.0063429899f, -0.0087303766f},
        { 0.0086257291f,  0.0043950285f},
        { 0.0050847711f, -0.0156493164f},
        { 0.0018022529f,  0.0002854488f},
        {-0.0000000000f, -0.0096778115f},
        {-0.0036173562f,  0.0005729329f},
        {-0.0011603659f, -0.0035712391f},
        {-0.0052564711f,  0.0026783058f},
    },
    {
        {-0.0054614245f, -0.0027827348f},   /* Filter 6 */
        {-0.0020112222f,  0.0061899056f},
        { 0.0018009090f,  0.0002852360f},
        {-0.0000000000f,  0.0092685623f},
        { 0.0105861265f, -0.0016766777f},
        { 0.0032233168f,  0.0099203492f},
        { 0.0165731197f, -0.0084444263f},
        {-0.0047348689f, -0.0065169880f},
        { 0.0284660210f, -0.0284660210f},
        {-0.0606982839f, -0.0440998847f},
        { 0.0679608493f, -0.1333806768f},
        {-0.2215345589f, -0.0719809416f},
        { 0.1420835849f, -0.8970804494f},
        { 1.1095595360f,  0.0000000000f},
        { 0.0905183226f,  0.5715101964f},
        { 0.0256033627f, -0.0083190368f},
        { 0.0667555589f,  0.1310151612f},
        { 0.0011182680f, -0.0008124692f},
        { 0.0251126307f,  0.0251126307f},
        { 0.0088213528f, -0.0121415505f},
        { 0.0049297142f,  0.0025118148f},
        { 0.0051901643f, -0.0159736834f},
        {-0.0011144870f, -0.0001765174f},
        {-0.0000000000f, -0.0087472824f},
        {-0.0049435003f,  0.0007829735f},
        {-0.0007306011f, -0.0022485590f},
        {-0.0054614245f,  0.0027827348f},
    },
    {
        {-0.0053826099f, -0.0027425768f},   /* Filter 7 */
        {-0.0020837980f,  0.0064132707f},
        { 0.0037729191f,  0.0005975717f},
        {-0.0000000000f,  0.0079766726f},
        { 0.0129897446f, -0.0020573734f},
        { 0.0021568809f,  0.0066381970f},
        { 0.0175076452f, -0.0089205908f},
        {-0.0090916622f, -0.0125135995f},
        { 0.0247376160f, -0.0247376159f},
        {-0.0774892752f, -0.0562992540f},
        { 0.0600222537f, -0.1178003057f},
        {-0.2992580667f, -0.0972348401f},
        { 0.1524445751f, -0.9624971666f},
        { 1.0763458014f,  0.0000000000f},
        { 0.0765237582f,  0.4831519944f},
        { 0.0675630592f, -0.0219525687f},
        { 0.0603186345f,  0.1183819857f},
        { 0.0125153543f, -0.0090929371f},
        { 0.0209691082f,  0.0209691082f},
        { 0.0105277637f, -0.0144902237f},
        { 0.0010806327f,  0.0005506098f},
        { 0.0050394423f, -0.0155098087f},
        {-0.0037772132f, -0.0005982518f},
        {-0.0000000000f, -0.0073941285f},
        {-0.0059141931f,  0.0009367162f},
        {-0.0002684621f, -0.0008262413f},
        {-0.0053826099f,  0.0027425768f},
    },
};

/* Raised root cosine pulse shaping filter set; beta 0.5; sample rate 8000; 12 phase steps;
   baud rate 1200; shifted to centre at 1800Hz; complex. */
#define PULSESHAPER_2400_GAIN           2.223f
#define PULSESHAPER_2400_COEFF_SETS     12
static const complexf_t pulseshaper_2400[PULSESHAPER_2400_COEFF_SETS][V27TER_RX_2400_FILTER_STEPS] =
{
    {
        { 0.0036326320f,  0.0018509185f},   /* Filter 0 */
        { 0.0003793370f, -0.0011674794f},
        { 0.0048754563f,  0.0007721964f},
        { 0.0000000000f, -0.0062069190f},
        {-0.0027810383f,  0.0004404732f},
        {-0.0021925965f, -0.0067481182f},
        {-0.0140173459f,  0.0071421944f},
        { 0.0019772880f,  0.0027215034f},
        {-0.0092149554f,  0.0092149553f},
        { 0.0334995425f,  0.0243388423f},
        { 0.0199195813f, -0.0390943796f},
        { 0.1477459776f,  0.0480055782f},
        { 0.0427277333f, -0.2697722907f},
        { 1.0040582418f,  0.0000000000f},
        { 0.1570693140f,  0.9916966187f},
        {-0.2597668560f,  0.0844033680f},
        { 0.0705271128f,  0.1384172525f},
        {-0.0354969538f,  0.0257900466f},
        { 0.0292796738f,  0.0292796738f},
        { 0.0076599673f, -0.0105430406f},
        { 0.0029973132f,  0.0015272073f},
        { 0.0048614662f, -0.0149620544f},
        {-0.0070080354f, -0.0011099638f},
        {-0.0000000000f, -0.0028157043f},
        {-0.0061305015f,  0.0009709761f},
        { 0.0015253788f,  0.0046946332f},
        {-0.0010937644f,  0.0005573008f},
    },
    {
        {-0.0002819961f, -0.0001436842f},   /* Filter 1 */
        { 0.0006588563f, -0.0020277512f},
        { 0.0041797109f,  0.0006620012f},
        { 0.0000000000f, -0.0065623410f},
        {-0.0042368606f,  0.0006710528f},
        {-0.0017245111f, -0.0053074995f},
        {-0.0146686673f,  0.0074740593f},
        { 0.0038644283f,  0.0053189292f},
        {-0.0067358415f,  0.0067358415f},
        { 0.0345347757f,  0.0250909833f},
        { 0.0269170677f, -0.0528277198f},
        { 0.1389398473f,  0.0451442930f},
        { 0.0525256151f, -0.3316336818f},
        { 1.0434222221f,  0.0000000000f},
        { 0.1499906453f,  0.9470036639f},
        {-0.2028542371f,  0.0659113371f},
        { 0.0727579142f,  0.1427954467f},
        {-0.0235379981f,  0.0171013566f},
        { 0.0275384769f,  0.0275384769f},
        { 0.0093041035f, -0.0128059998f},
        { 0.0001136455f,  0.0000579053f},
        { 0.0045116911f, -0.0138855574f},
        {-0.0082179267f, -0.0013015917f},
        {-0.0000000000f, -0.0013177606f},
        {-0.0055834514f,  0.0008843318f},
        { 0.0016945064f,  0.0052151545f},
        {-0.0002819961f,  0.0001436842f},
    },
    {
        { 0.0005112062f,  0.0002604726f},   /* Filter 2 */
        { 0.0009277352f, -0.0028552754f},
        { 0.0033466091f,  0.0005300508f},
        { 0.0000000000f, -0.0067017064f},
        {-0.0056208495f,  0.0008902551f},
        {-0.0011771217f, -0.0036228080f},
        {-0.0149285967f,  0.0076064999f},
        { 0.0056743444f,  0.0078100650f},
        {-0.0038028170f,  0.0038028170f},
        { 0.0344837758f,  0.0250539297f},
        { 0.0340482504f, -0.0668234539f},
        { 0.1257304818f,  0.0408523100f},
        { 0.0626620127f, -0.3956323778f},
        { 1.0763765574f,  0.0000000000f},
        { 0.1420845360f,  0.8970864542f},
        {-0.1491315874f,  0.0484557901f},
        { 0.0731690629f,  0.1436023716f},
        {-0.0123276338f,  0.0089565502f},
        { 0.0250869159f,  0.0250869159f},
        { 0.0105070407f, -0.0144617008f},
        {-0.0027029676f, -0.0013772308f},
        { 0.0040530413f, -0.0124739786f},
        {-0.0091186142f, -0.0014442466f},
        { 0.0000000000f,  0.0001565015f},
        {-0.0048632493f,  0.0007702630f},
        { 0.0018111360f,  0.0055741034f},
        { 0.0005112062f, -0.0002604726f},
    },
    {
        { 0.0012626700f,  0.0006433625f},   /* Filter 3 */
        { 0.0011774450f, -0.0036238031f},
        { 0.0023987660f,  0.0003799272f},
        { 0.0000000000f, -0.0066136620f},
        {-0.0068852097f,  0.0010905101f},
        {-0.0005635325f, -0.0017343746f},
        {-0.0147735044f,  0.0075274764f},
        { 0.0073440948f,  0.0101082792f},
        {-0.0004807357f,  0.0004807357f},
        { 0.0332365955f,  0.0241478001f},
        { 0.0411429005f, -0.0807474888f},
        { 0.1079056364f,  0.0350606666f},
        { 0.0730301872f, -0.4610944549f},
        { 1.1024802923f,  0.0000000000f},
        { 0.1334542238f,  0.8425968074f},
        {-0.0990668329f,  0.0321887653f},
        { 0.0719339457f,  0.1411783175f},
        {-0.0020666801f,  0.0015015310f},
        { 0.0220599213f,  0.0220599213f},
        { 0.0112587393f, -0.0154963252f},
        {-0.0053688554f, -0.0027355684f},
        { 0.0035031104f, -0.0107814651f},
        {-0.0096971580f, -0.0015358789f},
        { 0.0000000000f,  0.0015619067f},
        {-0.0039973434f,  0.0006331170f},
        { 0.0018730190f,  0.0057645597f},
        { 0.0012626700f, -0.0006433625f},
    },
    {
        { 0.0019511001f,  0.0009941351f},   /* Filter 4 */
        { 0.0013998188f, -0.0043081992f},
        { 0.0013630316f,  0.0002158830f},
        { 0.0000000000f, -0.0062936717f},
        {-0.0079839961f,  0.0012645408f},
        { 0.0001005750f,  0.0003095379f},
        {-0.0141915007f,  0.0072309308f},
        { 0.0088117029f,  0.0121282686f},
        { 0.0031486956f, -0.0031486956f},
        { 0.0307069943f,  0.0223099373f},
        { 0.0480159027f, -0.0942365150f},
        { 0.0853178815f,  0.0277214601f},
        { 0.0835162618f, -0.5273009241f},
        { 1.1213819981f,  0.0000000000f},
        { 0.1242110958f,  0.7842379942f},
        {-0.0530547129f,  0.0172385212f},
        { 0.0692420091f,  0.1358950944f},
        { 0.0070828755f, -0.0051460103f},
        { 0.0185973391f,  0.0185973390f},
        { 0.0115630893f, -0.0159152271f},
        {-0.0078088406f, -0.0039788030f},
        { 0.0028814530f, -0.0088682003f},
        {-0.0099506630f, -0.0015760302f},
        { 0.0000000000f,  0.0028570218f},
        {-0.0030168074f,  0.0004778154f},
        { 0.0018796273f,  0.0057848981f},
        { 0.0019511001f, -0.0009941351f},
    },
    {
        { 0.0025576725f,  0.0013031992f},   /* Filter 5 */
        { 0.0015873149f, -0.0048852529f},
        { 0.0002697804f,  0.0000427290f},
        { 0.0000000000f, -0.0057443897f},
        {-0.0088745956f,  0.0014055979f},
        { 0.0007973152f,  0.0024538838f},
        {-0.0131833524f,  0.0067172536f},
        { 0.0100180850f,  0.0137887111f},
        { 0.0069881240f, -0.0069881240f},
        { 0.0268364889f,  0.0194978505f},
        { 0.0544701590f, -0.1069037062f},
        { 0.0578903347f,  0.0188097100f},
        { 0.0940009470f, -0.5934986215f},
        { 1.1328263283f,  0.0000000000f},
        { 0.1144728051f,  0.7227528464f},
        {-0.0114127678f,  0.0037082330f},
        { 0.0652944573f,  0.1281475878f},
        { 0.0149986858f, -0.0108971831f},
        { 0.0148400450f,  0.0148400450f},
        { 0.0114369676f, -0.0157416354f},
        {-0.0099579979f, -0.0050738534f},
        { 0.0022089316f, -0.0067983924f},
        {-0.0098859888f, -0.0015657868f},
        { 0.0000000000f,  0.0040052626f},
        {-0.0019552917f,  0.0003096878f},
        { 0.0018321212f,  0.0056386894f},
        { 0.0025576725f, -0.0013031992f},
    },
    {
        { 0.0030665390f,  0.0015624797f},   /* Filter 6 */
        { 0.0017332683f, -0.0053344513f},
        {-0.0008479269f, -0.0001342984f},
        { 0.0000000000f, -0.0049758209f},
        {-0.0095191676f,  0.0015076880f},
        { 0.0015070535f,  0.0046382337f},
        {-0.0117630486f,  0.0059935726f},
        { 0.0109089996f,  0.0150149499f},
        { 0.0109262557f, -0.0109262557f},
        { 0.0215979519f,  0.0156918306f},
        { 0.0602999226f, -0.1183452615f},
        { 0.0256212387f,  0.0083248451f},
        { 0.1043613218f, -0.6589114533f},
        { 1.1366584301f,  0.0000000000f},
        { 0.1043613218f,  0.6589114533f},
        { 0.0256212387f, -0.0083248451f},
        { 0.0602999226f,  0.1183452615f},
        { 0.0215979519f, -0.0156918306f},
        { 0.0109262557f,  0.0109262557f},
        { 0.0109089996f, -0.0150149499f},
        {-0.0117630486f, -0.0059935726f},
        { 0.0015070535f, -0.0046382337f},
        {-0.0095191676f, -0.0015076880f},
        { 0.0000000000f,  0.0049758209f},
        {-0.0008479269f,  0.0001342984f},
        { 0.0017332683f,  0.0053344513f},
        { 0.0030665390f, -0.0015624797f},
    },
    {
        { 0.0034652296f,  0.0017656227f},   /* Filter 7 */
        { 0.0018321212f, -0.0056386894f},
        {-0.0019552917f, -0.0003096878f},
        { 0.0000000000f, -0.0040052626f},
        {-0.0098859888f,  0.0015657868f},
        { 0.0022089316f,  0.0067983924f},
        {-0.0099579979f,  0.0050738534f},
        { 0.0114369676f,  0.0157416354f},
        { 0.0148400450f, -0.0148400450f},
        { 0.0149986858f,  0.0108971831f},
        { 0.0652944573f, -0.1281475878f},
        {-0.0114127678f, -0.0037082330f},
        { 0.1144728051f, -0.7227528464f},
        { 1.1328263283f,  0.0000000000f},
        { 0.0940009470f,  0.5934986215f},
        { 0.0578903347f, -0.0188097100f},
        { 0.0544701590f,  0.1069037062f},
        { 0.0268364889f, -0.0194978505f},
        { 0.0069881240f,  0.0069881240f},
        { 0.0100180850f, -0.0137887111f},
        {-0.0131833524f, -0.0067172536f},
        { 0.0007973152f, -0.0024538838f},
        {-0.0088745956f, -0.0014055979f},
        { 0.0000000000f,  0.0057443897f},
        { 0.0002697804f, -0.0000427290f},
        { 0.0015873149f,  0.0048852529f},
        { 0.0034652296f, -0.0017656227f},
    },
    {
        { 0.0037449420f,  0.0019081433f},   /* Filter 8 */
        { 0.0018796273f, -0.0057848981f},
        {-0.0030168074f, -0.0004778154f},
        { 0.0000000000f, -0.0028570218f},
        {-0.0099506630f,  0.0015760302f},
        { 0.0028814530f,  0.0088682003f},
        {-0.0078088406f,  0.0039788030f},
        { 0.0115630893f,  0.0159152271f},
        { 0.0185973391f, -0.0185973390f},
        { 0.0070828755f,  0.0051460103f},
        { 0.0692420091f, -0.1358950944f},
        {-0.0530547129f, -0.0172385212f},
        { 0.1242110958f, -0.7842379942f},
        { 1.1213819981f,  0.0000000000f},
        { 0.0835162618f,  0.5273009241f},
        { 0.0853178815f, -0.0277214601f},
        { 0.0480159027f,  0.0942365150f},
        { 0.0307069943f, -0.0223099373f},
        { 0.0031486956f,  0.0031486956f},
        { 0.0088117029f, -0.0121282686f},
        {-0.0141915007f, -0.0072309308f},
        { 0.0001005750f, -0.0003095379f},
        {-0.0079839961f, -0.0012645408f},
        { 0.0000000000f,  0.0062936717f},
        { 0.0013630316f, -0.0002158830f},
        { 0.0013998188f,  0.0043081992f},
        { 0.0037449420f, -0.0019081433f},
    },
    {
        { 0.0039007144f,  0.0019875132f},   /* Filter 9 */
        { 0.0018730190f, -0.0057645597f},
        {-0.0039973434f, -0.0006331170f},
        { 0.0000000000f, -0.0015619067f},
        {-0.0096971580f,  0.0015358789f},
        { 0.0035031104f,  0.0107814651f},
        {-0.0053688554f,  0.0027355684f},
        { 0.0112587393f,  0.0154963252f},
        { 0.0220599213f, -0.0220599213f},
        {-0.0020666801f, -0.0015015310f},
        { 0.0719339457f, -0.1411783175f},
        {-0.0990668329f, -0.0321887653f},
        { 0.1334542238f, -0.8425968074f},
        { 1.1024802923f,  0.0000000000f},
        { 0.0730301872f,  0.4610944549f},
        { 0.1079056364f, -0.0350606666f},
        { 0.0411429005f,  0.0807474888f},
        { 0.0332365955f, -0.0241478001f},
        {-0.0004807357f, -0.0004807357f},
        { 0.0073440948f, -0.0101082792f},
        {-0.0147735044f, -0.0075274764f},
        {-0.0005635325f,  0.0017343746f},
        {-0.0068852097f, -0.0010905101f},
        { 0.0000000000f,  0.0066136620f},
        { 0.0023987660f, -0.0003799272f},
        { 0.0011774450f,  0.0036238031f},
        { 0.0039007144f, -0.0019875132f},
    },
    {
        { 0.0039314768f,  0.0020031875f},   /* Filter 10 */
        { 0.0018111360f, -0.0055741034f},
        {-0.0048632493f, -0.0007702630f},
        { 0.0000000000f, -0.0001565015f},
        {-0.0091186142f,  0.0014442466f},
        { 0.0040530413f,  0.0124739786f},
        {-0.0027029676f,  0.0013772308f},
        { 0.0105070407f,  0.0144617008f},
        { 0.0250869159f, -0.0250869159f},
        {-0.0123276338f, -0.0089565502f},
        { 0.0731690629f, -0.1436023716f},
        {-0.1491315874f, -0.0484557901f},
        { 0.1420845360f, -0.8970864542f},
        { 1.0763765574f,  0.0000000000f},
        { 0.0626620127f,  0.3956323778f},
        { 0.1257304818f, -0.0408523100f},
        { 0.0340482504f,  0.0668234539f},
        { 0.0344837758f, -0.0250539297f},
        {-0.0038028170f, -0.0038028170f},
        { 0.0056743444f, -0.0078100650f},
        {-0.0149285967f, -0.0076064999f},
        {-0.0011771217f,  0.0036228080f},
        {-0.0056208495f, -0.0008902551f},
        { 0.0000000000f,  0.0067017064f},
        { 0.0033466091f, -0.0005300508f},
        { 0.0009277352f,  0.0028552754f},
        { 0.0039314768f, -0.0020031875f},
    },
    {
        { 0.0038399827f,  0.0019565689f},   /* Filter 11 */
        { 0.0016945064f, -0.0052151545f},
        {-0.0055834514f, -0.0008843318f},
        {-0.0000000000f,  0.0013177606f},
        {-0.0082179267f,  0.0013015917f},
        { 0.0045116911f,  0.0138855574f},
        { 0.0001136455f, -0.0000579053f},
        { 0.0093041035f,  0.0128059998f},
        { 0.0275384769f, -0.0275384769f},
        {-0.0235379981f, -0.0171013566f},
        { 0.0727579142f, -0.1427954467f},
        {-0.2028542371f, -0.0659113371f},
        { 0.1499906453f, -0.9470036639f},
        { 1.0434222221f,  0.0000000000f},
        { 0.0525256151f,  0.3316336818f},
        { 0.1389398473f, -0.0451442930f},
        { 0.0269170677f,  0.0528277198f},
        { 0.0345347757f, -0.0250909833f},
        {-0.0067358415f, -0.0067358415f},
        { 0.0038644283f, -0.0053189292f},
        {-0.0146686673f, -0.0074740593f},
        {-0.0017245111f,  0.0053074995f},
        {-0.0042368606f, -0.0006710528f},
        { 0.0000000000f,  0.0065623410f},
        { 0.0041797109f, -0.0006620012f},
        { 0.0006588563f,  0.0020277512f},
        { 0.0038399827f, -0.0019565689f},
    },
};

float v27ter_rx_carrier_frequency(v27ter_rx_state_t *s)
{
    return dds_frequencyf(s->carrier_phase_rate);
}
/*- End of function --------------------------------------------------------*/

float v27ter_rx_symbol_timing_correction(v27ter_rx_state_t *s)
{
    int steps_per_symbol;
    
    steps_per_symbol = (s->bit_rate == 4800)  ?  PULSESHAPER_4800_COEFF_SETS*5  :  PULSESHAPER_2400_COEFF_SETS*20/3;
    return (float) s->total_baud_timing_correction/(float) steps_per_symbol;
}
/*- End of function --------------------------------------------------------*/

float v27ter_rx_signal_power(v27ter_rx_state_t *s)
{
    return power_meter_dbm0(&s->power);
}
/*- End of function --------------------------------------------------------*/

void v27ter_rx_signal_cutoff(v27ter_rx_state_t *s, float cutoff)
{
    /* The 0.4 factor allows for the gain of the DC blocker */
    s->carrier_on_power = (int32_t) (power_meter_level_dbm0(cutoff + 2.5f)*0.4f);
    s->carrier_off_power = (int32_t) (power_meter_level_dbm0(cutoff - 2.5f)*0.4f);
}
/*- End of function --------------------------------------------------------*/

int v27ter_rx_equalizer_state(v27ter_rx_state_t *s, complexf_t **coeffs)
{
    *coeffs = s->eq_coeff;
    return V27TER_EQUALIZER_PRE_LEN + 1 + V27TER_EQUALIZER_POST_LEN;
}
/*- End of function --------------------------------------------------------*/

static void equalizer_save(v27ter_rx_state_t *s)
{
    cvec_copyf(s->eq_coeff_save, s->eq_coeff, V27TER_EQUALIZER_PRE_LEN + 1 + V27TER_EQUALIZER_POST_LEN);
}
/*- End of function --------------------------------------------------------*/

static void equalizer_restore(v27ter_rx_state_t *s)
{
    cvec_copyf(s->eq_coeff, s->eq_coeff_save, V27TER_EQUALIZER_PRE_LEN + 1 + V27TER_EQUALIZER_POST_LEN);
    cvec_zerof(s->eq_buf, V27TER_EQUALIZER_MASK);

    s->eq_put_step = (s->bit_rate == 4800)  ?  PULSESHAPER_4800_COEFF_SETS*5/2  :  PULSESHAPER_2400_COEFF_SETS*20/(3*2);
    s->eq_step = 0;
    s->eq_delta = EQUALIZER_DELTA/(V27TER_EQUALIZER_PRE_LEN + 1 + V27TER_EQUALIZER_POST_LEN);
}
/*- End of function --------------------------------------------------------*/

static void equalizer_reset(v27ter_rx_state_t *s)
{
    /* Start with an equalizer based on everything being perfect */
    cvec_zerof(s->eq_coeff, V27TER_EQUALIZER_PRE_LEN + 1 + V27TER_EQUALIZER_POST_LEN);
    s->eq_coeff[V27TER_EQUALIZER_PRE_LEN] = complex_setf(1.414f, 0.0f);
    cvec_zerof(s->eq_buf, V27TER_EQUALIZER_MASK);

    s->eq_put_step = (s->bit_rate == 4800)  ?  PULSESHAPER_4800_COEFF_SETS*5/2  :  PULSESHAPER_2400_COEFF_SETS*20/(3*2);
    s->eq_step = 0;
    s->eq_delta = EQUALIZER_DELTA/(V27TER_EQUALIZER_PRE_LEN + 1 + V27TER_EQUALIZER_POST_LEN);
}
/*- End of function --------------------------------------------------------*/

static __inline__ complexf_t equalizer_get(v27ter_rx_state_t *s)
{
    int i;
    int p;
    complexf_t z;
    complexf_t z1;

    /* Get the next equalized value. */
    z = complex_setf(0.0, 0.0);
    p = s->eq_step - 1;
    for (i = 0;  i < V27TER_EQUALIZER_PRE_LEN + 1 + V27TER_EQUALIZER_POST_LEN;  i++)
    {
        p = (p - 1) & V27TER_EQUALIZER_MASK;
        z1 = complex_mulf(&s->eq_coeff[i], &s->eq_buf[p]);
        z = complex_addf(&z, &z1);
    }
    return z;
}
/*- End of function --------------------------------------------------------*/

static void tune_equalizer(v27ter_rx_state_t *s, const complexf_t *z, const complexf_t *target)
{
    int i;
    int p;
    complexf_t ez;
    complexf_t z1;

    /* Find the x and y mismatch from the exact constellation position. */
    ez = complex_subf(target, z);
    ez.re *= s->eq_delta;
    ez.im *= s->eq_delta;

    p = s->eq_step - 1;
    for (i = 0;  i < V27TER_EQUALIZER_PRE_LEN + 1 + V27TER_EQUALIZER_POST_LEN;  i++)
    {
        p = (p - 1) & V27TER_EQUALIZER_MASK;
        z1 = complex_conjf(&s->eq_buf[p]);
        z1 = complex_mulf(&ez, &z1);
        s->eq_coeff[i] = complex_addf(&s->eq_coeff[i], &z1);
        /* Leak a little to tame uncontrolled wandering */
        s->eq_coeff[i].re *= 0.9999f;
        s->eq_coeff[i].im *= 0.9999f;
    }
}
/*- End of function --------------------------------------------------------*/

static __inline__ void track_carrier(v27ter_rx_state_t *s, const complexf_t *z, const complexf_t *target)
{
    float error;

    /* For small errors the imaginary part of the difference between the actual and the target
       positions is proportional to the phase error, for any particular target. However, the
       different amplitudes of the various target positions scale things. */
    error = z->im*target->re - z->re*target->im;
    
    s->carrier_phase_rate += (int32_t) (s->carrier_track_i*error);
    s->carrier_phase += (int32_t) (s->carrier_track_p*error);
    //span_log(&s->logging, SPAN_LOG_FLOW, "Im = %15.5f   f = %15.5f\n", error, dds_frequencyf(s->carrier_phase_rate));
}
/*- End of function --------------------------------------------------------*/

static __inline__ int descramble(v27ter_rx_state_t *s, int in_bit)
{
    int out_bit;

    out_bit = (in_bit ^ (s->scramble_reg >> 5) ^ (s->scramble_reg >> 6)) & 1;
    if (s->scrambler_pattern_count >= 33)
    {
        out_bit ^= 1;
        s->scrambler_pattern_count = 0;
    }
    else
    {
        if (s->in_training > TRAINING_STAGE_NORMAL_OPERATION  &&  s->in_training < TRAINING_STAGE_TEST_ONES)
        {
            s->scrambler_pattern_count = 0;
        }
        else
        {
            if ((((s->scramble_reg >> 7) ^ in_bit) & ((s->scramble_reg >> 8) ^ in_bit) & ((s->scramble_reg >> 11) ^ in_bit) & 1))
                s->scrambler_pattern_count = 0;
            else
                s->scrambler_pattern_count++;
        }
    }
    s->scramble_reg <<= 1;
    if (s->in_training > TRAINING_STAGE_NORMAL_OPERATION  &&  s->in_training < TRAINING_STAGE_TEST_ONES)
        s->scramble_reg |= out_bit;
    else
        s->scramble_reg |= in_bit;
    return out_bit;
}
/*- End of function --------------------------------------------------------*/

static __inline__ void put_bit(v27ter_rx_state_t *s, int bit)
{
    int out_bit;

    bit &= 1;

    out_bit = descramble(s, bit);

    /* We need to strip the last part of the training before we let data
       go to the application. */
    if (s->in_training == TRAINING_STAGE_NORMAL_OPERATION)
    {
        s->put_bit(s->user_data, out_bit);
    }
    else
    {
        //span_log(&s->logging, SPAN_LOG_FLOW, "Test bit %d\n", out_bit);
        /* The bits during the final stage of training should be all ones. However,
           buggy modems mean you cannot rely on this. Therefore we don't bother
           testing for ones, but just rely on a constellation mismatch measurement. */
    }
}
/*- End of function --------------------------------------------------------*/

static __inline__ int find_quadrant(const complexf_t *z)
{
    int b1;
    int b2;

    /* Split the space along the two diagonals. */
    b1 = (z->im > z->re);
    b2 = (z->im < -z->re);
    return (b2 << 1) | (b1 ^ b2);
}
/*- End of function --------------------------------------------------------*/

static __inline__ int find_octant(complexf_t *z)
{
    float abs_re;
    float abs_im;
    int b1;
    int b2;
    int bits;

    /* Are we near an axis or a diagonal? */
    abs_re = fabsf(z->re);
    abs_im = fabsf(z->im);
    if (abs_im > abs_re*0.4142136  &&  abs_im < abs_re*2.4142136)
    {
        /* Split the space along the two axes. */
        b1 = (z->re < 0.0);
        b2 = (z->im < 0.0);
        bits = (b2 << 2) | ((b1 ^ b2) << 1) | 1;
    }
    else
    {
        /* Split the space along the two diagonals. */
        b1 = (z->im > z->re);
        b2 = (z->im < -z->re);
        bits = (b2 << 2) | ((b1 ^ b2) << 1);
    }
    return bits;
}
/*- End of function --------------------------------------------------------*/

static void decode_baud(v27ter_rx_state_t *s, complexf_t *z)
{
    static const uint8_t phase_steps_4800[8] =
    {
        4, 0, 2, 6, 7, 3, 1, 5
    };
    static const uint8_t phase_steps_2400[4] =
    {
        0, 2, 3, 1
    };
    int nearest;
    int raw_bits;

    switch (s->bit_rate)
    {
    case 4800:
    default:
        nearest = find_octant(z);
        raw_bits = phase_steps_4800[(nearest - s->constellation_state) & 7];
        put_bit(s, raw_bits);
        put_bit(s, raw_bits >> 1);
        put_bit(s, raw_bits >> 2);
        s->constellation_state = nearest;
        break;
    case 2400:
        nearest = find_quadrant(z);
        raw_bits = phase_steps_2400[(nearest - s->constellation_state) & 3];
        put_bit(s, raw_bits);
        put_bit(s, raw_bits >> 1);
        s->constellation_state = nearest;
        nearest <<= 1;
        break;
    }
    track_carrier(s, z, &v27ter_constellation[nearest]);
    if (--s->eq_skip <= 0)
    {
        /* Once we are in the data the equalization should not need updating.
           However, the line characteristics may slowly drift. We, therefore,
           tune up on the occassional sample, keeping the compute down. */
        s->eq_skip = 100;
        tune_equalizer(s, z, &v27ter_constellation[nearest]);
    }
}
/*- End of function --------------------------------------------------------*/

static __inline__ void process_half_baud(v27ter_rx_state_t *s, const complexf_t *sample)
{
    static const int abab_pos[2] =
    {
        0, 4
    };
    complexf_t z;
    complexf_t zz;
    float p;
    float q;
    int i;
    int j;
    int32_t angle;
    int32_t ang;

    /* Add a sample to the equalizer's circular buffer, but don't calculate anything
       at this time. */
    s->eq_buf[s->eq_step] = *sample;
    s->eq_step = (s->eq_step + 1) & V27TER_EQUALIZER_MASK;
        
    /* On alternate insertions we have a whole baud, and must process it. */
    if ((s->baud_phase ^= 1))
    {
        //span_log(&s->logging, SPAN_LOG_FLOW, "Samp, %f, %f, %f, -1, 0x%X\n", z.re, z.im, sqrt(z.re*z.re + z.im*z.im), s->eq_put_step);
        return;
    }
    //span_log(&s->logging, SPAN_LOG_FLOW, "Samp, %f, %f, %f, 1, 0x%X\n", z.re, z.im, sqrt(z.re*z.re + z.im*z.im), s->eq_put_step);

    /* Perform a Gardner test for baud alignment */
    p = s->eq_buf[(s->eq_step - 3) & V27TER_EQUALIZER_MASK].re
      - s->eq_buf[(s->eq_step - 1) & V27TER_EQUALIZER_MASK].re;
    p *= s->eq_buf[(s->eq_step - 2) & V27TER_EQUALIZER_MASK].re;

    q = s->eq_buf[(s->eq_step - 3) & V27TER_EQUALIZER_MASK].im
      - s->eq_buf[(s->eq_step - 1) & V27TER_EQUALIZER_MASK].im;
    q *= s->eq_buf[(s->eq_step - 2) & V27TER_EQUALIZER_MASK].im;

    s->gardner_integrate += (p + q > 0.0)  ?  s->gardner_step  :  -s->gardner_step;

    if (abs(s->gardner_integrate) >= 256)
    {
        /* This integrate and dump approach avoids rapid changes of the equalizer put step.
           Rapid changes, without hysteresis, are bad. They degrade the equalizer performance
           when the true symbol boundary is close to a sample boundary. */
        //span_log(&s->logging, SPAN_LOG_FLOW, "Hop %d\n", s->gardner_integrate);
        s->eq_put_step += (s->gardner_integrate/256);
        s->total_baud_timing_correction += (s->gardner_integrate/256);
        if (s->qam_report)
            s->qam_report(s->qam_user_data, NULL, NULL, s->gardner_integrate);
        s->gardner_integrate = 0;
    }
    //span_log(&s->logging, SPAN_LOG_FLOW, "Gardner=%10.5f 0x%X\n", p, s->eq_put_step);

    z = equalizer_get(s);

    //span_log(&s->logging, SPAN_LOG_FLOW, "Equalized symbol - %15.5f %15.5f\n", z.re, z.im);
    switch (s->in_training)
    {
    case TRAINING_STAGE_NORMAL_OPERATION:
        decode_baud(s, &z);
        break;
    case TRAINING_STAGE_SYMBOL_ACQUISITION:
        /* Allow time for the Gardner algorithm to settle the baud timing */
        /* Don't start narrowing the bandwidth of the Gardner algorithm too early.
           Some modems are a bit wobbly when they start sending the signal. Also, we start
           this analysis before our filter buffers have completely filled. */
        if (++s->training_count >= 30)
        {
            s->gardner_step = 32;
            s->in_training = TRAINING_STAGE_LOG_PHASE;
            s->angles[0] =
            s->start_angles[0] = arctan2(z.im, z.re);
        }
        break;
    case TRAINING_STAGE_LOG_PHASE:
        /* Record the current alternate phase angle */
        angle = arctan2(z.im, z.re);
        s->angles[1] =
        s->start_angles[1] = angle;
        s->training_count = 1;
        s->in_training = TRAINING_STAGE_WAIT_FOR_HOP;
        break;
    case TRAINING_STAGE_WAIT_FOR_HOP:
        angle = arctan2(z.im, z.re);
        /* Look for the initial ABAB sequence to display a phase reversal, which will
           signal the start of the scrambled ABAB segment */
        ang = angle - s->angles[(s->training_count - 1) & 0xF];
        s->angles[(s->training_count + 1) & 0xF] = angle;
        if ((ang > 0x20000000  ||  ang < -0x20000000)  &&  s->training_count >= 3)
        {
            /* We seem to have a phase reversal */
            /* Slam the carrier frequency into line, based on the total phase drift over the last
               section. Use the shift from the odd bits and the shift from the even bits to get
               better jitter suppression. We need to scale here, or at the maximum specified
               frequency deviation we could overflow, and get a silly answer. */
            /* Step back a few symbols so we don't get ISI distorting things. */
            i = (s->training_count - 8) & ~1;
            /* Avoid the possibility of a divide by zero */
            if (i)
            {
                j = i & 0xF;
                ang = (s->angles[j] - s->start_angles[0])/i
                    + (s->angles[j | 0x1] - s->start_angles[1])/i;
                if (s->bit_rate == 4800)
                    s->carrier_phase_rate += ang/10;
                else
                    s->carrier_phase_rate += 3*(ang/40);
            }
            span_log(&s->logging, SPAN_LOG_FLOW, "Coarse carrier frequency %7.2f (%d)\n", dds_frequencyf(s->carrier_phase_rate), s->training_count);
            /* Check if the carrier frequency is plausible */
            if (s->carrier_phase_rate < dds_phase_ratef(CARRIER_NOMINAL_FREQ - 20.0f)
                ||
                s->carrier_phase_rate > dds_phase_ratef(CARRIER_NOMINAL_FREQ + 20.0f))
            {
               span_log(&s->logging, SPAN_LOG_FLOW, "Training failed (sequence failed)\n");
               /* Park this modem */
               s->in_training = TRAINING_STAGE_PARKED;
               s->put_bit(s->user_data, PUTBIT_TRAINING_FAILED);
               break;
            }

            /* Make a step shift in the phase, to pull it into line. We need to rotate the equalizer
               buffer, as well as the carrier phase, for this to play out nicely. */
            angle += 0x80000000;
            p = angle*2.0f*3.14159f/(65536.0f*65536.0f);
            zz = complex_setf(cosf(p), -sinf(p));
            for (i = 0;  i <= V27TER_EQUALIZER_MASK;  i++)
                s->eq_buf[i] = complex_mulf(&s->eq_buf[i], &zz);
            s->carrier_phase += angle;

            s->gardner_step = 1;
            /* We have just seen the first element of the scrambled sequence so skip it. */
            s->training_bc = 1;
            s->training_bc ^= descramble(s, 1);
            descramble(s, 1);
            descramble(s, 1);
            s->training_count = 1;
            s->in_training = TRAINING_STAGE_TRAIN_ON_ABAB;
        }
        else if (++s->training_count > V27TER_TRAINING_SEG_3_LEN)
        {
            /* This is bogus. There are not this many bits in this section
               of a real training sequence. */
            span_log(&s->logging, SPAN_LOG_FLOW, "Training failed (sequence failed)\n");
            /* Park this modem */
            s->in_training = TRAINING_STAGE_PARKED;
            s->put_bit(s->user_data, PUTBIT_TRAINING_FAILED);
        }
        break;
    case TRAINING_STAGE_TRAIN_ON_ABAB:
        /* Train on the scrambled ABAB section */
        s->training_bc ^= descramble(s, 1);
        descramble(s, 1);
        descramble(s, 1);
        s->constellation_state = abab_pos[s->training_bc];
        track_carrier(s, &z, &v27ter_constellation[s->constellation_state]);
        tune_equalizer(s, &z, &v27ter_constellation[s->constellation_state]);

        if (++s->training_count >= V27TER_TRAINING_SEG_5_LEN)
        {
            s->constellation_state = (s->bit_rate == 4800)  ?  4  :  2;
            s->training_count = 0;
            s->in_training = TRAINING_STAGE_TEST_ONES;
            s->carrier_track_i = 400.0;
            s->carrier_track_p = 1000000.0;
        }
        break;
    case TRAINING_STAGE_TEST_ONES:
        decode_baud(s, &z);
        /* Measure the training error */
        if (s->bit_rate == 4800)
            zz = complex_subf(&z, &v27ter_constellation[s->constellation_state]);
        else
            zz = complex_subf(&z, &v27ter_constellation[s->constellation_state << 1]);
        s->training_error += powerf(&zz);
        if (++s->training_count >= V27TER_TRAINING_SEG_6_LEN)
        {
            if ((s->bit_rate == 4800  &&  s->training_error < 1.0f)
                ||
                (s->bit_rate == 2400  &&  s->training_error < 1.0f))
            {
                /* We are up and running */
                span_log(&s->logging, SPAN_LOG_FLOW, "Training succeeded (constellation mismatch %f)\n", s->training_error);
                s->put_bit(s->user_data, PUTBIT_TRAINING_SUCCEEDED);
                /* Apply some lag to the carrier off condition, to ensure the last few bits get pushed through
                   the processing. */
                s->carrier_present = (s->bit_rate == 4800)  ?  90  :  120;
                s->in_training = TRAINING_STAGE_NORMAL_OPERATION;
                equalizer_save(s);
                s->carrier_phase_rate_save = s->carrier_phase_rate;
                s->agc_scaling_save = s->agc_scaling;
            }
            else
            {
                /* Training has failed */
                span_log(&s->logging, SPAN_LOG_FLOW, "Training failed (constellation mismatch %f)\n", s->training_error);
                /* Park this modem */
                s->in_training = TRAINING_STAGE_PARKED;
                s->put_bit(s->user_data, PUTBIT_TRAINING_FAILED);
            }
        }
        break;
    case TRAINING_STAGE_PARKED:
        /* We failed to train! */
        /* Park here until the carrier drops. */
        break;
    }
    if (s->qam_report)
    {
        s->qam_report(s->qam_user_data,
                      &z,
                      &v27ter_constellation[s->constellation_state],
                      s->constellation_state);
    }
}
/*- End of function --------------------------------------------------------*/

int v27ter_rx(v27ter_rx_state_t *s, const int16_t amp[], int len)
{
    int i;
    int j;
    int step;
    complexf_t z;
    complexf_t zz;
    complexf_t samplex;
    int32_t power;

    if (s->bit_rate == 4800)
    {
        for (i = 0;  i < len;  i++)
        {
            s->rrc_filter[s->rrc_filter_step] =
            s->rrc_filter[s->rrc_filter_step + V27TER_RX_4800_FILTER_STEPS] = amp[i];
            if (++s->rrc_filter_step >= V27TER_RX_4800_FILTER_STEPS)
                s->rrc_filter_step = 0;

            /* There should be no DC in the signal, but sometimes there is.
               We need to measure the power with the DC blocked, but not using
               a slow to respond DC blocker. Use the most elementary HPF. */
            power = power_meter_update(&(s->power), (amp[i] - s->last_sample) >> 1);
            s->last_sample = amp[i];
            //span_log(&s->logging, SPAN_LOG_FLOW, "Power = %f\n", power_meter_dbm0(&(s->power)));
            if (s->carrier_present)
            {
                /* Look for power below turnoff threshold to turn the carrier off */
                if (power < s->carrier_off_power)
                {
                    if (--s->carrier_present <= 0)
                    {
                        /* Count down a short delay, to ensure we push the last
                           few bits through the filters before stopping. */
                        v27ter_rx_restart(s, s->bit_rate, FALSE);
                        s->put_bit(s->user_data, PUTBIT_CARRIER_DOWN);
                        continue;
                    }
                }
            }
            else
            {
                /* Look for power exceeding turnon threshold to turn the carrier on */
                if (power < s->carrier_on_power)
                    continue;
                s->carrier_present = 1;
                s->put_bit(s->user_data, PUTBIT_CARRIER_UP);
            }
            if (s->in_training != TRAINING_STAGE_PARKED)
            {
                /* Only spend effort processing this data if the modem is not
                   parked, after training failure. */
                z = dds_complexf(&(s->carrier_phase), s->carrier_phase_rate);
        
                /* Put things into the equalization buffer at T/2 rate. The Gardner algorithm
                   will fiddle the step to align this with the symbols. */
                if ((s->eq_put_step -= PULSESHAPER_4800_COEFF_SETS) <= 0)
                {
                    if (s->in_training == TRAINING_STAGE_SYMBOL_ACQUISITION)
                    {
                        /* Only AGC during the initial training */
                        s->agc_scaling = (1.0f/PULSESHAPER_4800_GAIN)*1.414f/sqrtf(power);
                    }
                    /* Pulse shape while still at the carrier frequency, using a quadrature
                       pair of filters. This results in a properly bandpass filtered complex
                       signal, which can be brought directly to bandband by complex mixing.
                       No further filtering, to remove mixer harmonics, is needed. */
                    step = -s->eq_put_step;
                    if (step > PULSESHAPER_4800_COEFF_SETS - 1)
                        step = PULSESHAPER_4800_COEFF_SETS - 1;
                    s->eq_put_step += PULSESHAPER_4800_COEFF_SETS*5/2;
                    zz.re = pulseshaper_4800[step][0].re*s->rrc_filter[s->rrc_filter_step];
                    zz.im = pulseshaper_4800[step][0].im*s->rrc_filter[s->rrc_filter_step];
                    for (j = 1;  j < V27TER_RX_4800_FILTER_STEPS;  j++)
                    {
                        zz.re += pulseshaper_4800[step][j].re*s->rrc_filter[j + s->rrc_filter_step];
                        zz.im += pulseshaper_4800[step][j].im*s->rrc_filter[j + s->rrc_filter_step];
                    }
                    samplex.re = zz.re*s->agc_scaling;
                    samplex.im = zz.im*s->agc_scaling;
                    /* Shift to baseband - since this is done in a full complex form, the
                       result is clean, and requires no further filtering, apart from the
                       equalizer. */
                    zz.re = samplex.re*z.re - samplex.im*z.im;
                    zz.im = -samplex.re*z.im - samplex.im*z.re;
                    process_half_baud(s, &zz);
                }
            }
        }
    }
    else
    {
        for (i = 0;  i < len;  i++)
        {
            s->rrc_filter[s->rrc_filter_step] =
            s->rrc_filter[s->rrc_filter_step + V27TER_RX_2400_FILTER_STEPS] = amp[i];
            if (++s->rrc_filter_step >= V27TER_RX_2400_FILTER_STEPS)
                s->rrc_filter_step = 0;

            /* There should be no DC in the signal, but sometimes there is.
               We need to measure the power with the DC blocked, but not using
               a slow to respond DC blocker. Use the most elementary HPF. */
            power = power_meter_update(&(s->power), (amp[i] - s->last_sample) >> 1);
            s->last_sample = amp[i];
            //span_log(&s->logging, SPAN_LOG_FLOW, "Power = %f\n", power_meter_dbm0(&(s->power)));
            if (s->carrier_present)
            {
                /* Look for power below turnoff threshold to turn the carrier off */
                if (power < s->carrier_off_power)
                {
                    if (--s->carrier_present <= 0)
                    {
                        /* Count down a short delay, to ensure we push the last
                           few bits through the filters before stopping. */
                        v27ter_rx_restart(s, s->bit_rate, FALSE);
                        s->put_bit(s->user_data, PUTBIT_CARRIER_DOWN);
                        continue;
                    }
                }
            }
            else
            {
                /* Look for power exceeding turnon threshold to turn the carrier on */
                if (power < s->carrier_on_power)
                    continue;
                s->carrier_present = 1;
                s->put_bit(s->user_data, PUTBIT_CARRIER_UP);
            }
            if (s->in_training != TRAINING_STAGE_PARKED)
            {
                /* Only spend effort processing this data if the modem is not
                   parked, after training failure. */
                z = dds_complexf(&(s->carrier_phase), s->carrier_phase_rate);
        
                /* Put things into the equalization buffer at T/2 rate. The Gardner algorithm
                   will fiddle the step to align this with the symbols. */
                if ((s->eq_put_step -= PULSESHAPER_2400_COEFF_SETS) <= 0)
                {
                    if (s->in_training == TRAINING_STAGE_SYMBOL_ACQUISITION)
                    {
                        /* Only AGC during the initial training */
                        s->agc_scaling = (1.0f/PULSESHAPER_2400_GAIN)*1.414f/sqrtf(power);
                    }
                    /* Pulse shape while still at the carrier frequency, using a quadrature
                       pair of filters. This results in a properly bandpass filtered complex
                       signal, which can be brought directly to bandband by complex mixing.
                       No further filtering, to remove mixer harmonics, is needed. */
                    step = -s->eq_put_step;
                    if (step > PULSESHAPER_2400_COEFF_SETS - 1)
                        step = PULSESHAPER_2400_COEFF_SETS - 1;
                    s->eq_put_step += PULSESHAPER_2400_COEFF_SETS*20/(3*2);
                    zz.re = pulseshaper_2400[step][0].re*s->rrc_filter[s->rrc_filter_step];
                    zz.im = pulseshaper_2400[step][0].im*s->rrc_filter[s->rrc_filter_step];
                    for (j = 1;  j < V27TER_RX_2400_FILTER_STEPS;  j++)
                    {
                        zz.re += pulseshaper_2400[step][j].re*s->rrc_filter[j + s->rrc_filter_step];
                        zz.im += pulseshaper_2400[step][j].im*s->rrc_filter[j + s->rrc_filter_step];
                    }
                    samplex.re = zz.re*s->agc_scaling;
                    samplex.im = zz.im*s->agc_scaling;
                    /* Shift to baseband - since this is done in a full complex form, the
                       result is clean, and requires no further filtering apart from the
                       equalizer. */
                    zz.re = samplex.re*z.re - samplex.im*z.im;
                    zz.im = -samplex.re*z.im - samplex.im*z.re;
                    process_half_baud(s, &zz);
                }
            }
        }
    }
    return 0;
}
/*- End of function --------------------------------------------------------*/

void v27ter_rx_set_put_bit(v27ter_rx_state_t *s, put_bit_func_t put_bit, void *user_data)
{
    s->put_bit = put_bit;
    s->user_data = user_data;
}
/*- End of function --------------------------------------------------------*/

int v27ter_rx_restart(v27ter_rx_state_t *s, int rate, int old_train)
{
    span_log(&s->logging, SPAN_LOG_FLOW, "Restarting V.27ter\n");
    if (rate != 4800  &&  rate != 2400)
        return -1;
    s->bit_rate = rate;

    vec_zerof(s->rrc_filter, sizeof(s->rrc_filter)/sizeof(s->rrc_filter[0]));
    s->rrc_filter_step = 0;

    s->scramble_reg = 0x3C;
    s->scrambler_pattern_count = 0;
    s->in_training = TRAINING_STAGE_SYMBOL_ACQUISITION;
    s->training_bc = 0;
    s->training_count = 0;
    s->training_error = 0.0f;
    s->carrier_present = 0;

    s->carrier_phase = 0;
    //s->carrier_track_i = 100000.0f;
    //s->carrier_track_p = 20000000.0f;
    s->carrier_track_i = 200000.0f;
    s->carrier_track_p = 10000000.0f;
    power_meter_init(&(s->power), 4);

    s->constellation_state = 0;

    if (s->old_train)
    {
        s->carrier_phase_rate = s->carrier_phase_rate_save;
        s->agc_scaling = s->agc_scaling_save;
        equalizer_restore(s);
    }
    else
    {
        s->carrier_phase_rate = dds_phase_ratef(CARRIER_NOMINAL_FREQ);
        s->agc_scaling = 0.0005f;
        equalizer_reset(s);
    }
    s->eq_skip = 0;
    s->last_sample = 0;

    s->gardner_integrate = 0;
    s->total_baud_timing_correction = 0;
    s->gardner_step = 512;
    s->baud_phase = 0;

    return 0;
}
/*- End of function --------------------------------------------------------*/

v27ter_rx_state_t *v27ter_rx_init(v27ter_rx_state_t *s, int rate, put_bit_func_t put_bit, void *user_data)
{
    if (s == NULL)
    {
        if ((s = (v27ter_rx_state_t *) malloc(sizeof(*s))) == NULL)
            return NULL;
    }
    memset(s, 0, sizeof(*s));
    v27ter_rx_signal_cutoff(s, -45.5f);
    s->put_bit = put_bit;
    s->user_data = user_data;
    span_log_init(&s->logging, SPAN_LOG_NONE, NULL);
    span_log_set_protocol(&s->logging, "V.27ter");

    v27ter_rx_restart(s, rate, FALSE);
    return s;
}
/*- End of function --------------------------------------------------------*/

int v27ter_rx_release(v27ter_rx_state_t *s)
{
    free(s);
    return 0;
}
/*- End of function --------------------------------------------------------*/

void v27ter_rx_set_qam_report_handler(v27ter_rx_state_t *s, qam_report_handler_t *handler, void *user_data)
{
    s->qam_report = handler;
    s->qam_user_data = user_data;
}
/*- End of function --------------------------------------------------------*/
/*- End of file ------------------------------------------------------------*/
