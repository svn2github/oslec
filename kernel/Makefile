# Makefile for oslec kernel mode driver
# David Rowe 7 Feb 2007
#
# Copyright David Rowe 2007 
# This program is distributed under the terms of the GNU General Public License 
# Version 2

ifeq ($(ARCH), blackfin)
EXTRA_CFLAGS    := -Idrivers/zaptel/oslec/kernel -Idrivers/zaptel/oslec/spandsp-0.0.3/src/spandsp -D__BLACKFIN__ -D__BLACKFIN_ASM__ -O6 -DEXPORT_SYMTAB
obj-$(CONFIG_OSLEC)        += oslec.o

oslec-objs := oslec_wrap.o \
	../spandsp-0.0.3/src/echo.o

else

KVERS := $(shell uname -r)
KERNEL26 := 2.6
KERNELVERSION := $(findstring $(KERNEL26),$(KVERS))
PWD := $(shell pwd)

ifeq ($(KERNELVERSION),2.6)

obj-m	:= oslec.o

oslec-objs := oslec_wrap.o \
        ../spandsp-0.0.3/src/echo.o

KDIR	 := /lib/modules/$(KVERS)/build
KINCLUDE := -I$(PWD)/../spandsp-0.0.3/src/spandsp 

all::
	$(MAKE) -C $(KDIR) EXTRA_CFLAGS='$(KINCLUDE) -DEXPORT_SYMTAB -O6' SUBDIRS=$(PWD) modules 

modules_install::
	$(MAKE) -C $(KDIR) EXTRA_CFLAGS='$(KINCLUDE) -DEXPORT_SYMTAB -O6' SUBDIRS=$(PWD) modules_install 

else

INCLUDE	:= -I/lib/modules/$(KVERS)/build/include -I/usr/include/asm/mach-default/
INCLUDE	+= -I../spandsp-0.0.3/src/spandsp -I.
CFLAGS	:= -O2 -Wall -DMODULE -D__KERNEL__ -DLINUX
CC	:= gcc

all:: oslec.o

oslec.o: oslec_wrap.o echo.o 
	ld -r oslec_wrap.o echo.o -o oslec.o 

oslec_wrap.o: oslec_wrap.c
	$(CC) $(CFLAGS) ${INCLUDE} -o oslec_wrap.o -c oslec_wrap.c 

echo.o: ../spandsp-0.0.3/src/echo.c
	$(CC) $(CFLAGS) ${INCLUDE} -o echo.o -c ../spandsp-0.0.3/src/echo.c 

endif

clean:
	rm -f *.o *.ko ../spandsp-0.0.3/src/echo.o *~ *.mod.c .oslec*

# usage: $ make patch SRC=/path/to/zaptel/src (run from oslec/kernel)
# In $(SRC) we expect to find:
#
#    zaptel-1.2.13-orig 
#    zaptel-1.2.13
#    zaptel-1.4.1-orig 
#    zaptel-1.4.1

patch:
	# 1.2.13 patch

	-cd $(SRC)/zaptel-1.2.13; \
	make clean; \
	rm -f *~; \
	cd ..; \
	diff -uN zaptel-1.2.13-orig zaptel-1.2.13 > $(PWD)/zaptel-1.2.13.patch

	# 1.2.18 patch

	-cd $(SRC)/zaptel-1.2.18; \
	make clean; \
	rm -f *~; \
	cd ..; \
	diff -uN zaptel-1.2.18-orig zaptel-1.2.18 > $(PWD)/zaptel-1.2.18.patch

	# 1.4.1 patch

	-cd $(SRC)/zaptel-1.4.1; \
	make clean; \
	rm -f *~ config.log config.status makeopts menuselect.makedeps \
	menuselect.makeopts menuselect-tree *.rej; \
	cd ..; \
	diff -uN zaptel-1.4.1-orig zaptel-1.4.1 > $(PWD)/zaptel-1.4.1.patch


endif
