# Verkracked Part 10 public release notes

This source-only release packages the independently written CB62 recovery and
firmware-acquisition utilities used during the Verkracked research project.

## Supported target profiles

- `cb62-original-resident` targets the resident function map validated during
  the original CB62 acquisition.
- `cb62-specimen2-2gb-bld0` targets the separately bootstrapped 2 GiB
  RAM-resident BLD validated during the second CB62 acquisition.

The common USB envelope and upload sequence were observed on both targets.
Their DRAM setup, resident function addresses, UART configuration, NAND
geometry, and boot behavior remain board- and firmware-specific. CV22 identity
alone does not establish compatibility.

## Included

- C11/libusb host utilities
- freestanding AArch64 read-only payload source
- exact UART capture and validation tools
- synthetic tests and fake USB transports
- selected protocol and safety audits

## Excluded

- vendor firmware and DRAM initialization assets
- device credentials, private captures, and customer data
- compiled binaries, object files, and debug-symbol trees
- NAND erase/program, repair, terminal, shell, or persistence payloads

Build and run all source tests with:

```sh
make clean test
```

Use these tools only with hardware you own or are explicitly authorized to
assess. Keep physical-device work isolated from production infrastructure.

No license is currently provided.
