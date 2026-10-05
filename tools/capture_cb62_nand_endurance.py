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
ALLOWED_TIERS = (1024, 4096, 8192)

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
    version, sequence, page, count, status, main_len, oob_len, crc = struct.unpack("<8I", raw[8:HEADER_SIZE])
    if (version, sequence, page, count) != (1, expected_page, expected_page, 1):
        raise ValueError(f"page {expected_page}: identity mismatch")
    signed_status = status if status < 0x80000000 else status - 0x100000000
    if signed_status != 0:
        raise ValueError(f"page {expected_page}: read status {signed_status}")
    if (main_len, oob_len) != (MAIN_SIZE, OOB_SIZE):
        raise ValueError(f"page {expected_page}: length mismatch")
    data = raw[HEADER_SIZE:]
    if (zlib.crc32(data) & 0xffffffff) != crc:
        raise ValueError(f"page {expected_page}: CRC mismatch")
    return Frame(sequence, page, data[:MAIN_SIZE], data[MAIN_SIZE:], crc)

def validate_tier(raw, page_count):
    if page_count not in ALLOWED_TIERS:
        raise ValueError("non-allowlisted endurance tier")
    if len(raw) != page_count * FRAME_SIZE:
        raise ValueError(f"tier size {len(raw)} != {page_count * FRAME_SIZE}")
    return [_decode_frame(raw[p*FRAME_SIZE:(p+1)*FRAME_SIZE], p) for p in range(page_count)]

def _read_exact(fd, length, deadline, wire=None):
    data = bytearray()
    while len(data) < length:
        remaining = deadline - time.monotonic()
        if remaining <= 0 or not select.select([fd], [], [], remaining)[0]:
            raise TimeoutError(f"received {len(data)} of {length} bytes")
        chunk = os.read(fd, length - len(data))
        if not chunk:
            raise EOFError("serial device returned EOF")
        data.extend(chunk)
        if wire is not None:
            wire.write(chunk)
    return bytes(data)

def _configure_uart(device):
    fd = os.open(device, os.O_RDONLY | os.O_NOCTTY | os.O_NONBLOCK)
    attrs = termios.tcgetattr(fd)
    attrs[0] = attrs[1] = attrs[3] = 0
    attrs[2] = termios.CS8 | termios.CREAD | termios.CLOCAL
    attrs[4] = attrs[5] = termios.B115200
    attrs[6][termios.VMIN] = 1; attrs[6][termios.VTIME] = 0
    termios.tcsetattr(fd, termios.TCSANOW, attrs)
    return fd

def _capture(device, prefix, page_count, timeout):
    if page_count not in ALLOWED_TIERS:
        raise ValueError("non-allowlisted endurance tier")
    names = ("wire.bin", "uart-frames.bin", "main.bin", "oob.bin", "interleaved.bin", "ledger.jsonl")
    paths = {name: f"{prefix}.{name}.partial" for name in names}
    handles = {name: open(path, "xb") for name, path in paths.items()}
    fd = _configure_uart(device); deadline = time.monotonic() + timeout
    try:
        scan = bytearray()
        while not scan.endswith(MAGIC):
            scan.extend(_read_exact(fd, 1, deadline, handles["wire.bin"]))
            if len(scan) > len(MAGIC): del scan[0]
        for page in range(page_count):
            if page and _read_exact(fd, len(MAGIC), deadline, handles["wire.bin"]) != MAGIC:
                raise ValueError(f"page {page}: bad magic")
            raw = MAGIC + _read_exact(fd, FRAME_SIZE-len(MAGIC), deadline,
                                      handles["wire.bin"])
            frame = _decode_frame(raw, page)
            handles["uart-frames.bin"].write(raw)
            handles["main.bin"].write(frame.main)
            handles["oob.bin"].write(frame.oob)
            handles["interleaved.bin"].write(frame.main + frame.oob)
            row = json.dumps({"sequence":frame.sequence,"page":frame.page,"main_bytes":MAIN_SIZE,
                              "oob_bytes":OOB_SIZE,"crc32":f"{frame.crc32:08x}"}, sort_keys=True)
            handles["ledger.jsonl"].write((row+"\n").encode())
            print(f"FRAME_OK sequence={page} page={page} crc=0x{frame.crc32:08x}", flush=True)
            if (page + 1) % 64 == 0:
                for handle in handles.values(): handle.flush(); os.fsync(handle.fileno())
    finally:
        os.close(fd)
        for handle in handles.values(): handle.close()
    for name, partial in paths.items(): os.replace(partial, f"{prefix}.{name}")

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("tier", type=int, choices=ALLOWED_TIERS)
    parser.add_argument("device")
    parser.add_argument("output_prefix")
    parser.add_argument("--timeout", type=int, default=3600)
    args = parser.parse_args()
    _capture(args.device, args.output_prefix, args.tier, args.timeout)

if __name__ == "__main__": main()
