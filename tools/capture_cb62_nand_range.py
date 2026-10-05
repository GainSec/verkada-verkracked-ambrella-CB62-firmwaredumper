#!/usr/bin/env python3
import argparse
import dataclasses
import json
import os
import select
import struct
import termios
import zlib

MAGIC = b"CB62NAND"
HEADER_SIZE = 40
MAIN_SIZE = 2048
OOB_SIZE = 128
FRAME_SIZE = HEADER_SIZE + MAIN_SIZE + OOB_SIZE
TOTAL_NAND_PAGES = 131072


@dataclasses.dataclass(frozen=True)
class Frame:
    sequence: int
    page: int
    main: bytes
    oob: bytes
    crc32: int


def _check_range(start_page, page_count):
    if (start_page < 0 or page_count <= 0 or
            start_page + page_count > TOTAL_NAND_PAGES):
        raise ValueError("fixed range outside NAND geometry")


def _decode_frame(raw, expected_sequence, expected_page):
    if len(raw) != FRAME_SIZE:
        raise ValueError(f"page {expected_page}: frame size mismatch")
    if raw[:8] != MAGIC:
        raise ValueError(f"page {expected_page}: bad magic")
    fields = struct.unpack("<8I", raw[8:HEADER_SIZE])
    version, sequence, page, count, status, main_len, oob_len, crc = fields
    if (version, sequence, page, count) != (
            1, expected_sequence, expected_page, 1):
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


def validate_range(raw, start_page, page_count):
    _check_range(start_page, page_count)
    if len(raw) != page_count * FRAME_SIZE:
        raise ValueError("range size mismatch")
    return [
        _decode_frame(raw[i * FRAME_SIZE:(i + 1) * FRAME_SIZE],
                      i, start_page + i)
        for i in range(page_count)
    ]


def _read_exact(fd, length, idle_timeout, wire=None):
    data = bytearray()
    while len(data) < length:
        if not select.select([fd], [], [], idle_timeout)[0]:
            raise TimeoutError(f"received {len(data)} of {length} bytes")
        chunk = os.read(fd, length - len(data))
        if not chunk:
            raise EOFError("serial device returned EOF")
        data.extend(chunk)
        if wire is not None:
            wire.write(chunk)
    return bytes(data)


def _configure_uart(device, baud):
    speeds = {115200: termios.B115200}
    if hasattr(termios, "B1000000"):
        speeds[1000000] = termios.B1000000
    if baud not in speeds:
        raise ValueError(
            f"baud {baud} is not supported by this host's termios; "
            "use a host/adapter that exposes the exact target rate"
        )
    fd = os.open(device, os.O_RDONLY | os.O_NOCTTY | os.O_NONBLOCK)
    attrs = termios.tcgetattr(fd)
    attrs[0] = attrs[1] = attrs[3] = 0
    attrs[2] = termios.CS8 | termios.CREAD | termios.CLOCAL
    attrs[4] = attrs[5] = speeds[baud]
    attrs[6][termios.VMIN], attrs[6][termios.VTIME] = 1, 0
    termios.tcsetattr(fd, termios.TCSANOW, attrs)
    return fd


def _capture(device, prefix, start_page, page_count, timeout, baud):
    _check_range(start_page, page_count)
    fd = _configure_uart(device, baud)
    names = ("wire.bin", "uart-frames.bin", "main.bin", "oob.bin",
             "interleaved.bin", "ledger.jsonl")
    paths = {}
    handles = {}
    try:
        paths = {name: f"{prefix}.{name}.partial" for name in names}
        for name, path in paths.items():
            handles[name] = open(path, "xb")
        scan = bytearray()
        while not scan.endswith(MAGIC):
            scan.extend(_read_exact(fd, 1, timeout, handles["wire.bin"]))
            if len(scan) > len(MAGIC):
                del scan[0]
        for sequence in range(page_count):
            page = start_page + sequence
            if sequence and _read_exact(
                    fd, len(MAGIC), timeout, handles["wire.bin"]) != MAGIC:
                raise ValueError(f"page {page}: bad magic")
            raw = MAGIC + _read_exact(
                fd, FRAME_SIZE - len(MAGIC), timeout, handles["wire.bin"])
            frame = _decode_frame(raw, sequence, page)
            handles["uart-frames.bin"].write(raw)
            handles["main.bin"].write(frame.main)
            handles["oob.bin"].write(frame.oob)
            handles["interleaved.bin"].write(frame.main + frame.oob)
            row = json.dumps({
                "crc32": f"{frame.crc32:08x}",
                "main_bytes": MAIN_SIZE,
                "oob_bytes": OOB_SIZE,
                "page": frame.page,
                "sequence": frame.sequence,
            }, sort_keys=True)
            handles["ledger.jsonl"].write((row + "\n").encode())
            print(f"FRAME_OK sequence={sequence} page={page} "
                  f"crc=0x{frame.crc32:08x}", flush=True)
            if (sequence + 1) % 64 == 0:
                for handle in handles.values():
                    handle.flush()
                    os.fsync(handle.fileno())
    finally:
        os.close(fd)
        for handle in handles.values():
            handle.close()
    for name, partial in paths.items():
        os.replace(partial, f"{prefix}.{name}")


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("start_page", type=int)
    parser.add_argument("page_count", type=int)
    parser.add_argument("device")
    parser.add_argument("output_prefix")
    parser.add_argument("--timeout", type=float, default=120)
    parser.add_argument("--baud", type=int, choices=(115200, 1000000),
                        default=115200)
    args = parser.parse_args()
    _capture(args.device, args.output_prefix, args.start_page,
             args.page_count, args.timeout, args.baud)


if __name__ == "__main__":
    main()
