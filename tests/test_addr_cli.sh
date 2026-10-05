#!/bin/sh
set -eu

bin=${1:-./out/cb62-inquiry-addr-probe}
tmpdir=$(mktemp -d)
trap 'rm -rf "$tmpdir"' EXIT HUP INT TERM

"$bin" --help >"$tmpdir/help.out" 2>"$tmpdir/help.err"
grep -q '^Usage: cb62-inquiry-addr-probe --execute-one-inquiry$' \
    "$tmpdir/help.out"
grep -q 'exactly one fixed INQUIRY_ADDR' "$tmpdir/help.out"

if "$bin" >"$tmpdir/noarg.out" 2>"$tmpdir/noarg.err"; then
    echo 'no-argument invocation unexpectedly succeeded' >&2
    exit 1
else
    code=$?
    test "$code" -eq 2
fi

if "$bin" --firmware anything >"$tmpdir/firmware.out" \
    2>"$tmpdir/firmware.err"; then
    echo 'disallowed --firmware unexpectedly succeeded' >&2
    exit 1
else
    code=$?
    test "$code" -eq 2
fi

if "$bin" --execute-one-inquiry extra >"$tmpdir/extra.out" \
    2>"$tmpdir/extra.err"; then
    echo 'extra argument unexpectedly succeeded' >&2
    exit 1
else
    code=$?
    test "$code" -eq 2
fi

echo 'test_addr_cli: PASS'
