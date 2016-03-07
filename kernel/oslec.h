/*
  oslec.h
  David Rowe
  7 Feb 2007

  Interface for OSLEC module.
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

#ifndef __OSLEC__
#define __OSLEC__

struct echo_can_state {
  void *ec;
};

struct echo_can_state *oslec_echo_can_create(int len, int adaption_mode);
void oslec_echo_can_free(struct echo_can_state *ec);
short oslec_echo_can_update(struct echo_can_state *ec, short iref, short isig);
int oslec_echo_can_traintap(struct echo_can_state *ec, int pos, short val);
void oslec_echo_can_identify(char *buf, size_t len);
static inline void echo_can_init(void) { printk("Zaptel Echo Canceller: OSLEC\n"); }
static inline void echo_can_shutdown(void) {}
short oslec_hpf_tx(struct echo_can_state *ec, short txlin);

#endif

