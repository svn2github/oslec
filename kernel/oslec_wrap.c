/*
  oslec_wrap.c
  David Rowe
  7 Feb 2007

  Wrapper for OSLEC to turn it into a kernel module compatable with Zaptel.

  The /proc/oslec interface points to the first echo canceller
  instance created. Zaptel appears to create/destroy e/c on a call by
  call basis, and with the current echo can function interface it is
  difficult to tell which channel is assigned to which e/c.  So to
  simply the /proc interface (at least in this first implementation)
  we limit it to the first echo canceller created.

  So if you only have one call up on a system, /proc/oslec will refer
  to that.  That should be sufficient for debugging the echo canceller
  algorithm, we can extend it to handle multiple simultaneous channels
  later.
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

#include <linux/kernel.h>       
#include <linux/module.h>      
#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/slab.h>
#include <linux/proc_fs.h>
#include <asm/atomic.h>
#include <asm/delay.h>


#define malloc(a) kmalloc((a), GFP_KERNEL)
#define free(a) kfree(a)

#include "oslec.h"
#include <echo.h>

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0)
EXPORT_SYMBOL(oslec_echo_can_create);
EXPORT_SYMBOL(oslec_echo_can_free);
EXPORT_SYMBOL(oslec_echo_can_update);
EXPORT_SYMBOL(oslec_echo_can_traintap);
EXPORT_SYMBOL(oslec_echo_can_identify);
EXPORT_SYMBOL(oslec_hpf_tx);
#endif

/* constants for isr cycle averaging */

#define LTC   5   /* base 2 log of TC */

/* number of cycles we are using per call */

static int cycles_last = 0;
static int cycles_worst = 0;
static int cycles_average = 0; 

#ifdef __BLACKFIN__
/* sample cycles register of Blackfin */

