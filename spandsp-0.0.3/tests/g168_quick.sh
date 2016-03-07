#!/bin/sh
# g168_tests_quick.sh
#
# Script to run a quick G168 test.  We run all tests that
# have been implemented so far at the default levels, ERL,
# and models.
#
# David Rowe 22 Jan 2007

./echo_tests 2aa 
./echo_tests 2ca -q
./echo_tests 3a  -q
./echo_tests 3ba -q 
./echo_tests 3bb -q

