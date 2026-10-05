CC ?= cc
PKG_CONFIG ?= pkg-config
LLVM_OBJCOPY ?= $(shell command -v llvm-objcopy 2>/dev/null || command -v llvm-objcopy-21 2>/dev/null || command -v llvm-objcopy-20 2>/dev/null || command -v llvm-objcopy-19 2>/dev/null || command -v aarch64-linux-gnu-objcopy 2>/dev/null || printf /opt/homebrew/opt/llvm/bin/llvm-objcopy)
AARCH64_CC ?= clang
AARCH64_LD ?= $(shell command -v ld.lld 2>/dev/null || command -v aarch64-linux-gnu-ld 2>/dev/null)
CFLAGS ?= -O2 -g
CFLAGS += -std=c11 -Wall -Wextra -Werror -pedantic
CPPFLAGS += $(shell $(PKG_CONFIG) --cflags libusb-1.0)
LDLIBS += $(shell $(PKG_CONFIG) --libs libusb-1.0)

BUILD_DIR := build
OUT_DIR := out
BIN := $(OUT_DIR)/cb62-inquiry-probe
ADDR_BIN := $(OUT_DIR)/cb62-inquiry-addr-probe
MEMORY_BIN := $(OUT_DIR)/cb62-amboot-header-probe
BLOCK_BIN := $(OUT_DIR)/cb62-amboot-header-block-probe
RESIDENT_BIN := $(OUT_DIR)/cb62-amboot-resident-bld-probe
PTB_BIN := $(OUT_DIR)/cb62-amboot-ptb-snapshot-probe
NAND_BIN := $(OUT_DIR)/cb62-nand-page0-uart-proof
NAND_PAGE110_BIN := $(OUT_DIR)/cb62-nand-page110-uart-proof
NAND_RANGE_PROFILE ?= cb62-original-resident
NAND_RANGE_START ?= 0
NAND_RANGE_COUNT ?= 1

ifeq ($(NAND_RANGE_PROFILE),cb62-original-resident)
NAND_READ_PAGES_ADDR := 0x01411a60u
UART_INIT_ADDR := 0x0140ed70u
NAND_UART_BAUD := 1000000u
else ifeq ($(NAND_RANGE_PROFILE),cb62-specimen2-2gb-bld0)
NAND_READ_PAGES_ADDR := 0x00013320u
UART_INIT_ADDR := 0x00011000u
NAND_UART_BAUD := 115200u
else
$(error unsupported NAND_RANGE_PROFILE '$(NAND_RANGE_PROFILE)')
endif

NAND_RANGE_BIN := $(OUT_DIR)/cb62-nand-range-$(NAND_RANGE_PROFILE)-$(NAND_RANGE_START)-$(NAND_RANGE_COUNT)
NAND_BLOCK0_BIN := $(OUT_DIR)/cb62-nand-block0-uart-proof
COMMON := src/protocol.c src/probe.c
REAL := $(COMMON) src/libusb_backend.c src/main.c
ADDR_COMMON := src/addr_protocol.c src/addr_probe.c
ADDR_REAL := $(ADDR_COMMON) src/libusb_backend.c src/addr_main.c
MEMORY_COMMON := src/memory_protocol.c src/memory_probe.c
MEMORY_REAL := $(MEMORY_COMMON) src/libusb_backend.c src/memory_main.c
BLOCK_COMMON := src/block_protocol.c src/block_probe.c
BLOCK_REAL := $(BLOCK_COMMON) src/libusb_backend.c src/block_main.c
RESIDENT_COMMON := src/resident_protocol.c src/resident_probe.c
RESIDENT_REAL := $(RESIDENT_COMMON) src/libusb_backend.c src/resident_main.c
PTB_COMMON := src/ptb_protocol.c src/ptb_probe.c
PTB_REAL := $(PTB_COMMON) src/libusb_backend.c src/ptb_main.c
NAND_COMMON := src/nand_protocol.c src/nand_probe.c src/nand_payload_blob.c
NAND_REAL := $(NAND_COMMON) src/libusb_backend.c src/nand_main.c
NAND_BLOCK0_COMMON := src/nand_protocol.c src/nand_block0_probe.c src/nand_block0_payload_blob.c
NAND_BLOCK0_REAL := $(NAND_BLOCK0_COMMON) src/libusb_backend.c src/nand_block0_main.c
ENDURANCE_COUNTS := 1024 4096 8192
ENDURANCE_PAYLOADS := $(foreach count,$(ENDURANCE_COUNTS),$(BUILD_DIR)/payload/cb62_nand_endurance_$(count).bin)
ENDURANCE_BINS := $(foreach count,$(ENDURANCE_COUNTS),$(OUT_DIR)/cb62-nand-endurance-$(count))
ENDURANCE_COMMON := src/nand_protocol.c src/nand_endurance_probe.c

