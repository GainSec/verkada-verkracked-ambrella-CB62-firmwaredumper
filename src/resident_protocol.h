#ifndef CB62_RESIDENT_PROTOCOL_H
#define CB62_RESIDENT_PROTOCOL_H

#include <stddef.h>
#include <stdint.h>

#define CB62_RESIDENT_COMMAND_SIZE 32u
#define CB62_RESIDENT_RESPONSE_SIZE 16u
#define CB62_RESIDENT_FIXED_ADDRESS 0x01400000u
#define CB62_RESIDENT_FIXED_LENGTH 0x00034000u
#define CB62_RESIDENT_COMMAND_TOKEN 0x55434d44u
#define CB62_RESIDENT_STATUS_TOKEN 0x55525350u
#define CB62_RESIDENT_READY_TO_SEND_COMMAND 2u
#define CB62_RESIDENT_SEND_DATA_COMMAND 3u
#define CB62_RESIDENT_MEMORY_SUBCOMMAND 1u
#define CB62_RESIDENT_RESPONSE_SUCCESS 0u

struct cb62_resident_ready_response {
    uint32_t signature;
    uint32_t status;
    uint32_t length;
    uint32_t crc32;
};

enum cb62_resident_parse_result {
    CB62_RESIDENT_PARSE_OK = 0,
    CB62_RESIDENT_PARSE_WRONG_RESPONSE_LENGTH = -1,
    CB62_RESIDENT_PARSE_BAD_SIGNATURE = -2,
    CB62_RESIDENT_PARSE_DEVICE_FAILURE = -3,
    CB62_RESIDENT_PARSE_WRONG_DATA_LENGTH = -4,
};

void cb62_build_fixed_resident_ready_request(
    uint8_t out[CB62_RESIDENT_COMMAND_SIZE]);
void cb62_build_resident_send_data_request(
    uint8_t out[CB62_RESIDENT_COMMAND_SIZE]);
int cb62_parse_resident_ready_response(
    const uint8_t *wire, size_t wire_len,
    struct cb62_resident_ready_response *out);
uint32_t cb62_resident_crc32(const uint8_t *data, size_t length);

#endif
