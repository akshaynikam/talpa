#! /bin/bash
#
# TALPA test script
#
# Copyright (C) 2004-2021 Sophos Limited, Oxford, England.
#
# This program is free software; you can redistribute it and/or modify it under the terms of the
# GNU General Public License Version 2 as published by the Free Software Foundation.
#
# This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without
# even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
# See the GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License along with this program; if not,
# write to the Free Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
#

. ${srcdir}/tlp-cleanup.sh

inc=1
rc=1

function testone
{
    if [ "$1" = "yes" ]; then
        . ${srcdir}/tlp-cleanup.sh

        tlp_insmod modules/tlp-exclusion.${ko}

        testpath=${2}

        if [[ ! -f $testpath ]]; then
            echo >&2 "config not available"
            exit $inc
        fi

        EXPECTED=""

        ABC=abcdefghijklmnopqrstuvwxyz

        for x in 0 1 2 3 4 5 6 7 8 9 a b c d e f g h i j k l m n o p q r s t u v w x y z
        do
            for y in 0 1 2 3 4 5 6 7 8 9 a b c d e f g h i j k l m n o p q r s t u v w x y z
            do
                C="/$x/$y/$ABC/$ABC/$ABC/$ABC/$ABC/$ABC/$ABC/$ABC/$ABC/$ABC/$ABC/$ABC"
                echo "+$C" >$testpath
                if [[ -z $EXPECTED ]] ; then
                    EXPECTED="$C"
                else
                    EXPECTED="$EXPECTED
$C"
                fi

                LINES=$(cat "$testpath" | wc -l)
                EXPECTED_LINES=$(echo "$EXPECTED" | wc -l)
                if [[ $LINES != $EXPECTED_LINES ]]
                then
                    echo >&2 "Line counts differ: '$LINES' vs '$EXPECTED_LINES'"
                    exit $inc
                fi
            done
        done

        let inc=($inc)+1
        rc=0
        fi
}

procfs=$talpaprocfs
securityfs=$talpasecurityfs

testone $procfs /proc/sys/talpa/intercept-filters/FilesystemExclusionProcessor/paths
testone $securityfs /sys/kernel/security/talpa/intercept-filters/FilesystemExclusionProcessor/paths

exit $rc
