#!/usr/bin/env python3
import importlib.util
import struct
import subprocess
import zlib
from pathlib import Path

project = Path(__file__).parents[1]
payload = (project / "payload" / "cb62_nand_range.c").read_text()
makefile = (project / "Makefile").read_text()
main_source = (project / "src" / "nand_range_main.c").read_text()

for needle in (
    "#ifndef RANGE_START_PAGE",
    "#ifndef RANGE_PAGE_COUNT",
    "#ifndef NAND_READ_PAGES_ADDR",
    "#ifndef UART_INIT_ADDR",
    "#ifndef REQUESTED_UART_BAUD",
    "#define TOTAL_NAND_PAGES 131072u",
    "RANGE_START_PAGE + sequence",
    "page / PAGES_PER_BLOCK",
    "page % PAGES_PER_BLOCK",
    "uart_init(0u, REQUESTED_UART_BAUD);",
):
    assert needle in payload, f"missing fixed-range contract: {needle}"

for forbidden in ("erase", "program", "write_spare", "mark_bad", "retry"):
    assert forbidden not in payload.lower(), f"forbidden capability: {forbidden}"

for needle in (
    "NAND_RANGE_PROFILE ?= cb62-original-resident",
    "NAND_READ_PAGES_ADDR := 0x01411a60u",
    "UART_INIT_ADDR := 0x0140ed70u",
    "NAND_UART_BAUD := 1000000u",
    "NAND_READ_PAGES_ADDR := 0x00013320u",
    "UART_INIT_ADDR := 0x00011000u",
    "NAND_UART_BAUD := 115200u",
):
    assert needle in makefile, f"missing target profile setting: {needle}"

assert '"--execute-fixed-range-" TARGET_PROFILE_NAME' in main_source

recipe = subprocess.run([
    "make", "-n", "-B", "nand-range",
    "NAND_RANGE_PROFILE=cb62-specimen2-2gb-bld0",
    "NAND_RANGE_START=5237", "NAND_RANGE_COUNT=125835",
], cwd=project, text=True, capture_output=True, check=True).stdout
for needle in (
    "-DNAND_READ_PAGES_ADDR=0x00013320u",
    "-DUART_INIT_ADDR=0x00011000u",
    "-DREQUESTED_UART_BAUD=115200u",
    "cb62-nand-range-cb62-specimen2-2gb-bld0-5237-125835",
):
    assert needle in recipe, f"newer CB62 recipe missing: {needle}"

module_path = project / "tools" / "capture_cb62_nand_range.py"
spec = importlib.util.spec_from_file_location("capture_cb62_nand_range",
                                              module_path)
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)


def frame(sequence, page):
    main = bytes([page & 0xff]) * 2048
    oob = bytes([(page ^ 0xa5) & 0xff]) * 128
    data = main + oob
    return b"CB62NAND" + struct.pack(
        "<8I", 1, sequence, page, 1, 0, 2048, 128, zlib.crc32(data)
    ) + data


raw = b"".join(frame(sequence, 717 + sequence) for sequence in range(4))
frames = module.validate_range(raw, 717, 4)
assert [item.page for item in frames] == [717, 718, 719, 720]
assert module._configure_uart.__code__.co_argcount == 2
try:
    module._configure_uart("/dev/null", 9600)
except ValueError:
    pass
else:
    raise AssertionError("accepted an unsupported UART rate")

for bad_start, bad_count, damaged in (
    (716, 4, raw),
    (717, 5, raw),
    (717, 4, raw[:-1]),
    (717, 4, raw[:3000] + bytes([raw[3000] ^ 1]) + raw[3001:]),
):
    try:
        module.validate_range(damaged, bad_start, bad_count)
    except ValueError:
        pass
    else:
        raise AssertionError("accepted incorrect or damaged fixed range")

print("test_nand_range: PASS")
