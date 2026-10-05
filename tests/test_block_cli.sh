#!/bin/sh
set -eu
bin=$1

help=$($bin --help)
printf '%s\n' "$help" | grep -F -- '--execute-one-fixed-header-read --output FILE' >/dev/null
printf '%s\n' "$help" | grep -F -- 'fixed address 0x01400000 and fixed length 64' >/dev/null

if $bin --execute-one-fixed-header-read >/tmp/cb62-block-cli.out 2>/tmp/cb62-block-cli.err; then
    echo 'missing output argument unexpectedly succeeded' >&2
    exit 1
fi
grep -F 'unsupported invocation; no transaction sent' /tmp/cb62-block-cli.err >/dev/null

if $bin --output /tmp/nope --execute-one-fixed-header-read >/tmp/cb62-block-cli.out 2>/tmp/cb62-block-cli.err; then
    echo 'reordered arguments unexpectedly succeeded' >&2
    exit 1
fi
grep -F 'unsupported invocation; no transaction sent' /tmp/cb62-block-cli.err >/dev/null

existing=$(mktemp /tmp/cb62-block-existing.XXXXXX)
trap 'rm -f "$existing"' EXIT HUP INT TERM
if $bin --execute-one-fixed-header-read --output "$existing" \
        >/tmp/cb62-block-cli.out 2>/tmp/cb62-block-cli.err; then
    echo 'existing output path unexpectedly succeeded' >&2
    exit 1
fi
grep -F 'output must be a new creatable file' /tmp/cb62-block-cli.err >/dev/null
test ! -s "$existing"

echo 'test_block_cli: PASS'
