# Makefile for oslec test kernel mode driver
# David Rowe 23 August 2009
#

KVERS := 2.6.15-27-686

obj-m	:= oslec.o

oslec-objs := oslec_test.o echo.o

KDIR	 := /lib/modules/$(KVERS)/build

all::
	$(MAKE) -C $(KDIR) SUBDIRS=$(PWD) modules 

clean::
	$(MAKE) -C $(KDIR) SUBDIRS=$(PWD) clean