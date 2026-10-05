#include <stddef.h>
#include <stdint.h>

#define NAND_READ_PAGES_ADDR 0x01411a60u
#define UART0_BASE 0xe4000000u
#define UART_LSR_OFFSET 0x14u
#define UART_LSR_THRE 0x40u
#define MAIN_BUFFER_ADDR 0x02100000u
#define SPARE_BUFFER_ADDR 0x02100800u
#define MAIN_LENGTH 2048u
#define SPARE_LENGTH 128u
#define FIRST_PAGE 0u
#define PAGES_PER_BLOCK 64u

#ifndef ENDURANCE_PAGE_COUNT
#error ENDURANCE_PAGE_COUNT must be one of the fixed build-profile bounds
#endif

#if ENDURANCE_PAGE_COUNT != 1024u && ENDURANCE_PAGE_COUNT != 4096u && \
    ENDURANCE_PAGE_COUNT != 8192u
#error unsupported endurance page count
#endif

typedef int (*nand_read_pages_fn)(uint32_t, uint32_t, uint32_t,
                                  void *, void *, uint32_t);

static void uart_put(uint8_t value)
{
    volatile uint8_t *uart = (volatile uint8_t *)(uintptr_t)UART0_BASE;
    while ((uart[UART_LSR_OFFSET] & UART_LSR_THRE) == 0u) { }
    uart[0] = value;
}

static void uart_u32(uint32_t value)
{
    uart_put((uint8_t)value);
    uart_put((uint8_t)(value >> 8));
    uart_put((uint8_t)(value >> 16));
    uart_put((uint8_t)(value >> 24));
}

static uint32_t crc32_parts(const uint8_t *main_data,
                            const uint8_t *spare_data)
{
    uint32_t crc = 0xffffffffu;
    for (unsigned part = 0; part < 2; part++) {
        const uint8_t *data = part == 0 ? main_data : spare_data;
        size_t length = part == 0 ? MAIN_LENGTH : SPARE_LENGTH;
        for (size_t i = 0; i < length; i++) {
            crc ^= data[i];
            for (unsigned bit = 0; bit < 8; bit++)
                crc = (crc >> 1) ^ (0xedb88320u &
                      (uint32_t)-(int32_t)(crc & 1u));
        }
    }
    return ~crc;
}

static void emit_frame(uint32_t page, int status,
                       const uint8_t *main_data, const uint8_t *spare_data)
{
    static const uint8_t magic[8] = {'C','B','6','2','N','A','N','D'};
    uint32_t crc = status == 0 ? crc32_parts(main_data, spare_data) : 0u;

    for (size_t i = 0; i < sizeof(magic); i++) uart_put(magic[i]);
    uart_u32(1u);
    uart_u32(page);
    uart_u32(page);
    uart_u32(1u);
    uart_u32((uint32_t)status);
    uart_u32(status == 0 ? MAIN_LENGTH : 0u);
    uart_u32(status == 0 ? SPARE_LENGTH : 0u);
    uart_u32(crc);
    if (status == 0) {
        for (size_t i = 0; i < MAIN_LENGTH; i++) uart_put(main_data[i]);
        for (size_t i = 0; i < SPARE_LENGTH; i++) uart_put(spare_data[i]);
    }
}

__attribute__((noreturn, section(".text.entry"))) void _start(void)
{
    uint8_t *main_data = (uint8_t *)(uintptr_t)MAIN_BUFFER_ADDR;
    uint8_t *spare_data = (uint8_t *)(uintptr_t)SPARE_BUFFER_ADDR;
    nand_read_pages_fn read_pages =
        (nand_read_pages_fn)(uintptr_t)NAND_READ_PAGES_ADDR;

    for (uint32_t page = FIRST_PAGE; page < ENDURANCE_PAGE_COUNT; page++) {
        uint32_t block = page / PAGES_PER_BLOCK;
        uint32_t page_in_block = page % PAGES_PER_BLOCK;
        int status = read_pages(block, page_in_block, 1u,
                                main_data, spare_data, 0u);
        emit_frame(page, status, main_data, spare_data);
        if (status != 0) break;
    }
    for (;;) __asm__ volatile("wfe");
}
