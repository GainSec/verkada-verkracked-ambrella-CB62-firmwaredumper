#ifndef CB62_INQUIRY_PROTOCOL_H
#define CB62_INQUIRY_PROTOCOL_H

#include <stddef.h>
#include <stdint.h>

#define CB62_INQUIRY_REQUEST_SIZE 32u
#define CB62_INQUIRY_RESPONSE_SIZE 16u

#define CB62_COMMAND_TOKEN 0x55434d44u
#define CB62_STATUS_TOKEN 0x55525350u
#define CB62_INQUIRY_STATUS_COMMAND 4u
#define CB62_INQUIRY_CHIP_SUBCOMMAND 1u
#define CB62_RESPONSE_SUCCESS 0u
#define CB62_EXPECTED_CHIP_ID 0x000055f0u

struct cb62_inquiry_response {
    uint32_t signature;
    uint32_t status;
    uint32_t chip_id;
    uint32_t p1;
};

enum cb62_parse_result {
    CB62_PARSE_OK = 0,
    CB62_PARSE_WRONG_LENGTH = -1,
    CB62_PARSE_BAD_SIGNATURE = -2,
    CB62_PARSE_DEVICE_FAILURE = -3,
};

void cb62_build_inquiry_request(uint8_t out[CB62_INQUIRY_REQUEST_SIZE]);
int cb62_parse_inquiry_response(const uint8_t *wire, size_t wire_len,
                                struct cb62_inquiry_response *out);

#endif