.PHONY: all clean nand-range test

all: $(BIN) $(ADDR_BIN) $(MEMORY_BIN) $(BLOCK_BIN) $(RESIDENT_BIN) $(PTB_BIN) $(NAND_BIN) $(NAND_PAGE110_BIN) $(NAND_BLOCK0_BIN) $(ENDURANCE_BINS)

nand-range: $(NAND_RANGE_BIN)

$(BUILD_DIR)/payload/cb62_nand_range_$(NAND_RANGE_PROFILE)_$(NAND_RANGE_START)_$(NAND_RANGE_COUNT).elf: payload/cb62_nand_range.c payload/cb62_nand_endurance.ld
	mkdir -p $(BUILD_DIR)/payload
	$(AARCH64_CC) --target=aarch64-none-elf -Oz -ffreestanding -fno-builtin -fno-stack-protector -fno-pic -fno-pie -nostdlib -DRANGE_START_PAGE=$(NAND_RANGE_START)u -DRANGE_PAGE_COUNT=$(NAND_RANGE_COUNT)u -DNAND_READ_PAGES_ADDR=$(NAND_READ_PAGES_ADDR) -DUART_INIT_ADDR=$(UART_INIT_ADDR) -DREQUESTED_UART_BAUD=$(NAND_UART_BAUD) -c $< -o $@.o
	$(AARCH64_LD) -T payload/cb62_nand_endurance.ld --build-id=none $@.o -o $@

$(BUILD_DIR)/payload/cb62_nand_range_$(NAND_RANGE_PROFILE)_$(NAND_RANGE_START)_$(NAND_RANGE_COUNT).bin: $(BUILD_DIR)/payload/cb62_nand_range_$(NAND_RANGE_PROFILE)_$(NAND_RANGE_START)_$(NAND_RANGE_COUNT).elf
	$(LLVM_OBJCOPY) -O binary $< $@

$(BUILD_DIR)/payload/cb62_nand_range_$(NAND_RANGE_PROFILE)_$(NAND_RANGE_START)_$(NAND_RANGE_COUNT)_blob.c: $(BUILD_DIR)/payload/cb62_nand_range_$(NAND_RANGE_PROFILE)_$(NAND_RANGE_START)_$(NAND_RANGE_COUNT).bin
	xxd -i -n cb62_nand_endurance_payload $< > $@

