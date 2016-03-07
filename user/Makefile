# Makefile for user mode oslec test programs
# David Rowe December 16 2006
#
# Copyright David Rowe 2007 
# This program is distributed under the terms of the GNU General Public License 
# Version 2

TOPDIR = $(shell pwd)
export TOPDIR

# extract dir from /path/to/dir
RELCWD = $(shell pwd | sed -e "s/.*\///")

VERSION = 0.1
PROJ=zaptap

DATE = $(shell date '+%d %b %Y')

TARGETS = sample speedtest 

# add Blackfin targets if Blackfin toolchain is present

BLACKFIN=$(shell which bfin-linux-uclibc-gcc 2>/dev/null)
ifeq ($(BLACKFIN),)
else
TARGETS += speedtest.bf speedtest_slow.bf tfir.bf
endif
 
all: $(TARGETS)

sample: sample.c
	gcc sample.c -o sample -Wall

speedtest: speedtest.c
	gcc speedtest.c -O6 -I../spandsp-0.0.3/src/spandsp/ \
	../spandsp-0.0.3/src/echo.c -o speedtest -Wall

echo.s : ../spandsp-0.0.3/src/echo.c
	bfin-linux-uclibc-gcc -D__BLACKFIN__ -D__BLACKFIN_ASM__ -O6 \
	-I../spandsp-0.0.3/src/spandsp/ \
	../spandsp-0.0.3/src/echo.c -c -S -Wall

speedtest.bf: speedtest.c echo.s
	bfin-linux-uclibc-gcc -D__BLACKFIN__ -D__BLACKFIN_ASM__ speedtest.c -O6 \
	-I../spandsp-0.0.3/src/spandsp/ \
	echo.s -o speedtest.bf -Wall

speedtest_slow.bf: speedtest.c
	bfin-linux-uclibc-gcc -D__BLACKFIN__ speedtest.c -O6 \
	-I../spandsp-0.0.3/src/spandsp/ \
	../spandsp-0.0.3/src/echo.c -o speedtest_slow.bf -Wall

tfir.bf: tfir.c
	bfin-linux-uclibc-gcc -D__BLACKFIN_ASM__ \
	-I../spandsp-0.0.3/src/spandsp/ \
	tfir.c -o tfir.bf -Wall

# generate tarball distro

dist:
	@if [ ! -d ../$(PROJ) ]; then \
	  echo "Please change directory name to $(PROJ)"; \
	  exit 1; \
        fi 

	# remove unneeded files
	rm -f *~ 

	# create tar ball containing only directories we need
	cd ..; tar vczf $(PROJ)-$(VERSION).tar.gz $(RELCWD)
	@echo
	@echo "Tar ball created in ../"
	ls -lh ../$(PROJ)-$(VERSION).tar.gz

clean:
	rm -f $(TARGETS) *~

publish:
	/home/david/attic/asciidoc-7.0.4/asciidoc.py \
	-a stylesdir=$(TOPDIR)/asciidoc -f $(TOPDIR)/asciidoc/asciidoc.conf \
	 --unsafe -a date='$(DATE)' -a icons -a badges README.txt 
	scp README.html $(WEBSITE)/zaptap.html
