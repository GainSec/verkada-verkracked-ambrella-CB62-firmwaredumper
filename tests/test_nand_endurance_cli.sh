#!/bin/sh
set -eu
out_dir=${1:-out}
for count in 1024 4096 8192; do
    bin="$out_dir/cb62-nand-endurance-$count"
    test -x "$bin"
    set +e
    output=$($bin 2>&1)
    status=$?
    set -e
    test "$status" -eq 2
    test "$output" = "Usage: cb62-nand-endurance-$count --execute-fixed-endurance-$count"
done
printf '%s\n' 'test_nand_endurance_cli: PASS'
