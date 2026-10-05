#ifndef CB62_PTB_PROTOCOL_H
#define CB62_PTB_PROTOCOL_H

#include <stddef.h>
#include <stdint.h>

#define CB62_PTB_COMMAND_SIZE 32u
#define CB62_PTB_RESPONSE_SIZE 16u
#define CB62_PTB_FIXED_LENGTH 0x00001000u
#define CB62_PTB_COMMAND_TOKEN 0x55434d44u
#define CB62_PTB_STATUS_TOKEN 0x55525350u
#define CB62_PTB_READY_TO_SEND_COMMAND 2u
#define CB62_PTB_SEND_DATA_COMMAND 3u
#define CB62_PTB_FIXED_SELECTOR 16u
#define CB62_PTB_RESPONSE_SUCCESS 0u
#define CB62_PTB_RECORD_SIZE 0x1cu
#define CB62_PTB_RECORD_MAGIC_OFFSET 0x18u
#define CB62_PTB_RECORD_MAGIC 0x8732dfe6u

struct cb62_ptb_ready_response {
    uint32_t signature;
    uint32_t status;
    uint32_t length;
    uint32_t crc32;
};

enum cb62_ptb_parse_result {
    CB62_PTB_PARSE_OK = 0,
    CB62_PTB_PARSE_WRONG_RESPONSE_LENGTH = -1,
    CB62_PTB_PARSE_BAD_SIGNATURE = -2,
    CB62_PTB_PARSE_DEVICE_FAILURE = -3,
    CB62_PTB_PARSE_WRONG_DATA_LENGTH = -4,
};

void cb62_build_fixed_ptb_ready_request(uint8_t out[CB62_PTB_COMMAND_SIZE]);
void cb62_build_ptb_send_data_request(uint8_t out[CB62_PTB_COMMAND_SIZE]);
int cb62_parse_ptb_ready_response(
    const uint8_t *wire, size_t wire_len,
    struct cb62_ptb_ready_response *out);
uint32_t cb62_ptb_crc32(const uint8_t *data, size_t length);
size_t cb62_ptb_count_record_magics(const uint8_t *data, size_t length);

#endif
