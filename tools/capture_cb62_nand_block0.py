#!/usr/bin/env python3
import argparse
import dataclasses
import json
import os
import select
import struct
import termios
import time
import zlib

MAGIC = b"CB62NAND"
HEADER_SIZE = 40
MAIN_SIZE = 2048
OOB_SIZE = 128
FRAME_SIZE = HEADER_SIZE + MAIN_SIZE + OOB_SIZE
PAGE_COUNT = 64


@dataclasses.dataclass(frozen=True)
class Frame:
    sequence: int
    page: int
    main: bytes
    oob: bytes
    crc32: int


def _decode_frame(raw, expected_page):
    if len(raw) != FRAME_SIZE:
        raise ValueError(f"page {expected_page}: frame size {len(raw)} != {FRAME_SIZE}")
    if raw[:8] != MAGIC:
        raise ValueError(f"page {expected_page}: bad magic")
    version, sequence, page, count, status, main_len, oob_len, crc = struct.unpack(
        "<8I", raw[8:HEADER_SIZE]
    )
    if (version, sequence, page, count) != (1, expected_page, expected_page, 1):
        raise ValueError(f"page {expected_page}: identity mismatch")
    signed_status = status if status < 0x80000000 else status - 0x100000000
    if signed_status != 0:
        raise ValueError(f"page {expected_page}: read status {signed_status}")
    if (main_len, oob_len) != (MAIN_SIZE, OOB_SIZE):
        raise ValueError(f"page {expected_page}: length mismatch")
    data = raw[HEADER_SIZE:]
    computed = zlib.crc32(data) & 0xFFFFFFFF
    if computed != crc:
        raise ValueError(f"page {expected_page}: CRC mismatch")
    return Frame(sequence, page, data[:MAIN_SIZE], data[MAIN_SIZE:], crc)


def validate_complete_block0(raw):
    if len(raw) != PAGE_COUNT * FRAME_SIZE:
        raise ValueError(f"block size {len(raw)} != {PAGE_COUNT * FRAME_SIZE}")
    return [
        _decode_frame(raw[page * FRAME_SIZE:(page + 1) * FRAME_SIZE], page)
        for page in range(PAGE_COUNT)
    ]


def _read_exact(fd, length, deadline):
    data = bytearray()
    while len(data) < length:
        remaining = deadline - time.monotonic()
        if remaining <= 0 or not select.select([fd], [], [], remaining)[0]:
            raise TimeoutError(f"received {len(data)} of {length} bytes")
        chunk = os.read(fd, length - len(data))
        if not chunk:
            raise EOFError("serial device returned EOF")
        data.extend(chunk)
    return bytes(data)


def _configure_uart(device):
    fd = os.open(device, os.O_RDONLY | os.O_NOCTTY | os.O_NONBLOCK)
    attrs = termios.tcgetattr(fd)
    attrs[0] = attrs[1] = attrs[3] = 0
    attrs[2] = termios.CS8 | termios.CREAD | termios.CLOCAL
    attrs[4] = attrs[5] = termios.B115200
    attrs[6][termios.VMIN] = 1
    attrs[6][termios.VTIME] = 0
    termios.tcsetattr(fd, termios.TCSANOW, attrs)
    return fd


def _capture(device, prefix, timeout):
    paths = {
        name: f"{prefix}.{name}.partial"
        for name in ("uart-frames.bin", "main.bin", "oob.bin", "interleaved.bin", "ledger.jsonl")
    }
    handles = {name: open(path, "xb") for name, path in paths.items()}
    fd = _configure_uart(device)
    deadline = time.monotonic() + timeout
    try:
        scan = bytearray()
        while not scan.endswith(MAGIC):
            scan.extend(_read_exact(fd, 1, deadline))
            if len(scan) > len(MAGIC):
                del scan[0]
        for page in range(PAGE_COUNT):
            if page != 0 and _read_exact(fd, len(MAGIC), deadline) != MAGIC:
                raise ValueError(f"page {page}: bad magic")
            raw = MAGIC + _read_exact(fd, FRAME_SIZE - len(MAGIC), deadline)
            frame = _decode_frame(raw, page)
            handles["uart-frames.bin"].write(raw)
            handles["main.bin"].write(frame.main)
            handles["oob.bin"].write(frame.oob)
            handles["interleaved.bin"].write(frame.main + frame.oob)
            row = json.dumps({
                "sequence": frame.sequence,
                "page": frame.page,
                "main_bytes": len(frame.main),
                "oob_bytes": len(frame.oob),
                "crc32": f"{frame.crc32:08x}",
            }, sort_keys=True)
            handles["ledger.jsonl"].write((row + "\n").encode())
            print(f"FRAME_OK sequence={page} page={page} crc=0x{frame.crc32:08x}", flush=True)
    finally:
        os.close(fd)
        for handle in handles.values():
            handle.close()
    for name, partial in paths.items():
        os.replace(partial, f"{prefix}.{name}")


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("device")
    parser.add_argument("output_prefix")
    parser.add_argument("--timeout", type=int, default=180)
    args = parser.parse_args()
    _capture(args.device, args.output_prefix, args.timeout)


if __name__ == "__main__":
    main()
