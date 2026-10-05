#!/bin/sh
set -eu
bin=${1:-out/cb62-nand-block0-uart-proof}
test -x "$bin"
set +e
output=$($bin 2>&1)
status=$?
set -e
test "$status" -eq 2
test "$output" = "Usage: cb62-nand-block0-uart-proof --execute-fixed-block0-uart-proof"
printf '%s\n' 'test_nand_block0_cli: PASS'
