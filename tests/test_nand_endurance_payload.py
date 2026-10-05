#!/usr/bin/env python3
from pathlib import Path


project = Path(__file__).parents[1]
source = project / "payload" / "cb62_nand_endurance.c"
linker = project / "payload" / "cb62_nand_endurance.ld"
makefile = project / "Makefile"

assert source.is_file(), "fixed endurance payload source is missing"
assert linker.is_file(), "fixed endurance linker script is missing"
text = source.read_text()
rules = makefile.read_text()

for needle in (
    "#define NAND_READ_PAGES_ADDR 0x01411a60u",
    "#define FIRST_PAGE 0u",
    "#define PAGES_PER_BLOCK 64u",
    "#ifndef ENDURANCE_PAGE_COUNT",
    "uint32_t block = page / PAGES_PER_BLOCK;",
    "uint32_t page_in_block = page % PAGES_PER_BLOCK;",
    "read_pages(block, page_in_block, 1u,",
    "main_data, spare_data, 0u)",
    "for (uint32_t page = FIRST_PAGE; page < ENDURANCE_PAGE_COUNT; page++)",
):
    assert needle in text, f"missing fixed safety contract: {needle}"

for forbidden in ("erase", "program", "write_spare", "mark_bad", "argv", "retry"):
    assert forbidden not in text.lower(), f"forbidden capability token: {forbidden}"

assert "read_pages(0u, page," not in text, "linear page was passed as page-within-block"

assert "ENDURANCE_COUNTS := 1024 4096 8192" in rules
for count in (1024, 4096, 8192):
    assert f"-DENDURANCE_PAGE_COUNT={count}u" in rules

assert ". = 0x02000000;" in linker.read_text()
print("test_nand_endurance_payload: PASS")