$(NAND_RANGE_BIN): $(BUILD_DIR)/payload/cb62_nand_range_$(NAND_RANGE_PROFILE)_$(NAND_RANGE_START)_$(NAND_RANGE_COUNT)_blob.c $(ENDURANCE_COMMON) src/nand_range_main.c src/nand_endurance_payload_blob.h src/probe.h | $(OUT_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -DRANGE_START_PAGE=$(NAND_RANGE_START) -DRANGE_PAGE_COUNT=$(NAND_RANGE_COUNT) -DTARGET_UART_REQUESTED_BAUD=$(NAND_UART_BAUD) -DTARGET_PROFILE_NAME=\"$(NAND_RANGE_PROFILE)\" $(ENDURANCE_COMMON) src/libusb_backend.c src/nand_range_main.c $< -o $@ $(LDLIBS)

$(BUILD_DIR)/payload/cb62_nand_page110.elf: payload/cb62_nand_page110.c payload/cb62_nand_page0.ld
	mkdir -p $(BUILD_DIR)/payload
	$(AARCH64_CC) --target=aarch64-none-elf -Oz -ffreestanding -fno-builtin -fno-stack-protector -fno-pic -fno-pie -nostdlib -c $< -o $@.o
	$(AARCH64_LD) -T payload/cb62_nand_page0.ld --build-id=none $@.o -o $@

$(BUILD_DIR)/payload/cb62_nand_page110.bin: $(BUILD_DIR)/payload/cb62_nand_page110.elf
	$(LLVM_OBJCOPY) -O binary $< $@

$(BUILD_DIR)/payload/cb62_nand_page110_blob.c: $(BUILD_DIR)/payload/cb62_nand_page110.bin
	xxd -i -n cb62_nand_page0_payload $< > $@

$(NAND_PAGE110_BIN): $(BUILD_DIR)/payload/cb62_nand_page110_blob.c src/nand_protocol.c src/nand_probe.c src/nand_page110_main.c src/probe.h | $(OUT_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) src/nand_protocol.c src/nand_probe.c src/libusb_backend.c src/nand_page110_main.c $< -o $@ $(LDLIBS)

$(BUILD_DIR)/payload/cb62_nand_endurance_1024.elf: payload/cb62_nand_endurance.c payload/cb62_nand_endurance.ld
	mkdir -p $(BUILD_DIR)/payload
	$(AARCH64_CC) --target=aarch64-none-elf -Oz -ffreestanding -fno-builtin -fno-stack-protector -fno-pic -fno-pie -nostdlib -DENDURANCE_PAGE_COUNT=1024u -c $< -o $@.o
	$(AARCH64_LD) -T payload/cb62_nand_endurance.ld --build-id=none $@.o -o $@

$(BUILD_DIR)/payload/cb62_nand_endurance_4096.elf: payload/cb62_nand_endurance.c payload/cb62_nand_endurance.ld
	mkdir -p $(BUILD_DIR)/payload
	$(AARCH64_CC) --target=aarch64-none-elf -Oz -ffreestanding -fno-builtin -fno-stack-protector -fno-pic -fno-pie -nostdlib -DENDURANCE_PAGE_COUNT=4096u -c $< -o $@.o
	$(AARCH64_LD) -T payload/cb62_nand_endurance.ld --build-id=none $@.o -o $@

$(BUILD_DIR)/payload/cb62_nand_endurance_8192.elf: payload/cb62_nand_endurance.c payload/cb62_nand_endurance.ld
	mkdir -p $(BUILD_DIR)/payload
	$(AARCH64_CC) --target=aarch64-none-elf -Oz -ffreestanding -fno-builtin -fno-stack-protector -fno-pic -fno-pie -nostdlib -DENDURANCE_PAGE_COUNT=8192u -c $< -o $@.o
	$(AARCH64_LD) -T payload/cb62_nand_endurance.ld --build-id=none $@.o -o $@

$(BUILD_DIR)/payload/%.bin: $(BUILD_DIR)/payload/%.elf
	$(LLVM_OBJCOPY) -O binary $< $@

$(BUILD_DIR)/payload/cb62_nand_endurance_%_blob.c: $(BUILD_DIR)/payload/cb62_nand_endurance_%.bin
	xxd -i -n cb62_nand_endurance_payload $< > $@

$(OUT_DIR)/cb62-nand-endurance-%: $(BUILD_DIR)/payload/cb62_nand_endurance_%_blob.c $(ENDURANCE_COMMON) src/nand_endurance_main.c src/nand_endurance_payload_blob.h src/probe.h | $(OUT_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -DENDURANCE_PAGE_COUNT=$* $(ENDURANCE_COMMON) src/libusb_backend.c src/nand_endurance_main.c $< -o $@ $(LDLIBS)

$(BUILD_DIR)/test_nand_endurance_probe: tests/test_nand_endurance_probe.c $(BUILD_DIR)/payload/cb62_nand_endurance_1024_blob.c $(ENDURANCE_COMMON) src/nand_endurance_probe.h src/nand_endurance_payload_blob.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) tests/test_nand_endurance_probe.c $(ENDURANCE_COMMON) $(BUILD_DIR)/payload/cb62_nand_endurance_1024_blob.c -o $@

$(BIN): $(REAL) | $(OUT_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(REAL) -o $@ $(LDLIBS)

$(ADDR_BIN): $(ADDR_REAL) src/probe.h | $(OUT_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(ADDR_REAL) -o $@ $(LDLIBS)

$(MEMORY_BIN): $(MEMORY_REAL) src/probe.h | $(OUT_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(MEMORY_REAL) -o $@ $(LDLIBS)

$(BLOCK_BIN): $(BLOCK_REAL) src/probe.h | $(OUT_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(BLOCK_REAL) -o $@ $(LDLIBS)

$(RESIDENT_BIN): $(RESIDENT_REAL) src/probe.h | $(OUT_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(RESIDENT_REAL) -o $@ $(LDLIBS)

$(PTB_BIN): $(PTB_REAL) src/probe.h | $(OUT_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(PTB_REAL) -o $@ $(LDLIBS)

$(NAND_BIN): $(NAND_REAL) src/probe.h | $(OUT_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(NAND_REAL) -o $@ $(LDLIBS)

$(NAND_BLOCK0_BIN): $(NAND_BLOCK0_REAL) src/probe.h | $(OUT_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(NAND_BLOCK0_REAL) -o $@ $(LDLIBS)

$(BUILD_DIR)/test_protocol: tests/test_protocol.c src/protocol.c src/protocol.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) tests/test_protocol.c src/protocol.c -o $@

$(BUILD_DIR)/test_probe: tests/test_probe.c tests/fake_libusb.c $(COMMON) src/probe.h src/protocol.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) tests/test_probe.c tests/fake_libusb.c $(COMMON) -o $@

$(BUILD_DIR)/test_addr_protocol: tests/test_addr_protocol.c src/addr_protocol.c src/addr_protocol.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) tests/test_addr_protocol.c src/addr_protocol.c -o $@

$(BUILD_DIR)/test_addr_probe: tests/test_addr_probe.c tests/fake_libusb.c $(ADDR_COMMON) src/addr_probe.h src/addr_protocol.h src/probe.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) tests/test_addr_probe.c tests/fake_libusb.c $(ADDR_COMMON) -o $@

$(BUILD_DIR)/test_memory_protocol: tests/test_memory_protocol.c src/memory_protocol.c src/memory_protocol.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) tests/test_memory_protocol.c src/memory_protocol.c -o $@

$(BUILD_DIR)/test_memory_probe: tests/test_memory_probe.c tests/fake_libusb.c $(MEMORY_COMMON) src/memory_probe.h src/memory_protocol.h src/probe.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) tests/test_memory_probe.c tests/fake_libusb.c $(MEMORY_COMMON) -o $@

$(BUILD_DIR)/test_block_protocol: tests/test_block_protocol.c src/block_protocol.c src/block_protocol.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) tests/test_block_protocol.c src/block_protocol.c -o $@

$(BUILD_DIR)/test_block_probe: tests/test_block_probe.c $(BLOCK_COMMON) src/block_probe.h src/block_protocol.h src/probe.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) tests/test_block_probe.c $(BLOCK_COMMON) -o $@

$(BUILD_DIR)/test_resident_protocol: tests/test_resident_protocol.c src/resident_protocol.c src/resident_protocol.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) tests/test_resident_protocol.c src/resident_protocol.c -o $@

