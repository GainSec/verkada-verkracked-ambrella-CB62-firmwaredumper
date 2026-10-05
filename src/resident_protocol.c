#include "resident_protocol.h"

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

static void build_command(uint8_t out[CB62_RESIDENT_COMMAND_SIZE],
                          uint32_t command, uint32_t p0, uint32_t p1,
                          uint32_t p2, uint32_t p3)
{
    const uint32_t words[8] = {
        CB62_RESIDENT_COMMAND_TOKEN, command, p0, p1, p2, p3, 0, 0,
    };
    for (size_t i = 0; i < 8; i++) put_u32le(out + i * 4, words[i]);
}

void cb62_build_fixed_resident_ready_request(
    uint8_t out[CB62_RESIDENT_COMMAND_SIZE])
{
    build_command(out, CB62_RESIDENT_READY_TO_SEND_COMMAND, 0,
                  CB62_RESIDENT_MEMORY_SUBCOMMAND,
                  CB62_RESIDENT_FIXED_ADDRESS, CB62_RESIDENT_FIXED_LENGTH);
}

void cb62_build_resident_send_data_request(
    uint8_t out[CB62_RESIDENT_COMMAND_SIZE])
{
    build_command(out, CB62_RESIDENT_SEND_DATA_COMMAND, 0, 0, 0, 0);
}

int cb62_parse_resident_ready_response(
    const uint8_t *wire, size_t wire_len,
    struct cb62_resident_ready_response *out)
{
    if (wire_len != CB62_RESIDENT_RESPONSE_SIZE)
        return CB62_RESIDENT_PARSE_WRONG_RESPONSE_LENGTH;
    out->signature = get_u32le(wire + 0);
    out->status = get_u32le(wire + 4);
    out->length = get_u32le(wire + 8);
    out->crc32 = get_u32le(wire + 12);
    if (out->signature != CB62_RESIDENT_STATUS_TOKEN)
        return CB62_RESIDENT_PARSE_BAD_SIGNATURE;
    if (out->status != CB62_RESIDENT_RESPONSE_SUCCESS)
        return CB62_RESIDENT_PARSE_DEVICE_FAILURE;
    if (out->length != CB62_RESIDENT_FIXED_LENGTH)
        return CB62_RESIDENT_PARSE_WRONG_DATA_LENGTH;
    return CB62_RESIDENT_PARSE_OK;
}

uint32_t cb62_resident_crc32(const uint8_t *data, size_t length)
{
    uint32_t crc = 0xffffffffu;
    for (size_t i = 0; i < length; i++) {
        crc ^= data[i];
        for (unsigned bit = 0; bit < 8; bit++)
            crc = (crc >> 1) ^ (0xedb88320u & (0u - (crc & 1u)));
    }
    return ~crc;
}
