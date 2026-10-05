#!/usr/bin/env python3
from pathlib import Path


source = Path(__file__).parents[1] / "payload" / "cb62_nand_block0.c"
assert source.is_file(), "fixed block-0 payload source is missing"
text = source.read_text()

required = (
    "#define NAND_READ_PAGES_ADDR 0x01411a60u",
    "#define FIRST_PAGE 0u",
    "#define PAGE_COUNT 64u",
    "read_pages(0u, page, 1u, main_data, spare_data, 0u)",
    "for (uint32_t page = FIRST_PAGE; page < PAGE_COUNT; page++)",
)
for needle in required:
    assert needle in text, f"missing fixed safety contract: {needle}"

for forbidden in ("erase", "program", "write_spare", "mark_bad", "argv", "retry"):
    assert forbidden not in text.lower(), f"forbidden capability token: {forbidden}"

print("test_nand_block0_payload: PASS")
