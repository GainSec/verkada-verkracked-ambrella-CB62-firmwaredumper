#!/usr/bin/env python3
import argparse
import os
import struct
import zlib

from capture_cb62_nand_endurance import MAGIC, FRAME_SIZE, _configure_uart, _read_exact


def capture(device, output, expected_page, timeout):
    import time
    fd = _configure_uart(device)
    deadline = time.monotonic() + timeout
    wire_path = output + ".wire.partial"
    frame_path = output + ".frame.partial"
    with open(wire_path, "xb") as wire:
        try:
            scan = bytearray()
            while not scan.endswith(MAGIC):
                scan.extend(_read_exact(fd, 1, deadline, wire))
                if len(scan) > len(MAGIC):
                    del scan[0]
            raw = MAGIC + _read_exact(fd, FRAME_SIZE - len(MAGIC), deadline, wire)
        finally:
            os.close(fd)
    version, sequence, page, count, status, main_len, oob_len, declared = struct.unpack(
        "<8I", raw[8:40]
    )
    actual = zlib.crc32(raw[40:]) & 0xffffffff
    with open(frame_path, "xb") as frame:
        frame.write(raw)
    print(f"FRAME page={page} sequence={sequence} status={status} bytes={len(raw)} "
          f"declared_crc=0x{declared:08x} actual_crc=0x{actual:08x}", flush=True)
    if (version, sequence, page, count, status, main_len, oob_len) != (
        1, expected_page, expected_page, 1, 0, 2048, 128
    ):
        raise ValueError("unexpected fixed-frame identity/status/length")
    if actual != declared:
        raise ValueError("CRC mismatch")
    os.replace(wire_path, output + ".wire")
    os.replace(frame_path, output + ".frame")


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("device")
    parser.add_argument("output")
    parser.add_argument("expected_page", type=int)
    parser.add_argument("--timeout", type=int, default=180)
    args = parser.parse_args()
    capture(args.device, args.output, args.expected_page, args.timeout)


if __name__ == "__main__":
    main()