$(BUILD_DIR)/test_resident_probe: tests/test_resident_probe.c $(RESIDENT_COMMON) src/resident_probe.h src/resident_protocol.h src/probe.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) tests/test_resident_probe.c $(RESIDENT_COMMON) -o $@

$(BUILD_DIR)/test_ptb_protocol: tests/test_ptb_protocol.c src/ptb_protocol.c src/ptb_protocol.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) tests/test_ptb_protocol.c src/ptb_protocol.c -o $@

$(BUILD_DIR)/test_ptb_probe: tests/test_ptb_probe.c $(PTB_COMMON) src/ptb_probe.h src/ptb_protocol.h src/probe.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) tests/test_ptb_probe.c $(PTB_COMMON) -o $@

$(BUILD_DIR)/test_nand_protocol: tests/test_nand_protocol.c src/nand_protocol.c src/nand_protocol.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) tests/test_nand_protocol.c src/nand_protocol.c -o $@

$(BUILD_DIR)/test_nand_probe: tests/test_nand_probe.c $(NAND_COMMON) src/nand_probe.h src/nand_protocol.h src/probe.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) tests/test_nand_probe.c $(NAND_COMMON) -o $@

$(BUILD_DIR)/test_nand_block0_probe: tests/test_nand_block0_probe.c $(NAND_BLOCK0_COMMON) src/nand_block0_probe.h src/nand_protocol.h src/probe.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) tests/test_nand_block0_probe.c $(NAND_BLOCK0_COMMON) -o $@

