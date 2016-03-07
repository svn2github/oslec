#!/bin/sh
# g168_tests.sh
#
# Script to run G168 tests.  As the tests are implemented and
# the echo can develops the idea is to fill this script out
# will all the tests required to test the echo can against G168.
#
# Created David Rowe 17 Jan 2007

echo "Test 2A(a) ---------------------------------------------"
echo
echo "Different models"
echo
./echo_tests 2aa -m 1
./echo_tests 2aa -m 2 -q 
./echo_tests 2aa -m 3 -q
./echo_tests 2aa -m 4 -q
./echo_tests 2aa -m 5 -q
./echo_tests 2aa -m 6 -q
./echo_tests 2aa -m 7 -q
echo
echo "Different ERLs"
echo
./echo_tests 2aa -erl 30
./echo_tests 2aa -erl 18 -q 
./echo_tests 2aa -erl 12 -q 
./echo_tests 2aa -erl 10 -q 
./echo_tests 2aa -erl  8 -q 
./echo_tests 2aa -erl  6 -q 
echo
echo "Different Rin levels"
echo
./echo_tests 2aa -m 1 -r -30
./echo_tests 2aa -m 1 -r -24 -q
./echo_tests 2aa -m 1 -r -18 -q
./echo_tests 2aa -m 1 -r -12 -q
./echo_tests 2aa -m 1 -r -6  -q
./echo_tests 2aa -m 1 -r  0  -q
echo
echo "Test 3B(a) ---------------------------------------------"
echo
echo "Range of Rin and Sgen values"
echo
./echo_tests 3ba -m 1 -r -30 -s -30    
./echo_tests 3ba -m 1 -r -30 -s -18 -q
./echo_tests 3ba -m 1 -r -30 -s -6  -q
./echo_tests 3ba -m 1 -r -18 -s -18 -q
./echo_tests 3ba -m 1 -r -18 -s -6  -q
./echo_tests 3ba -m 1 -r -6  -s -6  -q   
echo
echo "Test 3B(b) ---------------------------------------------"
echo
echo "Range of Rin and x values"
echo
./echo_tests 3bb -m 1 -r -30 -x 6    
./echo_tests 3bb -m 1 -r -30 -x 12 -q 
./echo_tests 3bb -m 1 -r -30 -x 18 -q 
./echo_tests 3bb -m 1 -r -30 -x 24 -q 
./echo_tests 3bb -m 1 -r -30 -x 30 -q 
echo
./echo_tests 3bb -m 1 -r -18 -x 6     
./echo_tests 3bb -m 1 -r -18 -x 12 -q
./echo_tests 3bb -m 1 -r -18 -x 18 -q
./echo_tests 3bb -m 1 -r -18 -x 24 -q
./echo_tests 3bb -m 1 -r -18 -x 30 -q
echo
./echo_tests 3bb -m 1 -r -6  -x 6    
./echo_tests 3bb -m 1 -r -6  -x 12 -q
./echo_tests 3bb -m 1 -r -6  -x 18 -q
./echo_tests 3bb -m 1 -r -6  -x 24 -q
./echo_tests 3bb -m 1 -r -6  -x 30 -q