static inline volatile unsigned int cycles(void) {
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
static __inline__ uint64_t cycles(void) {
  uint32_t lo, hi;
  /* We cannot use "=A", since this would use %rax on x86_64 */
  __asm__ __volatile__ ("rdtsc" : "=a" (lo), "=d" (hi));
  return (uint64_t)hi << 32 | lo;
}
#elif (defined(__powerpc__) || defined(__ppc__))
static inline __attribute__((always_inline)) unsigned long long cycles(void) {
        register uint32_t tbu, tbl, tmp;

        __asm__ volatile(
        "0:\n\t"
        "mftbu %0\n\t"          
        "mftb %1\n\t"           
        "mftbu %2\n\t"          
        "cmpw %0, %2\n\t"       
        "bne- 0b"               
        : "=r"(tbu), "=r"(tbl), "=r"(tmp)
        : /* nope */
        : "cc");

        return (((uint64_t)tbu)<<32) + tbl;
} 
#else
static inline volatile unsigned int cycles(void) {
        /* A dummy implementation for other architectures */
        static unsigned int dummy_cycles = 1;
        return dummy_cycles++;
}
#endif

/* vars to help us with /proc interface */

static echo_can_state_t *mon_ec;
static int num_ec;
static int len_ec;
 
/* We need this lock as multiple threads may try to manipulate
   the globals used for diagnostics at the same time.
   
   This lock locks two separate sctivities:
   1/ any access to the global mon_ec pointer and related globals num_ec 
      len_ec.
   2/ Any use of an echo canceller that does not require a channel lock. */

#ifdef DEFINE_SPINLOCK
static DEFINE_SPINLOCK(oslec_lock);
#else
static spinlock_t oslec_lock = SPIN_LOCK_UNLOCKED;
#endif

/* Thread safety issues:

  Due to the design of zaptel an e/c instance may be created and
  destroyed at any time.  So monitoring and controlling a running e/c
  instance through /proc/oslec has some special challenges:

  1/ oslec_echo_can_create() might be interrupted part way through,
     and called again by another thread.

  2/ proc_* might be interrupted and the e/c instance pointed to
     by mon_ec destroyed by another thread.

  3/ Call 1 might be destroyed while Call 2 is still up, mon_ec will
     then point to a destroyed instance.

  4/ What happens if an e/c is destroyed while we are modifying it's
     mode?

  The goal is to allow monitoring and control of at least once
  instance while maintaining stable multithreaded operation.  This is
  tricky (at least for me) given zaptel design and the use of globals
  here to monitor status.

  Thanks Dmitry for helping point these problems out.....
*/

struct echo_can_state *oslec_echo_can_create(int len, int adaption_mode) {
  struct echo_can_state *ec;
  unsigned long flags;


  ec = (struct echo_can_state *)malloc(sizeof(struct echo_can_state));
  ec->ec = (void*)echo_can_create(len,   ECHO_CAN_USE_ADAPTION 
				       | ECHO_CAN_USE_NLP 
				       | ECHO_CAN_USE_CLIP
				       | ECHO_CAN_USE_TX_HPF
				       | ECHO_CAN_USE_RX_HPF);
				   

  spin_lock_irqsave(&oslec_lock, flags);
  num_ec++;

  /* We monitor the first e/c created after mon_ec is set to NULL.  If
     no other calls exist this will be the first call.  If a monitored
     call hangs up, we will monitor the next call created, ignoring
     any other current calls.  Not perfect I know, however this is
     just meant to be a development tool.  Stability is more important
     than comprehensive monitoring abilities.
  */
  if (mon_ec == NULL) {
      mon_ec = (echo_can_state_t*)(ec->ec);
      len_ec = len;
  }

  spin_unlock_irqrestore(&oslec_lock, flags);

  return ec;
}

void oslec_echo_can_free(struct echo_can_state *ec) {
  unsigned long flags;
  spin_lock_irqsave(&oslec_lock, flags);

  /* if this is the e/c being monitored, disable monitoring */

  if (mon_ec == ec->ec)
    mon_ec = NULL;

  echo_can_free((echo_can_state_t*)(ec->ec));
  num_ec--;
  free(ec);

  spin_unlock_irqrestore(&oslec_lock, flags);
}

/* 
   This code in re-entrant, and will run in the context of an ISR.  No
   locking is required for cycles calculation, as only one thread will have
   ec->ec == mon_ec at a given time, so there will only ever be one
   writer to the cycles_* globals. 
*/

short oslec_echo_can_update(struct echo_can_state *ec, short iref, short isig) {
    short clean;
    u32   start_cycles = 0;

    if (ec->ec == mon_ec) {
      start_cycles = cycles();
    }

    clean = echo_can_update((echo_can_state_t*)(ec->ec), iref, isig);

    /*
      Simple IIR averager:

                   -LTC           -LTC
      y(n) = (1 - 2    )y(n-1) + 2    x(n)

    */

    if (ec->ec == mon_ec) {
      cycles_last = cycles() - start_cycles;
      cycles_average += (cycles_last - cycles_average) >> LTC;
    
      if (cycles_last > cycles_worst)
	cycles_worst = cycles_last;
    }

    return clean;
}

int oslec_echo_can_traintap(struct echo_can_state *ec, int pos, short val)
{
	return 1;
}

void oslec_echo_can_identify(char *buf, size_t len)
{
       strncpy(buf, "Oslec", len);
}

static int proc_read_info(char *buf, char **start, off_t offset,
                          int count, int *eof, void *data)
{
  int len;
  char mode_str[80];
  unsigned long flags;

  *eof = 1;

  spin_lock_irqsave(&oslec_lock, flags);

  if (mon_ec == NULL) {
    len = sprintf(buf, "no echo canceller being monitored - make a new call\n");
    spin_unlock_irqrestore(&oslec_lock, flags);
    return len;
  }

  if (mon_ec->adaption_mode & ECHO_CAN_USE_ADAPTION)
    sprintf(mode_str, "|ADAPTION");
  else
    sprintf(mode_str, "|        ");

  if (mon_ec->adaption_mode & ECHO_CAN_USE_NLP)
    strcat(mode_str, "|NLP");
  else
    strcat(mode_str, "|   ");

  if (mon_ec->adaption_mode & ECHO_CAN_USE_CNG)
    strcat(mode_str, "|CNG");
  else if (mon_ec->adaption_mode & ECHO_CAN_USE_CLIP)
    strcat(mode_str, "|CLIP");
  else
    strcat(mode_str, "|   ");		
	
  if (mon_ec->adaption_mode & ECHO_CAN_USE_TX_HPF)
    strcat(mode_str, "|TXHPF");
  else
    strcat(mode_str, "|   ");		

  if (mon_ec->adaption_mode & ECHO_CAN_USE_RX_HPF)
    strcat(mode_str, "|RXHPF|");
  else
    strcat(mode_str, "|   |");		

  len = sprintf(buf,
		"channels.......: %d\n"
		"length (taps)..: %d\n"
		"mode...........: [%0d] %s\n"
		"Ltx............: %d\n"
		"Lrx............: %d\n"
		"Lclean.........: %d\n"
		"Lclean_bg......: %d\n"
		"shift..........: %d\n"
		"Double Talk....: %d\n"
		"Lbgn...........: %d\n"
		"MIPs (last)....: %d\n"
		"MIPs (worst)...: %d\n"
		"MIPs (avergage): %d\n",
		num_ec,
		len_ec,
		mon_ec->adaption_mode, mode_str,
		mon_ec->Ltx,
		mon_ec->Lrx,
		mon_ec->Lclean,
		mon_ec->Lclean_bg,
		mon_ec->shift,
		(mon_ec->nonupdate_dwell != 0),
		mon_ec->Lbgn,
		8*cycles_last/1000,
		8*cycles_worst/1000,
		8*cycles_average/1000
		);

  spin_unlock_irqrestore(&oslec_lock, flags);

  return len;
}

static int proc_read_mode(char *buf, char **start, off_t offset,
                          int count, int *eof, void *data)
{
  int len;
  unsigned long flags;

  *eof = 1;

  spin_lock_irqsave(&oslec_lock, flags);

  if (mon_ec == NULL) {
    len = sprintf(buf, "%d\n", 0);
    spin_unlock_irqrestore(&oslec_lock, flags);
    return len;
  }

  len = sprintf(buf, "%d\n", mon_ec->adaption_mode);

  spin_unlock_irqrestore(&oslec_lock, flags);

  return len;
}

static int proc_write_mode(struct file *file, const char *buffer,
                           unsigned long count, void *data)
{

  int   new_mode;
  char *endbuffer;
  unsigned long flags;

  spin_lock_irqsave(&oslec_lock, flags);

  if (mon_ec == NULL) {
    printk("no echo canceller being monitored - make a new call\n");
    spin_unlock_irqrestore(&oslec_lock, flags);
    return count;
  }

  new_mode = simple_strtol (buffer, &endbuffer, 10);
  mon_ec->adaption_mode = new_mode;

  spin_unlock_irqrestore(&oslec_lock, flags);

  return count;
}

static int proc_write_reset(struct file *file, const char *buffer,
                           unsigned long count, void *data)
{
  unsigned long flags;
  spin_lock_irqsave(&oslec_lock, flags);

  if (mon_ec == NULL) {
    printk("no echo canceller being monitored - make a new call\n");
    spin_unlock_irqrestore(&oslec_lock, flags);
    return count;
  }

  /* Not sure how thread safe this is, should be OKJ as its only resetting
     same state variables.  There is a chance this will be interrupted
     by ISR that calls oslec_echo_can_update() 
  */
  echo_can_flush(mon_ec);

  spin_unlock_irqrestore(&oslec_lock, flags);

  return count;
}

static int __init init_oslec(void)
{
    struct proc_dir_entry *proc_oslec, *proc_mode, *proc_reset;

    printk("Open Source Line Echo Canceller Installed\n");

    num_ec = 0;
    mon_ec = NULL;

    proc_oslec = proc_mkdir("oslec", 0);
    create_proc_read_entry("oslec/info", 0, NULL, proc_read_info, NULL);
    proc_mode = create_proc_read_entry("oslec/mode", 0, NULL, proc_read_mode, NULL);
    proc_reset = create_proc_read_entry("oslec/reset", 0, NULL, NULL, NULL);

    proc_mode->write_proc = proc_write_mode;
    proc_reset->write_proc = proc_write_reset;
    spin_lock_init(&oslec_lock);

    return 0;
}

static void __exit cleanup_oslec(void)
{
    remove_proc_entry("oslec/reset", NULL);
    remove_proc_entry("oslec/info", NULL);
    remove_proc_entry("oslec/mode", NULL);
    remove_proc_entry("oslec", NULL);
    printk("Open Source Line Echo Canceller Removed\n");
}

short oslec_hpf_tx(struct echo_can_state *ec, short txlin) {
    if (ec != NULL) 
        return echo_can_hpf_tx((echo_can_state_t*)(ec->ec), txlin);  
    else
        return txlin;
}

MODULE_LICENSE("GPL");
MODULE_AUTHOR("David Rowe");
MODULE_DESCRIPTION("Open Source Line Echo Canceller Zaptel Wrapper");

module_init(init_oslec);
module_exit(cleanup_oslec);

