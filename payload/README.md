# CB62 fixed page-0 NAND proof payload

This freestanding AArch64 payload is linked at `0x02000000`. It makes one
fixed call to the acquired CB62 resident AMBOOT `nand_read_pages` entry at
`0x01411a60`, requesting page 0 only. Main and OOB buffers are fixed at
`0x02100000` and `0x02100800`. It emits a 40-byte little-endian frame header,
2,048 main bytes, 128 OOB bytes, and an IEEE CRC-32 through initialized UART0,
then waits forever. It contains no storage-write operation or caller input.

## Fixed block-0 proof

`cb62_nand_block0.c` uses the same fixed code and buffer addresses, then reads
pages 0 through 63 exactly once each. It emits one independently checksummed
frame per page with sequence equal to page number. A failed page emits its
status and stops the loop. The payload contains no caller-controlled range and
no second attempt. `tools/capture_cb62_nand_block0.py` requires all 64 frames
in exact order and produces separate main, OOB, interleaved, raw-frame, and
JSONL-ledger files while retaining `.partial` files after interruption.

The checked-in payload blobs are what the host executables upload. To audit a
source rebuild on a host with Clang, LLVM utilities, and GNU-flavor LLD:

```sh
clang --target=aarch64-none-elf -fuse-ld=lld -Os -ffreestanding \
  -fno-builtin -fno-stack-protector -fno-pic -fno-pie -nostdlib \
  -Wl,-T,payload/cb62_nand_block0.ld -Wl,--build-id=none \
  payload/cb62_nand_block0.c -o build/payload/cb62_nand_block0.elf
llvm-objcopy -O binary build/payload/cb62_nand_block0.elf \
  build/payload/cb62_nand_block0.bin
llvm-objdump -d --no-show-raw-insn --print-imm-hex \
  build/payload/cb62_nand_block0.elf \
  > build/payload/cb62_nand_block0.objdump.txt
```

The audited local block-0 build is 520 bytes with SHA-256
`c2127b878cf883a37afcc71dd3640fdbfb79a5c8f166c368e182c9f19295722e`.
Toolchain-version differences can alter code generation, so compare rebuilt
bytes and disassembly before replacing the checked-in blob.