test: $(BIN) $(ADDR_BIN) $(MEMORY_BIN) $(BLOCK_BIN) $(RESIDENT_BIN) $(PTB_BIN) $(NAND_BIN) $(NAND_PAGE110_BIN) $(NAND_RANGE_BIN) $(NAND_BLOCK0_BIN) $(ENDURANCE_BINS) $(BUILD_DIR)/test_protocol $(BUILD_DIR)/test_probe $(BUILD_DIR)/test_addr_protocol $(BUILD_DIR)/test_addr_probe $(BUILD_DIR)/test_memory_protocol $(BUILD_DIR)/test_memory_probe $(BUILD_DIR)/test_block_protocol $(BUILD_DIR)/test_block_probe $(BUILD_DIR)/test_resident_protocol $(BUILD_DIR)/test_resident_probe $(BUILD_DIR)/test_ptb_protocol $(BUILD_DIR)/test_ptb_probe $(BUILD_DIR)/test_nand_protocol $(BUILD_DIR)/test_nand_probe $(BUILD_DIR)/test_nand_block0_probe $(BUILD_DIR)/test_nand_endurance_probe
	$(BUILD_DIR)/test_protocol
	$(BUILD_DIR)/test_probe
	$(BUILD_DIR)/test_addr_protocol
	$(BUILD_DIR)/test_addr_probe
	$(BUILD_DIR)/test_memory_protocol
	$(BUILD_DIR)/test_memory_probe
	$(BUILD_DIR)/test_block_protocol
	$(BUILD_DIR)/test_block_probe
	$(BUILD_DIR)/test_resident_protocol
	$(BUILD_DIR)/test_resident_probe
	$(BUILD_DIR)/test_ptb_protocol
	$(BUILD_DIR)/test_ptb_probe
	$(BUILD_DIR)/test_nand_protocol
	$(BUILD_DIR)/test_nand_probe
	$(BUILD_DIR)/test_nand_block0_probe
	$(BUILD_DIR)/test_nand_endurance_probe
	python3 tests/test_nand_block0_payload.py
	python3 tests/test_nand_endurance_payload.py
	python3 tests/test_nand_page110_payload.py
	python3 tests/test_nand_range.py
	python3 tools/test_public_release.py -v
	python3 tests/test_capture_nand_block0.py
	python3 tests/test_capture_nand_endurance.py
	sh tests/test_payload_toolchain.sh
	sh tests/test_cli.sh $(BIN)
	sh tests/test_addr_cli.sh $(ADDR_BIN)
	sh tests/test_memory_cli.sh $(MEMORY_BIN)
	sh tests/test_block_cli.sh $(BLOCK_BIN)
	sh tests/test_resident_cli.sh $(RESIDENT_BIN)
	sh tests/test_ptb_cli.sh $(PTB_BIN)
	sh tests/test_nand_block0_cli.sh $(NAND_BLOCK0_BIN)
	sh tests/test_nand_endurance_cli.sh $(OUT_DIR)
	sh tests/test_nand_page110_cli.sh $(NAND_PAGE110_BIN)
	sh tests/test_nand_range_cli.sh $(NAND_RANGE_BIN) $(NAND_RANGE_PROFILE) $(NAND_RANGE_START) $(NAND_RANGE_COUNT)

$(BUILD_DIR) $(OUT_DIR):
	mkdir -p $@

clean:
	rm -rf $(BUILD_DIR) $(OUT_DIR)
