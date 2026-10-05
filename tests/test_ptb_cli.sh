#!/bin/sh
set -eu
bin=$1

help=$($bin --help)
printf '%s\n' "$help" | grep -F -- '--execute-one-fixed-ptb-snapshot-read --output FILE' >/dev/null
printf '%s\n' "$help" | grep -F -- 'fixed selector 16 and fixed length 0x1000' >/dev/null

if $bin --execute-one-fixed-ptb-snapshot-read >/tmp/cb62-ptb-cli.out 2>/tmp/cb62-ptb-cli.err; then
    echo 'missing output argument unexpectedly succeeded' >&2
    exit 1
fi
grep -F 'unsupported invocation; no transaction sent' /tmp/cb62-ptb-cli.err >/dev/null

existing=$(mktemp /tmp/cb62-ptb-existing.XXXXXX)
trap 'rm -f "$existing"' EXIT HUP INT TERM
if $bin --execute-one-fixed-ptb-snapshot-read --output "$existing" \
        >/tmp/cb62-ptb-cli.out 2>/tmp/cb62-ptb-cli.err; then
    echo 'existing output path unexpectedly succeeded' >&2
    exit 1
fi
grep -F 'output must be a new creatable file' /tmp/cb62-ptb-cli.err >/dev/null
test ! -s "$existing"

echo 'test_ptb_cli: PASS'
