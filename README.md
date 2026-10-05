# Verkracked Part 10 — Verkada CB62 Firmware Dumper

This repository is Part 10 of the Verkracked Security Research Project by
[Jon “GainSec” Gaines](https://gainsec.com). It provides deliberately narrow,
read-only firmware-acquisition and recovery-evidence tools for lawfully owned
or explicitly authorized Verkada CB62 hardware.

The project is intended for owner-controlled interoperability, preservation,
repair research, and security assessment in an isolated laboratory. It is not
intended to access Verkada infrastructure, accounts, or devices without
authorization. Verkada is a trademark of its owner; this independent project
is not affiliated with or endorsed by Verkada.

## Related Verkracked releases

| Part | Project | Links |
| --- | --- | --- |
| Part 0 | Introduction to the Verkracked research project | [GainSec article](https://gainsec.com/2026/09/06/verkracked-security-research-on-verkada-anti-crime-devices-part-0/) |
| Part 4 | Alarm-hub local cloud framework | [GitHub](https://github.com/GainSec/verkada-verkracked-alarm-hub-local-framework) · [Parts 4 & 5 article](https://gainsec.com/2026/09/06/verkracked-parts-4-5-local-cloud-and-sub-ghz-frameworks-for-verkada-alarm-hubs/) |
| Part 5 | Alarm-hub Sub-GHz interoperability framework | [GitHub](https://github.com/GainSec/verkada-verkracked-subghz-framework) · [Parts 4 & 5 article](https://gainsec.com/2026/09/06/verkracked-parts-4-5-local-cloud-and-sub-ghz-frameworks-for-verkada-alarm-hubs/) |
| Part 7B | CB62 local cloud emulator | [GitHub](https://github.com/GainSec/verkada-verkracked-bullet-cam-cloud-emulator) |
| Part 8 | VerkEye CB62 model runtime | [GitHub](https://github.com/GainSec/VerkEye) · [GainSec article](https://gainsec.com/2026/10/04/verkracked-part-8-verkeye/) |

Parts without linked public material remain subject to their disclosure and
publication schedules.

## Public release boundary

This is a source-only release. It contains independently written host code,
read-only payload source, receiver tools, tests, and selected protocol audits.
It does not include vendor firmware, DRAM initialization data, credentials,
private captures, terminal or shell payloads, or any repair/write payload.
Generated binaries and debug-symbol trees are excluded from version control.

No license is currently provided. Review the source and applicable law before
using or redistributing it.

See [Public release notes](PUBLIC_RELEASE_NOTES.md) for the exact release
boundary and validation summary.

This repository contains deliberately narrow libusb utilities for a
Verkada CB62-TE waiting in ROM USB-download mode:
`cb62-inquiry-probe` identifies the Ambarella silicon,
`cb62-inquiry-addr-probe` reports the recovery address, and
`cb62-amboot-header-probe` performs one fixed 32-bit read at `0x01400038`.
`cb62-amboot-header-block-probe` performs one fixed, CRC-checked 64-byte read
from `0x01400000`; it accepts no device address or length, refuses to overwrite
its local output, sends no retries, and issues no trailing command. Its wire
and safety analysis is in
`docs/plans/2026-09-08-cb62-fixed-header-block-read-audit.md`.
`cb62-amboot-resident-bld-probe` performs the same two-stage read once for the
fixed `0x34000`-byte known resident-image mapping at `0x01400000`. It has no
caller-controlled target range; details are in
`docs/plans/2026-09-08-cb62-fixed-resident-bld-read-audit.md`.
`cb62-amboot-ptb-snapshot-probe` uses only fixed command-2 selector `16`, then
command `3`, to retrieve the resident bootloader's fixed 4 KiB in-memory
partition-table snapshot. It does not provide an address or storage selector
and does not read NAND directly. Its static basis and safety analysis are in
`docs/plans/2026-09-08-cb62-fixed-ptb-snapshot-read-audit.md`.
The repository also contains fixed NAND-read experiments. The verified
page-0 utility uploads one 1,200-byte AArch64 payload to fixed RAM address
`0x02000000`, issues one fixed jump, and returns one 2,048-byte main area plus
128-byte OOB area over UART. The separately gated block-0 utility uses a fixed
520-byte payload to read pages 0 through 63 once each and emit 64 independently
sequenced and checksummed UART frames. Three endurance utilities use the same
read-only design with compile-time bounds of exactly 1,024, 4,096, or 8,192
pages. A separate fixed-range build target supports two explicitly reviewed
CB62 resident-code profiles. Its page range and profile are compiled into the
executable and repeated in the required execution token; the executable
accepts no runtime address, page range, payload, operation selector, or retry
count. These are acquisition tools, not flashers.

A fixed page-110 diagnostic is also included. It accepts no page argument and
reads only physical `(block 1, page 46)` so a failed long-stream frame can be
isolated without broadening the host interface.

## What is shared, and what is target-specific

The USB command envelope, `4255:0001` AMBOOT identity, upload address
`0x02000000`, CRC, and upload/jump sequence were observed on both assessed
CB62 specimens. Those pieces are consistent with an Ambarella CV22 recovery
implementation. They are not proof that every CV22 board is compatible.

The DRAM bootstrap, resident function addresses, UART setup, NAND geometry,
and boot-chain behavior are target- and firmware-specific. The fixed-range
reader therefore exposes only these reviewed profiles:

| Build profile | Required precondition | NAND read | UART init | Baud | Validation |
| --- | --- | --- | --- | --- | --- |
| `cb62-original-resident` | Original specimen already at AMBOOT `4255:0001` | `0x01411a60` | `0x0140ed70` | 1,000,000 | Original CB62 acquisition |
| `cb62-specimen2-2gb-bld0` | Specimen 2 bootstrapped with its external `LINUX.CV22.LPDDR4.2GB.NAND` profile, then at AMBOOT `4255:0001` | `0x00013320` | `0x00011000` | 115,200 | Complete 131,072-page acquisition |

The second name describes the tested specimen and RAM-resident BLD, not a
confirmed retail hardware revision. Its runtime metadata still reported
`CB62-512TE-HW`. This repository does not include or redistribute either DRAM
initialization profile, vendor firmware, terminal payload, or repair/write
payload.

The tool is mechanically independent of AmbaUSB-derived clients and does not
link or redistribute their code, binaries, board profiles, DRAM initialization
data, or firmware.

## Verified hardware result

Both assessed CB62-TE recovery environments returned:

```text
signature: 0x55525350 (PSRU on the wire)
status:    0
chip ID:   0x000055f0 / 22000
platform:  Ambarella CV22
```

The captured experiment contained exactly one 32-byte bulk OUT request and one
16-byte bulk IN response. No subsequent transaction was sent.

The separately captured address inquiry returned `0x01400000`, exactly matching
the boot log's `BL31: Non Secure code at 0x1400000` value. This correlation does
not establish that arbitrary access or execution at that address is safe.

Static analysis of the recovered CV22 BLD images found the little-endian value
`0x00000309` at load offset `0x38`. The third utility encodes only a read of
`0x01400000 + 0x38 = 0x01400038`. The captured camera response returned the
same value with status zero.

The fixed page-0 payload subsequently called the recovered resident vendor
function at `0x01411a60` as
`nand_read_pages(0, 0, 1, 0x02100000, 0x02100800, 0)`. Its UART frame passed an
independent CRC-32 check (`0x52ea3610`) and yielded exact main and OOB images.
A fresh-recovery repeat matched byte-for-byte, and the 64-page stage then
completed with every frame ordered and CRC-valid. The first 1,024-page
endurance attempt reproduced those 64 pages, then failed closed at linear page
64 because the original payload passed it as a page-within-block value. Live
RAM geometry and resident disassembly establish the ABI as
`(block, page_in_block, count, main, spare, mode)`, with 64 pages per block.
The corrected payload maps a linear page to `block = page / 64` and
`page_in_block = page % 64`. Two hardware runs validated identical pages
0–109. The raw-wire repeat showed normal BL2/BL31 console output taking over
UART at page-110 data offset 270. The fixed page-110 diagnostic then returned
the complete page with matching target/host CRC `0xbb64c67c`. This establishes
a recovery/secure-boot takeover deadline, not a bad NAND page or ABI failure.

## Safety model

The inquiry executables accept only `--help` and `--execute-one-inquiry`. The
fixed header probe accepts only `--help` and `--execute-one-read`. Before
opening the bulk interface each requires exactly one device matching every
invariant:

| Property | Required value |
| --- | --- |
| USB VID:PID | `4255:0001` |
| Manufacturer | `AMBA` |
| Product | `Ambarella USB generic class` |
| Matching interfaces | exactly one |
| Interface class/subclass/protocol | `ff/ff/00` |
| Bulk endpoints | exactly two |
| Bulk OUT / IN | `0x01` / `0x81` |

After those checks the inquiry and word probes each perform one fixed OUT/IN
exchange. The block readers perform exactly command 2/response/command 3/data,
validate the advertised CRC-32, release the interface, and exit. The NAND
tools add only a compile-time embedded payload, fixed RAM address, and one
fixed jump. The fixed proof binaries have no caller-provided code, address,
page range, operation, or automatic retry. The range tool accepts its profile
and page range only at build time; the resulting executable requires an exact
profile-and-range token. Their payloads contain no NAND erase, program,
spare-write, bad-block-marking, or partition-update function. The memory
encoders expose no caller address or length, and the PTB encoder exposes no
caller selector. If a kernel driver owns the interface, the backend refuses to
detach it.

This is still an active vendor-specific USB transaction. Review the source and
capture traffic before using it against another device or firmware generation.

## Exact protocol exchange

The request is eight little-endian 32-bit words:

```text
0x55434d44  command token (wire bytes spell DMCU)
0x00000004  INQUIRY_STATUS command
0x00000001  INQUIRY_CHIP parameter
0x00000000  remaining parameter
0x00000000  remaining parameter
0x00000000  remaining parameter
0x00000000  remaining parameter
0x00000000  remaining parameter
```

Exact request bytes:

```text
44 4d 43 55 04 00 00 00 01 00 00 00 00 00 00 00
00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
```

The response consists of a little-endian status token, status, chip ID, and
final parameter. The verified response was:

```text
50 53 52 55 00 00 00 00 f0 55 00 00 00 00 00 00
```

`cb62-inquiry-addr-probe` changes only request word 2 from `1` to `2`. Its
verified request and response were:

```text
44 4d 43 55 04 00 00 00 02 00 00 00 00 00 00 00
00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00

50 53 52 55 00 00 00 00 00 00 40 01 00 00 00 00
```

`cb62-amboot-header-probe` uses ready-to-receive command `0`, flag `0x08`,
request type `1`, and the fixed address `0x01400038`:

```text
44 4d 43 55 00 00 00 00 08 00 00 00 01 00 00 00
38 00 40 01 00 00 00 00 00 00 00 00 00 00 00 00
```

It accepts only a 16-byte response with signature `0x55525350` and status zero.
The expected and captured third word is `0x00000309`:

```text
50 53 52 55 00 00 00 00 09 03 00 00 00 00 00 00
```

## Dependencies

- C11 compiler
- `make`
- `pkg-config`
- libusb 1.0 development headers and library
- Python 3 for fixed UART-frame validation
- Clang plus either LLD or GNU AArch64 binutils when rebuilding the AArch64 payload
- LLVM `objcopy` (versioned names are detected) or GNU AArch64 `objcopy`

Debian, Ubuntu, or Kali:

```sh
sudo apt update
sudo apt install build-essential clang binutils-aarch64-linux-gnu \
  llvm pkg-config libusb-1.0-0-dev
```

macOS with Homebrew:

```sh
brew install libusb pkg-config
```

## Compile and test

Build all host executables under `out/`:

```sh
make
```

Perform a clean build and run every test:

```sh
make clean test
```

Expected final test output:

```text
test_protocol: PASS
test_probe: PASS
test_addr_protocol: PASS
test_addr_probe: PASS
test_memory_protocol: PASS
test_memory_probe: PASS
test_block_protocol: PASS
test_block_probe: PASS
test_resident_protocol: PASS
test_resident_probe: PASS
test_ptb_protocol: PASS
test_ptb_probe: PASS
test_nand_protocol: PASS
test_nand_probe: PASS
test_nand_block0_probe: PASS
test_nand_block0_payload: PASS
test_nand_range: PASS
test_capture_nand_block0: PASS
test_cli: PASS
test_addr_cli: PASS
test_memory_cli: PASS
test_block_cli: PASS
test_resident_cli: PASS
test_ptb_cli: PASS
test_nand_block0_cli: PASS
test_nand_range_cli: PASS
```

The tests cover exact wire serialization, response validation, device identity,
interface topology, transfer counts, failure paths, and CLI rejection of
firmware-like or additional arguments. The block-0 tests additionally require
fixed pages 0–63 and reject gaps, truncation, trailing data, and CRC damage.
The range test verifies both address profiles, compile-time bounds, exact frame
identity, and CRC rejection.

Build output under `build/` and `out/` is generated locally and excluded from
version control. Rebuild it for the target host.

## Put the camera into recovery mode

The assessed unit entered recovery using this sequence:

1. Connect PoE and the Micro-USB service connection.
2. Hold the on-board `SW2` tactile button while applying power.
3. Release it after UART reports the AmbaUSB path.
4. Confirm UART stops at `entering usb_task`.
5. Confirm USB identity and descriptors independently.

Observed UART tail:

```text
Flash via ambausb set
Entered USB download mode (4)...
init_usb_dev done
init_usb_hardware done
entering usb_task
```

Linux descriptor check:

```sh
lsusb -d 4255:0001 -v
```

Some host USB-ID databases label VID `4255` as GoPro. The device-provided
manufacturer is `AMBA`; the database label is not product identity.

## Execute exactly one inquiry

Start UART logging and a raw USB capture first. Then run:

```sh
sudo ./out/cb62-inquiry-probe --execute-one-inquiry
```

To perform only the fixed address inquiry instead:

```sh
sudo ./out/cb62-inquiry-addr-probe --execute-one-inquiry
```

The fixed header read must be treated as a separate experiment. Start UART and
USB capture, revalidate the exact recovery identity, and then run it once:

```sh
sudo ./out/cb62-amboot-header-probe --execute-one-read
```

The resident BLD read is a separate experiment with a separate execution
token. Its destination must not exist:

```sh
sudo ./out/cb62-amboot-resident-bld-probe \
  --execute-one-fixed-resident-bld-read --output NEW-FILE.bin
```

It reads resident RAM only; it does not access NAND or upload or execute a
loader.

The PTB snapshot read is also a separate, fixed experiment. Its destination
must not exist:

```sh
sudo ./out/cb62-amboot-ptb-snapshot-probe \
  --execute-one-fixed-ptb-snapshot-read --output NEW-FILE.bin
```

This invokes only selector `16`, which static analysis of the resident stack
BLD shows serializes a 4 KiB structure from its existing `.bss.noinit` state.
It does not invoke the NAND page-read routine found only in the separate NAND
BLD. Do not confuse this metadata snapshot with a firmware dump.

Never automatically retry it. A transport failure, malformed response, or
unexpected value is a hard stop for evidence review.

## Execute the fixed NAND proofs

These commands are storage-write-capability-free but execute code from RAM.
Always arm raw UART and USB capture first. A completed jump replaces the USB
recovery task, so return the camera to fresh SW2 recovery before another run.

The already verified page-0 operation is:

```sh
sudo ./out/cb62-nand-page0-uart-proof \
  --execute-fixed-page0-uart-proof
```

To reproduce only the fixed page-110 diagnostic, arm the one-frame receiver
before executing its separately gated uploader:

```sh
python3 tools/capture_cb62_nand_single.py \
  /dev/ttyUSB0 cb62-page110 110

sudo ./out/cb62-nand-page110-uart-proof \
  --execute-fixed-page110-uart-proof
```

The payload calls `nand_read_pages(1, 46, 1, main, oob, 0)` exactly once and
reports linear page/sequence 110. It then enters a terminal `wfe` loop.

Do not repeat it merely because a transport result is uncertain; inspect the
capture first. The 64-page operation must not run until a fresh-recovery
page-0 comparison matches both previously recorded SHA-256 values. Once that
gate passes, arm the receiver before running the uploader:

```sh
python3 tools/capture_cb62_nand_block0.py \
  /dev/ttyUSB0 cb62-block0

sudo ./out/cb62-nand-block0-uart-proof \
  --execute-fixed-block0-uart-proof
```

The receiver requires exactly 64 frames with sequence/page values 0 through
63, one page per frame, zero status, 2,048 main bytes, 128 OOB bytes, and a
matching CRC-32. It produces `.main.bin`, `.oob.bin`, `.interleaved.bin`,
`.uart-frames.bin`, and `.ledger.jsonl`. Interrupted captures retain `.partial`
files and are never represented as complete.

The endurance ladder is deliberately fixed to three compiled tiers. Start
with 1,024 pages; do not skip directly to a longer tier. Arm the exact-tier
receiver first, then issue the matching uploader token once:

```sh
python3 tools/capture_cb62_nand_endurance.py 1024 \
  /dev/ttyUSB0 cb62-endurance-1024

sudo ./out/cb62-nand-endurance-1024 \
  --execute-fixed-endurance-1024
```

The 4,096- and 8,192-page binaries require their correspondingly numbered
token and receiver tier. A failed, truncated, corrupt, out-of-order, or
nonzero-status run remains `.partial` and is not automatically retried. The
longest tier must succeed twice from separate fresh recovery boots before it
is used to size later full-NAND shards.

The receiver also writes `.wire.bin` before frame decoding. On any validation
failure this remains `.wire.bin.partial`, preserving the offending bytes for
bit-level analysis even when the validated frame/main/OOB outputs stop at the
last good page.

## Build a reviewed fixed range

Build one range only after independently establishing which resident BLD is in
RAM. For specimen 2, the initial FORCEUSB device is `4255:0013`; a compatible
2 GiB DRAM bootstrap must first produce the exact `4255:0001` identity required
by this repository. That bootstrap is intentionally out of scope here.

For example, build pages 5,237 through 131,071 for the second assessed
specimen:

```sh
make nand-range \
  NAND_RANGE_PROFILE=cb62-specimen2-2gb-bld0 \
  NAND_RANGE_START=5237 \
  NAND_RANGE_COUNT=125835
```

Arm the receiver before executing the resulting uploader:

```sh
python3 tools/capture_cb62_nand_range.py \
  5237 125835 /dev/ttyUSB0 cb62-specimen2-range-5237 --baud 115200

sudo ./out/cb62-nand-range-cb62-specimen2-2gb-bld0-5237-125835 \
  --execute-fixed-range-cb62-specimen2-2gb-bld0-5237-125835
```

The host validates the exact AMBOOT USB descriptors, embeds only the compiled
payload, performs one upload and one jump, and never retries. The receiver
requires every page and sequence number in order, validates the per-frame CRC,
uses `.partial` files until completion, and preserves main, OOB, interleaved,
raw-wire, and JSONL-ledger outputs separately.

Use `--baud 1000000` with `cb62-original-resident`. Some hosts, including
macOS termios, do not expose a 1,000,000-baud constant; the receiver fails
before opening output files on those hosts. Do not substitute a nearby baud
rate.

The assessed hardware execution returned:

```text
decoded: signature=0x55525350 status=0 value=0x00000309 p1=0x00000000 expected=0x00000309
result: fixed AMBOOT header read matched expected value; no further transaction sent
```

Raw usbmon frames 1599–1602 contain exactly one bulk OUT
submission/completion and one bulk IN submission/completion. The camera stayed
enumerated in recovery and produced no new UART output.

Root may be unnecessary if a narrowly scoped temporary udev policy grants
access. Do not weaken persistent USB permissions merely to run the probe.

Successful output ends with:

```text
decoded: signature=0x55525350 status=0 chip_id=22000 (0x000055f0) p1=0x00000000
result: CV22 inquiry matched expected chip ID; no further transaction sent
```

| Exit | Meaning |
| --- | --- |
| `0` | Valid response containing the expected CV22 ID |
| `1` | Discovery, identity, topology, USB, or response failure |
| `2` | Unsupported command-line invocation |
| `3` | Valid response containing an unexpected chip ID |

## Capture the transaction on Linux

Example using `usbmon` and Wireshark's capture utility:

```sh
sudo modprobe usbmon
sudo dumpcap -i usbmon1 -w cb62-inquiry.pcapng
```

Run the probe once, stop the capture, hash it, and filter the exact target
bus/address printed by the probe. Bus/address values can change after reconnect.
The only target bulk data should be the single request and response documented
above.

## Repository layout

```text
Makefile                  build, clean, and test targets
src/protocol.[ch]         request encoding and response decoding
src/probe.[ch]            safety gates and one-shot transaction
src/addr_protocol.[ch]    fixed address-inquiry encoding/decoding
src/addr_probe.[ch]       separate one-shot address transaction
src/libusb_backend.[ch]   USB discovery and libusb implementation
src/main.c                CLI and transcript output
src/addr_main.c           address-inquiry CLI and transcript output
src/memory_protocol.[ch]  fixed 0x01400038 read encoding/decoding
src/memory_probe.[ch]     one-shot fixed-read safety state machine
src/memory_main.c         fixed-read CLI and transcript output
src/block_*               fixed 64-byte block-read implementation and CLI
src/resident_*            fixed 0x34000 resident-image reader and CLI
src/ptb_*                 fixed selector-16 4 KiB snapshot reader and CLI
src/nand_*                fixed page-0 payload/uploader/protocol
src/nand_block0_*         fixed pages-0-through-63 payload/uploader
src/nand_endurance_*      fixed 1024/4096/8192-page uploaders
src/nand_range_main.c     exact profile/range execution gate
src/nand_page110_main.c   fixed page-110 diagnostic host gate
payload/cb62_nand_range.c profile-bound, read-only range payload
payload/                  other freestanding AArch64 source and linker scripts
tools/capture_cb62_nand_block0.py  exact 64-frame UART receiver
tools/capture_cb62_nand_endurance.py exact-tier endurance receiver
tools/capture_cb62_nand_single.py exact one-frame diagnostic receiver
tools/capture_cb62_nand_range.py exact fixed-range UART receiver
tests/fake_libusb.[ch]    isolated fake transport
tests/test_protocol.c     wire-format tests
tests/test_probe.c        safety and transaction tests
tests/test_cli.sh         command-line allowlist tests
tests/test_memory_*       fixed-read protocol, transport, and CLI tests
tests/test_block_*        fixed block protocol, transport, and CLI tests
tests/test_resident_*     resident-image protocol, transport, and CLI tests
tests/test_ptb_*          PTB snapshot protocol, transport, and CLI tests
tests/test_nand_*         fixed page-0 and block-0 safety/transport tests
tests/test_nand_endurance_* fixed-tier payload/uploader/receiver tests
tests/test_payload_toolchain.sh cross-platform payload-tool discovery test
tests/run-tests.sh        test wrapper
build/                    generated test/payload output (ignored)
out/                      generated host executables (ignored)
```

## Limitations

- The hard-coded identity describes the observed CB62-TE recovery ROM.
- CV22 identification does not prove a board, DRAM profile, resident ABI, or
  NAND geometry is safe. Do not use these profiles on an unverified CV22 target.
- The successful page-0 operation and gated block-0 operation read raw NAND;
  they do not expose a console or bypass the signed/encrypted boot chain. They
  do not access OTP, EEPROM, or microSD storage.
- The memory tools are limited in source to the fixed word at `0x01400038`, the
  fixed 64-byte header, and the fixed `0x34000` resident-image mapping at
  `0x01400000`. There is no address or length input.
- The selector-16 tool retrieves only the fixed 4 KiB metadata snapshot already
  materialized by the resident bootloader; it has no storage-address input and
  is not a NAND acquisition primitive.
- The NAND tools contain one fixed loader-upload sequence and one fixed jump.
  No caller-supplied loader or runtime address/range is possible. The range is
  chosen at compile time and encoded into both the filename and execution
  token. No erase, program, spare-write, bad-block-marking, partition update,
  or automatic retry is present in the host utilities or payloads.
- Any changed identity, topology, endpoint set, or chip ID is intentionally
  rejected and requires source review rather than a runtime override.

## Provenance and future work

The wire format was cross-checked against independently available Ambarella
protocol implementations before the hardware experiment. This implementation
was written separately around the minimum inquiry and libusb API surface. Keep
future NAND acquisition or higher-risk loader work separate so these fixed
evidence utilities retain their narrow, auditable boundaries.
