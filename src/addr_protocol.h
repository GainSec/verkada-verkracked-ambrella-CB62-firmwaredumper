#ifndef CB62_ADDR_INQUIRY_PROTOCOL_H
#define CB62_ADDR_INQUIRY_PROTOCOL_H

#include <stddef.h>
#include <stdint.h>

#define CB62_ADDR_INQUIRY_REQUEST_SIZE 32u
#define CB62_ADDR_INQUIRY_RESPONSE_SIZE 16u

#define CB62_ADDR_COMMAND_TOKEN 0x55434d44u
#define CB62_ADDR_STATUS_TOKEN 0x55525350u
#define CB62_ADDR_INQUIRY_STATUS_COMMAND 4u
#define CB62_ADDR_INQUIRY_SUBCOMMAND 2u
#define CB62_ADDR_RESPONSE_SUCCESS 0u

struct cb62_addr_inquiry_response {
    uint32_t signature;
    uint32_t status;
    uint32_t address;
    uint32_t p1;
};

enum cb62_addr_parse_result {
    CB62_ADDR_PARSE_OK = 0,
    CB62_ADDR_PARSE_WRONG_LENGTH = -1,
    CB62_ADDR_PARSE_BAD_SIGNATURE = -2,
    CB62_ADDR_PARSE_DEVICE_FAILURE = -3,
};

void cb62_build_addr_inquiry_request(
    uint8_t out[CB62_ADDR_INQUIRY_REQUEST_SIZE]);
int cb62_parse_addr_inquiry_response(
    const uint8_t *wire, size_t wire_len,
    struct cb62_addr_inquiry_response *out);

#endif
