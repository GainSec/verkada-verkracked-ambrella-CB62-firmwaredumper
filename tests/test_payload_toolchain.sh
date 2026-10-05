#!/bin/sh
set -eu

tmpdir=$(mktemp -d)
trap 'rm -rf "$tmpdir"' EXIT HUP INT TERM
touch "$tmpdir/llvm-objcopy-21"
chmod +x "$tmpdir/llvm-objcopy-21"

recipe=$(PATH="$tmpdir:$PATH" make -n -B \
    AARCH64_LD=/opt/test/aarch64-linker \
    build/payload/cb62_nand_endurance_1024.bin)

printf '%s\n' "$recipe" | grep -F '/opt/test/aarch64-linker' >/dev/null
printf '%s\n' "$recipe" | grep -F "$tmpdir/llvm-objcopy-21" >/dev/null
if printf '%s\n' "$recipe" | grep -F -- '-fuse-ld=lld' >/dev/null; then
    echo 'payload recipe still hard-codes Clang LLD discovery' >&2
    exit 1
fi

echo 'test_payload_toolchain: PASS'
