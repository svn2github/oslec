#!/bin/sh
# test-1.2.sh
# David Rowe 19 Feb 2007
#
# Test script to automate installation and testing of oslec, run
# from oslec/kernel.  Similar to install-1.2.sh but doesnt patch
# zaptel.  Used during development.
#
# usage: $test-1.2 /path/to/just/above/zaptel
#        $test-1.2 ~
#
# Copyright David Rowe 2007 
# This program is distributed under the terms of the GNU General Public License 
# Version 2

if [ $# -ne 1 ] || [ ! -d $1 ]; then
    echo "usage: test-1.2 /path/to/zaptel"
    exit
fi
echo $1

killall -9 asterisk
rmmod wctdm zaptel oslec 
make clean && make
insmod oslec.o
cd $1/zaptel-1.2.13
make
insmod zaptel.o
dmesg | grep zaptel
insmod wctdm.o 
./ztcfg
asterisk
