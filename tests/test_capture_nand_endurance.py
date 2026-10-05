#!/usr/bin/env python3
import contextlib
import importlib.util
import io
import os
import struct
import tempfile
import threading
import zlib
from pathlib import Path

module_path = Path(__file__).parents[1] / "tools" / "capture_cb62_nand_endurance.py"
assert module_path.is_file(), "fixed-tier endurance receiver is missing"
spec = importlib.util.spec_from_file_location("capture_cb62_nand_endurance", module_path)
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)

def frame(page):
    main = bytes([page & 0xff]) * 2048
    oob = bytes([(page ^ 0xa5) & 0xff]) * 128
    data = main + oob
    return b"CB62NAND" + struct.pack("<8I", 1, page, page, 1, 0, 2048, 128,
                                      zlib.crc32(data)) + data

raw = b"".join(frame(page) for page in range(1024))
frames = module.validate_tier(raw, 1024)
assert len(frames) == 1024 and frames[-1].page == 1023
for bad in (raw[:-1], raw + b"x", raw[:2216] + raw[2*2216:],
            raw[:3000] + bytes([raw[3000] ^ 1]) + raw[3001:]):
    try:
        module.validate_tier(bad, 1024)
    except ValueError:
        pass
    else:
        raise AssertionError("accepted damaged endurance stream")
for count in (1, 64, 2048, 131072):
    try:
        module.validate_tier(b"", count)
    except ValueError:
        pass
    else:
        raise AssertionError("accepted non-allowlisted tier")

with tempfile.TemporaryDirectory() as temporary_directory:
    read_fd, write_fd = os.pipe()
    original = module._configure_uart
    module._configure_uart = lambda _device: read_fd
    def writer():
        view = memoryview(raw)
        while view:
            used = os.write(write_fd, view)
            view = view[used:]
        os.close(write_fd)
    thread = threading.Thread(target=writer); thread.start()
    prefix = str(Path(temporary_directory) / "tier")
    try:
        with contextlib.redirect_stdout(io.StringIO()):
            module._capture("synthetic-uart", prefix, 1024, 10)
    finally:
        module._configure_uart = original; thread.join(timeout=5)
    assert Path(prefix + ".uart-frames.bin").read_bytes() == raw
    assert Path(prefix + ".wire.bin").read_bytes() == raw
    assert Path(prefix + ".main.bin").stat().st_size == 1024 * 2048
    assert Path(prefix + ".oob.bin").stat().st_size == 1024 * 128
    assert not list(Path(temporary_directory).glob("*.partial"))

with tempfile.TemporaryDirectory() as temporary_directory:
    damaged = bytearray(frame(0)); damaged[-1] ^= 1
    read_fd, write_fd = os.pipe()
    original = module._configure_uart
    module._configure_uart = lambda _device: read_fd
    os.write(write_fd, damaged); os.close(write_fd)
    prefix = str(Path(temporary_directory) / "damaged")
    try:
        with contextlib.redirect_stdout(io.StringIO()):
            try:
                module._capture("synthetic-uart", prefix, 1024, 10)
            except ValueError as error:
                assert "page 0: CRC mismatch" in str(error)
            else:
                raise AssertionError("accepted corrupt live frame")
    finally:
        module._configure_uart = original
    assert Path(prefix + ".wire.bin.partial").read_bytes() == damaged
    assert Path(prefix + ".uart-frames.bin.partial").stat().st_size == 0

print("test_capture_nand_endurance: PASS")
