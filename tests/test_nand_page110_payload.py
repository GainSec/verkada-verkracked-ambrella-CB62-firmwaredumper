#!/usr/bin/env python3
from pathlib import Path

project = Path(__file__).parents[1]
text = (project / "payload" / "cb62_nand_page110.c").read_text()

for needle in (
    "#define NAND_READ_PAGES_ADDR 0x01411a60u",
    "#define TARGET_BLOCK 1u",
    "#define TARGET_PAGE_IN_BLOCK 46u",
    "#define TARGET_LINEAR_PAGE 110u",
    "read_pages(TARGET_BLOCK, TARGET_PAGE_IN_BLOCK, 1u,",
    "uart_u32(TARGET_LINEAR_PAGE);",
):
    assert needle in text, f"missing fixed page-110 contract: {needle}"

for forbidden in ("erase", "program", "write_spare", "mark_bad", "argv", "retry"):
    assert forbidden not in text.lower(), f"forbidden capability token: {forbidden}"

print("test_nand_page110_payload: PASS")
