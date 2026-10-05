#include "../src/protocol.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

static int failures;

#define CHECK(expr) do { \
    if (!(expr)) { \
        fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #expr); \
        failures++; \
    } \
} while (0)

static void put_u32le(uint8_t *p, uint32_t v)
{
    p[0] = (uint8_t)v;
    p[1] = (uint8_t)(v >> 8);
    p[2] = (uint8_t)(v >> 16);
    p[3] = (uint8_t)(v >> 24);
}

static void test_exact_request(void)
{
    static const uint8_t expected[CB62_INQUIRY_REQUEST_SIZE] = {
        0x44, 0x4d, 0x43, 0x55, 0x04, 0x00, 0x00, 0x00,
        0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    };
    uint8_t actual[CB62_INQUIRY_REQUEST_SIZE];
    cb62_build_inquiry_request(actual);
    CHECK(memcmp(actual, expected, sizeof(expected)) == 0);
}

static void test_valid_response(void)
{
    uint8_t wire[CB62_INQUIRY_RESPONSE_SIZE] = {0};
    struct cb62_inquiry_response out = {0};
    put_u32le(wire + 0, CB62_STATUS_TOKEN);
    put_u32le(wire + 4, CB62_RESPONSE_SUCCESS);
    put_u32le(wire + 8, CB62_EXPECTED_CHIP_ID);
    put_u32le(wire + 12, 0xa5a5a5a5u);
    CHECK(cb62_parse_inquiry_response(wire, sizeof(wire), &out) == CB62_PARSE_OK);
    CHECK(out.signature == CB62_STATUS_TOKEN);
    CHECK(out.status == CB62_RESPONSE_SUCCESS);
    CHECK(out.chip_id == CB62_EXPECTED_CHIP_ID);
    CHECK(out.p1 == 0xa5a5a5a5u);
}

static void test_short_response_rejected(void)
{
    uint8_t wire[CB62_INQUIRY_RESPONSE_SIZE] = {0};
    struct cb62_inquiry_response out = {0};
    CHECK(cb62_parse_inquiry_response(wire, sizeof(wire) - 1, &out) ==
          CB62_PARSE_WRONG_LENGTH);
}

static void test_bad_signature_rejected(void)
{
    uint8_t wire[CB62_INQUIRY_RESPONSE_SIZE] = {0};
    struct cb62_inquiry_response out = {0};
    put_u32le(wire + 0, 0x11223344u);
    CHECK(cb62_parse_inquiry_response(wire, sizeof(wire), &out) ==
          CB62_PARSE_BAD_SIGNATURE);
}

static void test_failure_status_rejected(void)
{
    uint8_t wire[CB62_INQUIRY_RESPONSE_SIZE] = {0};
    struct cb62_inquiry_response out = {0};
    put_u32le(wire + 0, CB62_STATUS_TOKEN);
    put_u32le(wire + 4, 1);
    CHECK(cb62_parse_inquiry_response(wire, sizeof(wire), &out) ==
          CB62_PARSE_DEVICE_FAILURE);
    CHECK(out.status == 1);
}

int main(void)
{
    test_exact_request();
    test_valid_response();
    test_short_response_rejected();
    test_bad_signature_rejected();
    test_failure_status_rejected();
    if (failures != 0) return 1;
    puts("test_protocol: PASS");
    return 0;
}
