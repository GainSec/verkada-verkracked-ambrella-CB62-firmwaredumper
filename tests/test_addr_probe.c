#include "../src/addr_probe.h"
#include "fake_libusb.h"

#include <stdio.h>
#include <string.h>

static int failures;

#define CHECK(expr) do { \
    if (!(expr)) { \
        fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #expr); \
        failures++; \
    } \
} while (0)

static void set_success_address(struct fake_libusb *fake, uint32_t address)
{
    fake->response[0] = 0x50;
    fake->response[1] = 0x53;
    fake->response[2] = 0x52;
    fake->response[3] = 0x55;
    fake->response[4] = 0;
    fake->response[5] = 0;
    fake->response[6] = 0;
    fake->response[7] = 0;
    fake->response[8] = (uint8_t)address;
    fake->response[9] = (uint8_t)(address >> 8);
    fake->response[10] = (uint8_t)(address >> 16);
    fake->response[11] = (uint8_t)(address >> 24);
    memset(fake->response + 12, 0, 4);
}

static void check_rejected_without_bulk(struct fake_libusb *fake,
                                        int expected)
{
    struct cb62_addr_probe_observation observation = {0};
    CHECK(cb62_probe_addr_once(fake_libusb_ops(), fake, &observation) ==
          expected);
    CHECK(fake->open_calls == 0);
    CHECK(fake->out_calls == 0);
    CHECK(fake->in_calls == 0);
    CHECK(fake->close_calls == 0);
}

static void test_identity_and_topology_refusal(void)
{
    struct fake_libusb fake;
    fake_libusb_init_valid(&fake);
    fake.devices[0].pid = 0x0013;
    check_rejected_without_bulk(&fake, CB62_ADDR_PROBE_IDENTITY_REJECTED);

    fake_libusb_init_valid(&fake);
    strcpy(fake.devices[0].product, "unexpected");
    check_rejected_without_bulk(&fake, CB62_ADDR_PROBE_IDENTITY_REJECTED);

    fake_libusb_init_valid(&fake);
    fake.devices[0].matching_interface_count = 2;
    check_rejected_without_bulk(&fake, CB62_ADDR_PROBE_TOPOLOGY_REJECTED);

    fake_libusb_init_valid(&fake);
    fake.devices[0].bulk_endpoint_count = 3;
    check_rejected_without_bulk(&fake, CB62_ADDR_PROBE_TOPOLOGY_REJECTED);

    fake_libusb_init_valid(&fake);
    fake.devices[0].endpoint_in = 0x82;
    check_rejected_without_bulk(&fake, CB62_ADDR_PROBE_TOPOLOGY_REJECTED);
}

static void test_duplicate_device_refusal(void)
{
    struct fake_libusb fake;
    fake_libusb_init_valid(&fake);
    fake.devices[1] = fake.devices[0];
    fake.device_count = 2;
    check_rejected_without_bulk(&fake, CB62_ADDR_PROBE_DEVICE_COUNT_REJECTED);
}

static void test_exact_success_trace(void)
{
    static const uint8_t expected[CB62_ADDR_INQUIRY_REQUEST_SIZE] = {
        0x44, 0x4d, 0x43, 0x55, 0x04, 0x00, 0x00, 0x00,
        0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    };
    struct fake_libusb fake;
    struct cb62_addr_probe_observation observation = {0};
    fake_libusb_init_valid(&fake);
    set_success_address(&fake, 0x20000000u);
    CHECK(cb62_probe_addr_once(fake_libusb_ops(), &fake, &observation) ==
          CB62_ADDR_PROBE_OK);
    CHECK(fake.open_calls == 1);
    CHECK(fake.out_calls == 1);
    CHECK(fake.in_calls == 1);
    CHECK(fake.close_calls == 1);
    CHECK(fake.sent_len == sizeof(expected));
    CHECK(memcmp(fake.sent, expected, sizeof(expected)) == 0);
    CHECK(observation.response.address == 0x20000000u);
}

static void test_short_transfer_has_no_retry(void)
{
    struct fake_libusb fake;
    struct cb62_addr_probe_observation observation = {0};
    fake_libusb_init_valid(&fake);
    fake.short_out = 1;
    CHECK(cb62_probe_addr_once(fake_libusb_ops(), &fake, &observation) ==
          CB62_ADDR_PROBE_OUT_FAILED);
    CHECK(fake.out_calls == 1);
    CHECK(fake.in_calls == 0);
    CHECK(fake.close_calls == 1);

    fake_libusb_init_valid(&fake);
    fake.short_in = 1;
    CHECK(cb62_probe_addr_once(fake_libusb_ops(), &fake, &observation) ==
          CB62_ADDR_PROBE_IN_FAILED);
    CHECK(fake.out_calls == 1);
    CHECK(fake.in_calls == 1);
    CHECK(fake.close_calls == 1);
}

static void test_bad_response_closes_without_retry(void)
{
    struct fake_libusb fake;
    struct cb62_addr_probe_observation observation = {0};
    fake_libusb_init_valid(&fake);
    memset(fake.response, 0, sizeof(fake.response));
    CHECK(cb62_probe_addr_once(fake_libusb_ops(), &fake, &observation) ==
          CB62_ADDR_PROBE_RESPONSE_REJECTED);
    CHECK(fake.out_calls == 1);
    CHECK(fake.in_calls == 1);
    CHECK(fake.close_calls == 1);
}

int main(void)
{
    test_identity_and_topology_refusal();
    test_duplicate_device_refusal();
    test_exact_success_trace();
    test_short_transfer_has_no_retry();
    test_bad_response_closes_without_retry();
    if (failures != 0) return 1;
    puts("test_addr_probe: PASS");
    return 0;
}
