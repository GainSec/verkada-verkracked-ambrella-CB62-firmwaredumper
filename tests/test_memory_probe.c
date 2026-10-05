#include "../src/memory_probe.h"
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

static void put_u32le(uint8_t *p, uint32_t value)
{
    p[0] = (uint8_t)value;
    p[1] = (uint8_t)(value >> 8);
    p[2] = (uint8_t)(value >> 16);
    p[3] = (uint8_t)(value >> 24);
}

static void set_memory_response(struct fake_libusb *fake, uint32_t signature,
                                uint32_t status, uint32_t value)
{
    put_u32le(fake->response + 0, signature);
    put_u32le(fake->response + 4, status);
    put_u32le(fake->response + 8, value);
    put_u32le(fake->response + 12, 0);
}

static void check_rejected_without_open(struct fake_libusb *fake,
                                        int expected)
{
    struct cb62_memory_probe_observation observation = {0};
    CHECK(cb62_probe_memory_once(fake_libusb_ops(), fake, &observation) ==
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
    check_rejected_without_open(&fake, CB62_MEMORY_PROBE_IDENTITY_REJECTED);

    fake_libusb_init_valid(&fake);
    strcpy(fake.devices[0].manufacturer, "unexpected");
    check_rejected_without_open(&fake, CB62_MEMORY_PROBE_IDENTITY_REJECTED);

    fake_libusb_init_valid(&fake);
    strcpy(fake.devices[0].product, "unexpected");
    check_rejected_without_open(&fake, CB62_MEMORY_PROBE_IDENTITY_REJECTED);

    fake_libusb_init_valid(&fake);
    fake.devices[0].matching_interface_count = 2;
    check_rejected_without_open(&fake, CB62_MEMORY_PROBE_TOPOLOGY_REJECTED);

    fake_libusb_init_valid(&fake);
    fake.devices[0].interface_number = 1;
    check_rejected_without_open(&fake, CB62_MEMORY_PROBE_TOPOLOGY_REJECTED);

    fake_libusb_init_valid(&fake);
    fake.devices[0].interface_class = 0xfe;
    check_rejected_without_open(&fake, CB62_MEMORY_PROBE_TOPOLOGY_REJECTED);

    fake_libusb_init_valid(&fake);
    fake.devices[0].interface_subclass = 0xfe;
    check_rejected_without_open(&fake, CB62_MEMORY_PROBE_TOPOLOGY_REJECTED);

    fake_libusb_init_valid(&fake);
    fake.devices[0].interface_protocol = 1;
    check_rejected_without_open(&fake, CB62_MEMORY_PROBE_TOPOLOGY_REJECTED);

    fake_libusb_init_valid(&fake);
    fake.devices[0].bulk_endpoint_count = 3;
    check_rejected_without_open(&fake, CB62_MEMORY_PROBE_TOPOLOGY_REJECTED);

    fake_libusb_init_valid(&fake);
    fake.devices[0].endpoint_out = 0x02;
    check_rejected_without_open(&fake, CB62_MEMORY_PROBE_TOPOLOGY_REJECTED);

    fake_libusb_init_valid(&fake);
    fake.devices[0].endpoint_in = 0x82;
    check_rejected_without_open(&fake, CB62_MEMORY_PROBE_TOPOLOGY_REJECTED);
}

static void test_discovery_and_duplicate_refusal(void)
{
    struct fake_libusb fake;

    fake_libusb_init_valid(&fake);
    fake.discover_result = -1;
    check_rejected_without_open(&fake, CB62_MEMORY_PROBE_DISCOVERY_FAILED);

    fake_libusb_init_valid(&fake);
    fake.devices[1] = fake.devices[0];
    fake.device_count = 2;
    check_rejected_without_open(&fake,
                                CB62_MEMORY_PROBE_DEVICE_COUNT_REJECTED);
}

static void test_exact_success_trace(void)
{
    static const uint8_t expected[CB62_MEMORY_READ_REQUEST_SIZE] = {
        0x44, 0x4d, 0x43, 0x55, 0x00, 0x00, 0x00, 0x00,
        0x08, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
        0x38, 0x00, 0x40, 0x01, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    };
    struct fake_libusb fake;
    struct cb62_memory_probe_observation observation = {0};

    fake_libusb_init_valid(&fake);
    set_memory_response(&fake, CB62_MEMORY_STATUS_TOKEN,
                        CB62_MEMORY_RESPONSE_SUCCESS,
                        CB62_MEMORY_EXPECTED_VALUE);
    CHECK(cb62_probe_memory_once(fake_libusb_ops(), &fake, &observation) ==
          CB62_MEMORY_PROBE_OK);
    CHECK(fake.open_calls == 1);
    CHECK(fake.out_calls == 1);
    CHECK(fake.in_calls == 1);
    CHECK(fake.close_calls == 1);
    CHECK(fake.sent_len == sizeof(expected));
    CHECK(memcmp(fake.sent, expected, sizeof(expected)) == 0);
    CHECK(observation.request_transferred == sizeof(expected));
    CHECK(observation.response_transferred ==
          CB62_MEMORY_READ_RESPONSE_SIZE);
    CHECK(observation.response.value == CB62_MEMORY_EXPECTED_VALUE);
}

static void test_open_and_transfer_failures_have_no_retry(void)
{
    struct fake_libusb fake;
    struct cb62_memory_probe_observation observation = {0};

    fake_libusb_init_valid(&fake);
    fake.open_result = -1;
    CHECK(cb62_probe_memory_once(fake_libusb_ops(), &fake, &observation) ==
          CB62_MEMORY_PROBE_OPEN_FAILED);
    CHECK(fake.open_calls == 1);
    CHECK(fake.out_calls == 0);
    CHECK(fake.in_calls == 0);
    CHECK(fake.close_calls == 0);

    fake_libusb_init_valid(&fake);
    fake.out_result = -1;
    CHECK(cb62_probe_memory_once(fake_libusb_ops(), &fake, &observation) ==
          CB62_MEMORY_PROBE_OUT_FAILED);
    CHECK(fake.open_calls == 1);
    CHECK(fake.out_calls == 1);
    CHECK(fake.in_calls == 0);
    CHECK(fake.close_calls == 1);

    fake_libusb_init_valid(&fake);
    fake.short_out = 1;
    CHECK(cb62_probe_memory_once(fake_libusb_ops(), &fake, &observation) ==
          CB62_MEMORY_PROBE_OUT_FAILED);
    CHECK(fake.out_calls == 1);
    CHECK(fake.in_calls == 0);
    CHECK(fake.close_calls == 1);

    fake_libusb_init_valid(&fake);
    fake.in_result = -1;
    CHECK(cb62_probe_memory_once(fake_libusb_ops(), &fake, &observation) ==
          CB62_MEMORY_PROBE_IN_FAILED);
    CHECK(fake.open_calls == 1);
    CHECK(fake.out_calls == 1);
    CHECK(fake.in_calls == 1);
    CHECK(fake.close_calls == 1);

    fake_libusb_init_valid(&fake);
    fake.short_in = 1;
    CHECK(cb62_probe_memory_once(fake_libusb_ops(), &fake, &observation) ==
          CB62_MEMORY_PROBE_IN_FAILED);
    CHECK(fake.out_calls == 1);
    CHECK(fake.in_calls == 1);
    CHECK(fake.close_calls == 1);
}

static void test_bad_responses_close_without_retry(void)
{
    struct fake_libusb fake;
    struct cb62_memory_probe_observation observation = {0};

    fake_libusb_init_valid(&fake);
    set_memory_response(&fake, 0, CB62_MEMORY_RESPONSE_SUCCESS,
                        CB62_MEMORY_EXPECTED_VALUE);
    CHECK(cb62_probe_memory_once(fake_libusb_ops(), &fake, &observation) ==
          CB62_MEMORY_PROBE_RESPONSE_REJECTED);
    CHECK(fake.out_calls == 1);
    CHECK(fake.in_calls == 1);
    CHECK(fake.close_calls == 1);

    fake_libusb_init_valid(&fake);
    set_memory_response(&fake, CB62_MEMORY_STATUS_TOKEN, 1,
                        CB62_MEMORY_EXPECTED_VALUE);
    CHECK(cb62_probe_memory_once(fake_libusb_ops(), &fake, &observation) ==
          CB62_MEMORY_PROBE_RESPONSE_REJECTED);
    CHECK(fake.out_calls == 1);
    CHECK(fake.in_calls == 1);
    CHECK(fake.close_calls == 1);
}

int main(void)
{
    test_identity_and_topology_refusal();
    test_discovery_and_duplicate_refusal();
    test_exact_success_trace();
    test_open_and_transfer_failures_have_no_retry();
    test_bad_responses_close_without_retry();
    if (failures != 0) return 1;
    puts("test_memory_probe: PASS");
    return 0;
}
