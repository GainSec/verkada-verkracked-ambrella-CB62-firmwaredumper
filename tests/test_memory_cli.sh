#!/bin/sh
set -eu

bin=${1:-./out/cb62-amboot-header-probe}
tmpdir=$(mktemp -d)
trap 'rm -rf "$tmpdir"' EXIT HUP INT TERM

"$bin" --help >"$tmpdir/help.out" 2>"$tmpdir/help.err"
grep -q '^Usage: cb62-amboot-header-probe --execute-one-read$' \
    "$tmpdir/help.out"
grep -q '0x01400038' "$tmpdir/help.out"
grep -q 'exactly one fixed 32-bit read' "$tmpdir/help.out"
grep -q 'no further transaction' "$tmpdir/help.out"

assert_exit_2()
{
    name=$1
    shift
    if "$bin" "$@" >"$tmpdir/$name.out" 2>"$tmpdir/$name.err"; then
        echo "$name invocation unexpectedly succeeded" >&2
        exit 1
    else
        code=$?
        test "$code" -eq 2
    fi
    grep -q 'no transaction sent' "$tmpdir/$name.err"
}

assert_exit_2 noarg
assert_exit_2 address --address 0x01400038
assert_exit_2 length --length 4
assert_exit_2 firmware --firmware image.bin
assert_exit_2 upload --upload image.bin
assert_exit_2 jump --jump 0x01400000
assert_exit_2 erase --erase
assert_exit_2 program --program image.bin
assert_exit_2 extra --execute-one-read extra

echo 'test_memory_cli: PASS'
