# Makefile for Oslec
# David Rowe June 8 2007

TOPDIR = $(shell pwd)
export TOPDIR

VERSION = 0.1
PROJ=oslec
REV     := $(shell svn info | grep Revision | sed 's/Revision: //')

DATE = $(shell date '+%d %b %Y')

# What you really want is the kernel dir Makefile

all: 
	make -C kernel

# generate tarball distro

dist:
	make clean

	# keep track of which SVN revision this tar ball is

	echo $(REV) > SVN_REVISION

	# create tar ball containing only directories we need

	cd ..; \
	if [ -d $(PROJ)-$(VERSION) ] ; then \
		echo "error $(PROJ)-$(VERSION) exists"; \
	        exit 1; \
	fi
	cd ..; ln -s $(PROJ) $(PROJ)-$(VERSION)
	cd ..; tar chz --exclude=*~ --exclude=.svn \
	-f $(PROJ)-$(VERSION).tar.gz $(PROJ)-$(VERSION) 
	@echo
	@echo "Tar ball created in ../"
	ls -lh ../$(PROJ)-$(VERSION).tar.gz
	rm -Rf ../$(PROJ)-$(VERSION)

clean:
	rm -f *~
	cd kernel; make clean
	cd user; make clean

