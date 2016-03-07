/*
  oslec_test.c
  David Rowe
  22 August 2009

  Test module for Linux kernel version of Oslec.  Checks echo canceller
  output signal is identical (bit exact) to output from reference output
  captured from earlier version of Oslec.  Used to ensure minor changes
  to Kernel version of Oslec do not introduce bugs.
*/

/*
  Copyright (C) 2009 David Rowe
 
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

#include <linux/kernel.h>       
#include <linux/module.h>      
#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/slab.h>
#include <linux/proc_fs.h>
#include <asm/atomic.h>
#include <asm/delay.h>

#include "echo.h"
#include "tx.h"
#include "ec.h"

#include "bit_operations.h"

#define TAPS 128
#define N    8000 /* number of samples */

static int __init init_oslec(void)
{
  int                    i, pass, fail;
    struct oslec_state  *oslec;
    int16_t              rx,clean;

    printk("oslec_test installed\n");
    printk("Testing OSLEC with %d taps (%d ms tail)\n", TAPS, (TAPS*1000)/N);

    /* note NLP not switched on to make output more interesting for bit exact
       testing */

    oslec = oslec_create(TAPS, ECHO_CAN_USE_ADAPTION);
    pass = fail = 0;

    for(i=0; i<N; i++) {

      /* echo is modelled as a simple divide by 4 (12 dB loss) */

      rx = tx[i]/4;

      clean = oslec_update(oslec, tx[i], rx);
      if (clean == ec[i])
	pass++;
      else
	fail++;
    }
    
    if (fail == 0)
      printk("Oslec Unit Test PASSED! pass: %d  fail: %d\n", pass, fail); 
    else
      printk("Oslec Unit Test FAILED! pass: %d  fail: %d\n", pass, fail);

    oslec_free(oslec);

    return 0;
}

static void __exit cleanup_oslec(void)
{
    printk("oslec_test removed\n");
}

MODULE_LICENSE("GPL");
MODULE_AUTHOR("David Rowe");
MODULE_DESCRIPTION("Oslec Unit Test Module");

module_init(init_oslec);
module_exit(cleanup_oslec);

