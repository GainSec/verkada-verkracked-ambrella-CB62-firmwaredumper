#include "addr_protocol.h"

static void put_u32le(uint8_t *p, uint32_t value)
{
    p[0] = (uint8_t)value;
    p[1] = (uint8_t)(value >> 8);
    p[2] = (uint8_t)(value >> 16);
    p[3] = (uint8_t)(value >> 24);
}

static uint32_t get_u32le(const uint8_t *p)
{
    return (uint32_t)p[0] |
           ((uint32_t)p[1] << 8) |
           ((uint32_t)p[2] << 16) |
           ((uint32_t)p[3] << 24);
}

void cb62_build_addr_inquiry_request(
    uint8_t out[CB62_ADDR_INQUIRY_REQUEST_SIZE])
{
    static const uint32_t words[8] = {
        CB62_ADDR_COMMAND_TOKEN,
        CB62_ADDR_INQUIRY_STATUS_COMMAND,
        CB62_ADDR_INQUIRY_SUBCOMMAND,
        0, 0, 0, 0, 0,
    };
    for (size_t i = 0; i < 8; i++) put_u32le(out + i * 4, words[i]);
}

int cb62_parse_addr_inquiry_response(
    const uint8_t *wire, size_t wire_len,
    struct cb62_addr_inquiry_response *out)
{
    if (wire_len != CB62_ADDR_INQUIRY_RESPONSE_SIZE)
        return CB62_ADDR_PARSE_WRONG_LENGTH;
    out->signature = get_u32le(wire + 0);
    out->status = get_u32le(wire + 4);
    out->address = get_u32le(wire + 8);
    out->p1 = get_u32le(wire + 12);
    if (out->signature != CB62_ADDR_STATUS_TOKEN)
        return CB62_ADDR_PARSE_BAD_SIGNATURE;
    if (out->status != CB62_ADDR_RESPONSE_SUCCESS)
        return CB62_ADDR_PARSE_DEVICE_FAILURE;
    return CB62_ADDR_PARSE_OK;
}
