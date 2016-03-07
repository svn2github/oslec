#!/bin/sh
# install-1.4.sh
# David Rowe 19 Feb 2007
#
# Test script to automate installation and testing of oslec, run
# from oslec/kernel.
#
# usage: $ ./install-1.4 /path/to/just/above/zaptel
#       
#
# Copyright David Rowe 2007 
# This program is distributed under the terms of the GNU General Public License 
# Version 2

if [ $# -ne 1 ] || [ ! -d $1 ]; then
    echo "usage: install-1.4 /path/to/zaptel"
    exit
fi
echo $1

PATCH_DIR=`pwd`
echo $PATCH_DIR
killall -9 asterisk
rmmod wctdm zaptel oslec 
make clean
make
insmod oslec.o
cd $1
rm -Rf zaptel-1.4.1
ls -l zaptel-1.4.1.tar.gz
tar xvzf zaptel-1.4.1.tar.gz
cd zaptel-1.4.1
patch -p1 < $PATCH_DIR/zaptel-1.4.1.patch
./configure
make
insmod zaptel.ko
dmesg | grep zaptel
insmod wctdm.ko 
./ztcfg
asterisk
