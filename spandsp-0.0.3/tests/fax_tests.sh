#!/bin/bash
#
# spandsp fax tests
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License version 2, as
# published by the Free Software Foundation.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
#
# $Id: fax_tests.sh,v 1.2 2006/10/24 13:49:53 steveu Exp $
#

run_fax_test()
{
    rm -f fax_tests_1.tif
    echo -i ${FILE} ${OPTS}
    ./fax_tests -i ${FILE} ${OPTS} >xyzzy 2>xyzzy2
    RETVAL=$?
    if [ $RETVAL != 0 ]
    then
        echo fax_tests failed!
        exit $RETVAL
    fi
    # Now use tiffcmp to check the results. It will return non-zero if any page images differ. The -t
    # option means the normal differences in tags will be ignored.
    tiffcmp -t ${FILE} fax_tests_1.tif >/dev/null
    RETVAL=$?
    if [ $RETVAL != 0 ]
    then
        echo fax_tests failed!
        exit $RETVAL
    fi
    rm -f fax_tests_1.tif
    echo tested ${FILE}
}

OPTS="-e"

FILE="../itutests/fax/R8_385_A4.tif"
run_fax_test

FILE="../itutests/fax/R16_385_A4.tif"
run_fax_test

FILE="../itutests/fax/R8_77_A4.tif"
run_fax_test

FILE="../itutests/fax/R16_77_A4.tif"
run_fax_test

FILE="../itutests/fax/R8_154_A4.tif"
run_fax_test

FILE="../itutests/fax/R16_154_A4.tif"
run_fax_test

FILE="../itutests/fax/R8_385_B4.tif"
run_fax_test

FILE="../itutests/fax/R16_385_B4.tif"
run_fax_test

FILE="../itutests/fax/R8_77_B4.tif"
run_fax_test

FILE="../itutests/fax/R16_77_B4.tif"
run_fax_test

FILE="../itutests/fax/R8_154_B4.tif"
run_fax_test

FILE="../itutests/fax/R16_154_B4.tif"
run_fax_test

FILE="../itutests/fax/R8_385_A3.tif"
run_fax_test

FILE="../itutests/fax/R16_385_A3.tif"
run_fax_test

FILE="../itutests/fax/R8_77_A3.tif"
run_fax_test

FILE="../itutests/fax/R16_77_A3.tif"
run_fax_test

FILE="../itutests/fax/R8_154_A3.tif"
run_fax_test

FILE="../itutests/fax/R16_154_A3.tif"
run_fax_test

OPTS=""

FILE="../itutests/fax/R8_385_A4.tif"
run_fax_test

FILE="../itutests/fax/R16_385_A4.tif"
run_fax_test

FILE="../itutests/fax/R8_77_A4.tif"
run_fax_test

FILE="../itutests/fax/R16_77_A4.tif"
run_fax_test

FILE="../itutests/fax/R8_154_A4.tif"
run_fax_test

FILE="../itutests/fax/R16_154_A4.tif"
run_fax_test

FILE="../itutests/fax/R8_385_B4.tif"
run_fax_test

FILE="../itutests/fax/R16_385_B4.tif"
run_fax_test

FILE="../itutests/fax/R8_77_B4.tif"
run_fax_test

FILE="../itutests/fax/R16_77_B4.tif"
run_fax_test

FILE="../itutests/fax/R8_154_B4.tif"
run_fax_test

FILE="../itutests/fax/R16_154_B4.tif"
run_fax_test

FILE="../itutests/fax/R8_385_A3.tif"
run_fax_test

FILE="../itutests/fax/R16_385_A3.tif"
run_fax_test

FILE="../itutests/fax/R8_77_A3.tif"
run_fax_test

FILE="../itutests/fax/R16_77_A3.tif"
run_fax_test

FILE="../itutests/fax/R8_154_A3.tif"
run_fax_test

FILE="../itutests/fax/R16_154_A3.tif"
run_fax_test
