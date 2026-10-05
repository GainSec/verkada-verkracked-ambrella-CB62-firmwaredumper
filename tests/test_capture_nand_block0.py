#!/usr/bin/env python3
import importlib.util
import os
import struct
import tempfile
import threading
import zlib
from pathlib import Path


module_path = Path(__file__).parents[1] / "tools" / "capture_cb62_nand_block0.py"
assert module_path.is_file(), "fixed block-0 receiver is missing"
spec = importlib.util.spec_from_file_location("capture_cb62_nand_block0", module_path)
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)
assert hasattr(module, "validate_complete_block0"), "block validator is missing"


def frame(page):
    main = bytes([page]) * 2048
    oob = bytes([page ^ 0xA5]) * 128
    data = main + oob
    header = b"CB62NAND" + struct.pack(
        "<8I", 1, page, page, 1, 0, len(main), len(oob), zlib.crc32(data)
    )
    return header + data


raw = b"".join(frame(page) for page in range(64))
frames = module.validate_complete_block0(raw)
assert len(frames) == 64
assert frames[0].page == 0 and frames[-1].page == 63
assert frames[7].main == bytes([7]) * 2048
assert frames[7].oob == bytes([7 ^ 0xA5]) * 128

for label, damaged in (
    ("truncated", raw[:-1]),
    ("trailing", raw + b"x"),
    ("gap", raw[: 7 * 2216] + raw[8 * 2216 :]),
    ("crc", raw[:100] + bytes([raw[100] ^ 1]) + raw[101:]),
):
    try:
        module.validate_complete_block0(damaged)
    except ValueError:
        pass
    else:
        raise AssertionError(f"accepted {label} block")


# Exercise the live capture path against the same contiguous byte stream the
# UART produces.  This guards the frame-boundary handling separately from the
# offline block validator above.
with tempfile.TemporaryDirectory() as temporary_directory:
    read_fd, write_fd = os.pipe()
    original_configure_uart = module._configure_uart
    module._configure_uart = lambda _device: read_fd

    def write_stream():
        view = memoryview(raw)
        while view:
            written = os.write(write_fd, view)
            view = view[written:]
        os.close(write_fd)

    writer = threading.Thread(target=write_stream)
    writer.start()
    try:
        prefix = str(Path(temporary_directory) / "block0")
        module._capture("synthetic-uart", prefix, 5)
    finally:
        module._configure_uart = original_configure_uart
        writer.join(timeout=5)

    captured = Path(prefix + ".uart-frames.bin").read_bytes()
    assert captured == raw

print("test_capture_nand_block0: PASS")
