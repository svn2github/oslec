/*
   speedtest.c
   David Rowe
   Created 27 Feb 2007

   Measures execution speed of oslec in user mode.
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
#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>
#include <time.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <echo.h>

#define TAPS 128
#define N    8000 /* sample rate                            */
#define AMP  1000
#define SECS 10   /* number of simulated seconds to process */

/* constant for isr cycle averaging */

#define LTC   5   

/* number of cycles we are using per call */

long long cycles_last = 0;
long long cycles_worst = 0;
long long cycles_average = 0; 

#ifdef __BLACKFIN__
/* sample cycles register of Blackfin */

static inline unsigned int cycles(void) {
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
#elif defined(__X86__) || defined (__i386) || defined (__x86_64__)
static __inline__ uint64_t cycles() {
  uint64_t x;
  __asm__ volatile ("rdtsc\n\t" : "=A" (x));
  return x;
}
#else
static inline volatile unsigned int cycles(void) {
        /* A dummy implementation for other architectures */
        static unsigned int dummy_cycles = 1;
        return dummy_cycles++;
}
#endif

int main(int argc, char **argv) {
    int                i,j;
    echo_can_state_t  *ec;
    short              tx[N],rx[N],clean;
    struct timeval     tv_before;
    struct timeval     tv_after;
    unsigned long long t_before_ms, t_after_ms;
    unsigned long long before_clocks, after_clocks;
    unsigned long long t_ms;
    unsigned long long start_cycles;
    float              mips_cpu, mips_per_ec;
    FILE               *f;

    printf("\nTesting OSLEC with %d taps (%d ms tail)\n", TAPS, (TAPS*1000)/N);
    for(i=0; i<N; i++) {
	tx[i] = (short)AMP*(float)rand()/RAND_MAX;
	rx[i] = tx[i]/4;
    }

    /* note NLP not switched on to make output more interesting for bit exact
       testing */

    ec = echo_can_create(TAPS,   ECHO_CAN_USE_ADAPTION);
   
    /* dump output of first run for bit exact testing when optimising */

    f = fopen("out.txt","wt");
    assert(f != NULL);
    for(i=0; i<N; i++) {
	clean = echo_can_update(ec, tx[i], rx[i]);
	fprintf(f,"%d\n", clean);
    }
    fclose(f);

    gettimeofday(&tv_before, NULL);
    before_clocks = cycles();
    for(j=0; j<SECS; j++) {
	
 	for(i=0; i<N; i++) {	    
	    start_cycles = cycles();
	    clean = echo_can_update(ec, tx[i], rx[i]);
	    cycles_last = cycles() - start_cycles;
	    cycles_average += (cycles_last - cycles_average +(1<<(LTC-1))) >> LTC;
    
	    if (cycles_last > cycles_worst)
		cycles_worst = cycles_last;    
	}
  
    }
    after_clocks = cycles();
    gettimeofday(&tv_after, NULL);
    t_before_ms = 1000*tv_before.tv_sec + tv_before.tv_usec/1000;
    t_after_ms = 1000*tv_after.tv_sec + tv_after.tv_usec/1000;
    t_ms = t_after_ms - t_before_ms;

    mips_cpu = (after_clocks - before_clocks)/(1E3*t_ms);
    printf("CPU executes %5.2f MIPS\n-------------------------\n\n", mips_cpu);

    printf("Method 1: gettimeofday() at start and end\n");
    printf("  %llu ms for %ds of speech\n", t_ms, SECS);
    mips_per_ec = mips_cpu/((float)SECS*1E3/(float)t_ms);
    printf("  %5.2f MIPS\n", mips_per_ec);
    printf("  %5.2f instances possible at 100%% CPU load\n", 
	   (float)SECS*1E3/(float)t_ms);
    
    printf("Method 2: samples clock cycles at start and end\n");
    printf("  %5.2f MIPS\n", (after_clocks - before_clocks)/(1E6*SECS));
    printf("  %5.2f instances possible at 100%% CPU load\n", 
	   mips_cpu/((after_clocks - before_clocks)/(1E6*SECS)));

    printf("Method 3: samples clock cycles for each call, IIR average\n");
    mips_per_ec = 8*(float)cycles_average/1000;
    printf("  cycles_worst %lld cycles_last %lld cycles_av: %lld"
	   "\n  %5.2f MIPS\n",
	   cycles_worst, cycles_last, cycles_average, mips_per_ec);
    printf("  %5.2f instances possible at 100%% CPU load\n", mips_cpu/mips_per_ec);

    return 0;
}

