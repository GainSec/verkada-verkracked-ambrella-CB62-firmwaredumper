#!/bin/sh
set -eu

bin=${1:?binary path required}
profile=${2:?profile required}
start=${3:?start page required}
count=${4:?page count required}
expected="Usage: cb62-nand-range-$profile-$start-$count --execute-fixed-range-$profile-$start-$count"

set +e
output=$($bin 2>&1)
status=$?
set -e

test "$status" -eq 2
test "$output" = "$expected"
printf '%s\n' 'test_nand_range_cli: PASS'
