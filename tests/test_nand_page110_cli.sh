#!/bin/sh
set -eu
bin=${1:?page-110 binary required}

if "$bin" >/dev/null 2>&1; then
    echo "page-110 CLI accepted a missing token" >&2
    exit 1
fi
if "$bin" --execute-fixed-page0-uart-proof >/dev/null 2>&1; then
    echo "page-110 CLI accepted the page-0 token" >&2
    exit 1
fi
if "$bin" --execute-fixed-page110-uart-proof extra >/dev/null 2>&1; then
    echo "page-110 CLI accepted an extra argument" >&2
    exit 1
fi

echo "test_nand_page110_cli: PASS"
