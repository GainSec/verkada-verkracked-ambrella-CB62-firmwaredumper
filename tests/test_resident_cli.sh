#!/bin/sh
set -eu
bin=$1

help=$($bin --help)
printf '%s\n' "$help" | grep -F -- '--execute-one-fixed-resident-bld-read --output FILE' >/dev/null
printf '%s\n' "$help" | grep -F -- 'fixed address 0x01400000 and fixed length 0x34000' >/dev/null

if $bin --execute-one-fixed-resident-bld-read >/tmp/cb62-resident-cli.out 2>/tmp/cb62-resident-cli.err; then
    echo 'missing output argument unexpectedly succeeded' >&2
    exit 1
fi
grep -F 'unsupported invocation; no transaction sent' /tmp/cb62-resident-cli.err >/dev/null

existing=$(mktemp /tmp/cb62-resident-existing.XXXXXX)
trap 'rm -f "$existing"' EXIT HUP INT TERM
if $bin --execute-one-fixed-resident-bld-read --output "$existing" \
        >/tmp/cb62-resident-cli.out 2>/tmp/cb62-resident-cli.err; then
    echo 'existing output path unexpectedly succeeded' >&2
    exit 1
fi
grep -F 'output must be a new creatable file' /tmp/cb62-resident-cli.err >/dev/null
test ! -s "$existing"

echo 'test_resident_cli: PASS'
