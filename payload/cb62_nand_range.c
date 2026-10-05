#include <stddef.h>
#include <stdint.h>

#ifndef RANGE_START_PAGE
#error fixed RANGE_START_PAGE required
#endif
#ifndef RANGE_PAGE_COUNT
#error fixed RANGE_PAGE_COUNT required
#endif
#ifndef NAND_READ_PAGES_ADDR
#error exact resident NAND_READ_PAGES_ADDR required
#endif
#ifndef UART_INIT_ADDR
#error exact resident UART_INIT_ADDR required
#endif
#ifndef REQUESTED_UART_BAUD
#error fixed REQUESTED_UART_BAUD required
#endif

#define UART0_BASE 0xe4000000u
#define UART_LSR_OFFSET 0x14u
#define UART_LSR_THRE 0x40u
#define MAIN_BUFFER_ADDR 0x02100000u
#define SPARE_BUFFER_ADDR 0x02100800u
#define CRC_TABLE_ADDR 0x02101000u
#define MAIN_LENGTH 2048u
#define SPARE_LENGTH 128u
#define TOTAL_NAND_PAGES 131072u
#define PAGES_PER_BLOCK 64u

#if RANGE_PAGE_COUNT == 0 || RANGE_START_PAGE >= TOTAL_NAND_PAGES || \
    RANGE_PAGE_COUNT > (TOTAL_NAND_PAGES - RANGE_START_PAGE)
#error invalid fixed NAND range
#endif

typedef int (*nand_read_pages_fn)(uint32_t, uint32_t, uint32_t,
                                  void *, void *, uint32_t);
typedef void (*uart_init_fn)(uint32_t, uint32_t);

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

static void crc32_init_table(void)
{
    uint32_t *table = (uint32_t *)(uintptr_t)CRC_TABLE_ADDR;
    for (uint32_t i = 0; i < 256u; i++) {
        uint32_t value = i;
        for (unsigned bit = 0; bit < 8; bit++)
            value = (value >> 1) ^
                (0xedb88320u & (uint32_t)-(int32_t)(value & 1u));
        table[i] = value;
    }
}

static uint32_t crc32_parts(const uint8_t *main_data,
                            const uint8_t *spare_data)
{
    const uint32_t *table = (const uint32_t *)(uintptr_t)CRC_TABLE_ADDR;
    uint32_t crc = 0xffffffffu;
    for (unsigned part = 0; part < 2; part++) {
        const uint8_t *data = part == 0 ? main_data : spare_data;
        size_t length = part == 0 ? MAIN_LENGTH : SPARE_LENGTH;
        for (size_t i = 0; i < length; i++)
            crc = (crc >> 8) ^ table[(crc ^ data[i]) & 0xffu];
    }
    return ~crc;
}

static void emit_frame(uint32_t sequence, uint32_t page, int status,
                       const uint8_t *main_data, const uint8_t *spare_data)
{
    static const uint8_t magic[8] = {'C','B','6','2','N','A','N','D'};
    uint32_t crc = status == 0 ? crc32_parts(main_data, spare_data) : 0u;

    for (size_t i = 0; i < sizeof(magic); i++) uart_put(magic[i]);
    uart_u32(1u);
    uart_u32(sequence);
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
    uart_init_fn uart_init = (uart_init_fn)(uintptr_t)UART_INIT_ADDR;

    crc32_init_table();
    uart_init(0u, REQUESTED_UART_BAUD);
    for (uint32_t sequence = 0u; sequence < RANGE_PAGE_COUNT; sequence++) {
        uint32_t page = RANGE_START_PAGE + sequence;
        uint32_t block = page / PAGES_PER_BLOCK;
        uint32_t page_in_block = page % PAGES_PER_BLOCK;
        int status = read_pages(block, page_in_block, 1u,
                                main_data, spare_data, 0u);
        emit_frame(sequence, page, status, main_data, spare_data);
        if (status != 0) break;
    }
    for (;;) __asm__ volatile("wfe");
}
