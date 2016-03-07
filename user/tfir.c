/*
   tfir.c
   David Rowe
   Created 30 May 2007

   Test program for Spandsp fir2 function, used for developing optimised
   Blackfin assembler.
*/

/*
  Copyright (C) 2007 David Rowe
 
  All rights reserved.
 
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License version 2, as
  published by the Free Software Foundation.
 
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

#include "fir.h"

#define TAPS 256
#define N    100

/* C-callable function to return value of CYCLES register */

int cycles() {
  int ret;

   __asm__ __volatile__ 
   (
   "%0 = CYCLES;\n\t"
   : "=&d" (ret)
   : 
   : "R1"
   );

   return ret;
}

static int32_t dot(int16_t *x, int16_t *y, int n) {
    int32_t acc = 0;
    int     i;

    for(i=0; i<n; i++)
	acc += *x++ * *y++;

    return acc;
}

#ifdef FIRST_TRY
static int16_t fir16_asm(fir16_state_t *fir, int16_t sample)
{
    int i;
    int32_t y, y_dot;
    int offset1;
    int offset2;

    fir->history[fir->curr_pos] = sample;

    offset2 = fir->curr_pos;
    offset1 = fir->taps - offset2;
    y = 0;
    for (i = fir->taps - 1;  i >= offset1;  i--)
        y += fir->coeffs[i]*fir->history[i - offset1]; 
    y_dot = 0;
    y_dot += dot_asm((int16_t*)&fir->coeffs[offset1], fir->history, fir->curr_pos);
    printf("offset1 %d offset2: %d y = %d  y_dot = %d\n", offset1, offset2, y, y_dot);
    assert(y_dot == y);

    for (  ;  i >= 0;  i--)
        y += fir->coeffs[i]*fir->history[i + offset2];
    y_dot += dot((int16_t*)fir->coeffs, &fir->history[offset2], offset1);
    assert(y_dot == y);

    if (fir->curr_pos <= 0)
    	fir->curr_pos = fir->taps;
    fir->curr_pos--;
    return (int16_t) (y >> 15);
}
#endif

static int16_t fir16_asm(fir16_state_t *fir, int16_t sample)
{
    int i;
    int32_t y, y_dot;

    fir->history[fir->curr_pos] = sample;
    fir->history[fir->curr_pos + fir->taps] = sample;

    /*
    y = 0;
    for (i=0; i<fir->taps; i++)
        y += fir->coeffs[i]*fir->history[fir->curr_pos+i]; 
    */
    y_dot = dot_asm((int16_t*)fir->coeffs, &fir->history[fir->curr_pos], fir->taps);
    //assert(y_dot == y);
    
    if (fir->curr_pos <= 0)
    	fir->curr_pos = fir->taps;
    fir->curr_pos--;
    return (int16_t) (y_dot >> 15);
}

int main() {
    int16_t       taps[TAPS];
    fir16_state_t fir, fir_asm;
    int16_t       out, out_asm, in;
    int           i, before;

    for(i=0; i<TAPS; i++)
	taps[i] = 0x8000;
    
    fir16_create(&fir, taps, TAPS);
    fir16_create(&fir_asm, taps, TAPS);

    /* first check the results are the same for C and asm */

    for(i=0; i<N; i++) {
	in = i;
	out = fir16(&fir, in);
	out_asm = fir16_asm(&fir_asm, in);
	//printf("[%d] out = %d  out_asm = %d\n", i, out, out_asm);
	assert(out == out_asm);
    }

    printf("OK\n");

    /* now measure the speed */

    before = cycles();
    out = fir16(&fir, in);
    printf("C version: %d cycles\n", cycles() - before);
    before = cycles();
    out_asm = fir16_asm(&fir_asm, in);
    printf("ASM version: %d cycles\n", cycles() - before);

    return 0;
}
