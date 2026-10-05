#ifndef CB62_MEMORY_PROTOCOL_H
#define CB62_MEMORY_PROTOCOL_H

#include <stddef.h>
#include <stdint.h>

#define CB62_MEMORY_READ_REQUEST_SIZE 32u
#define CB62_MEMORY_READ_RESPONSE_SIZE 16u

#define CB62_MEMORY_COMMAND_TOKEN 0x55434d44u
#define CB62_MEMORY_STATUS_TOKEN 0x55525350u
#define CB62_MEMORY_READY_TO_RECEIVE_COMMAND 0u
#define CB62_MEMORY_COMMAND_FLAG 0x08u
#define CB62_MEMORY_READ_REQUEST_TYPE 1u
#define CB62_MEMORY_FIXED_ADDRESS 0x01400038u
#define CB62_MEMORY_EXPECTED_VALUE 0x00000309u
#define CB62_MEMORY_RESPONSE_SUCCESS 0u

struct cb62_memory_read_response {
    uint32_t signature;
    uint32_t status;
    uint32_t value;
    uint32_t p1;
};

enum cb62_memory_parse_result {
    CB62_MEMORY_PARSE_OK = 0,
    CB62_MEMORY_PARSE_WRONG_LENGTH = -1,
    CB62_MEMORY_PARSE_BAD_SIGNATURE = -2,
    CB62_MEMORY_PARSE_DEVICE_FAILURE = -3,
};

void cb62_build_memory_read_request(
    uint8_t out[CB62_MEMORY_READ_REQUEST_SIZE]);
int cb62_parse_memory_read_response(
    const uint8_t *wire, size_t wire_len,
    struct cb62_memory_read_response *out);

#endif
