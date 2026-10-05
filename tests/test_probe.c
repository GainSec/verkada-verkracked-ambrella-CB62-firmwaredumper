#include "../src/probe.h"
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

static void check_rejected_without_bulk(struct fake_libusb *f, int expected)
{
    struct cb62_probe_observation obs = {0};
    CHECK(cb62_probe_once(fake_libusb_ops(), f, &obs) == expected);
    CHECK(f->open_calls == 0);
    CHECK(f->out_calls == 0);
    CHECK(f->in_calls == 0);
    CHECK(f->close_calls == 0);
}

static void test_wrong_pid_rejected(void)
{
    struct fake_libusb f;
    fake_libusb_init_valid(&f);
    f.devices[0].pid = 0x0013;
    check_rejected_without_bulk(&f, CB62_PROBE_IDENTITY_REJECTED);
}

static void test_wrong_strings_rejected(void)
{
    struct fake_libusb f;
    fake_libusb_init_valid(&f);
    strcpy(f.devices[0].manufacturer, "Ambarella");
    check_rejected_without_bulk(&f, CB62_PROBE_IDENTITY_REJECTED);
    fake_libusb_init_valid(&f);
    strcpy(f.devices[0].product, "unexpected");
    check_rejected_without_bulk(&f, CB62_PROBE_IDENTITY_REJECTED);
}

static void test_wrong_interface_rejected(void)
{
    struct fake_libusb f;
    fake_libusb_init_valid(&f);
    f.devices[0].interface_class = 0x02;
    check_rejected_without_bulk(&f, CB62_PROBE_TOPOLOGY_REJECTED);
}

static void test_wrong_endpoints_rejected(void)
{
    struct fake_libusb f;
    fake_libusb_init_valid(&f);
    f.devices[0].endpoint_out = 0x02;
    check_rejected_without_bulk(&f, CB62_PROBE_TOPOLOGY_REJECTED);
    fake_libusb_init_valid(&f);
    f.devices[0].endpoint_in = 0x82;
    check_rejected_without_bulk(&f, CB62_PROBE_TOPOLOGY_REJECTED);
}

static void test_extra_interface_or_endpoint_rejected(void)
{
    struct fake_libusb f;
    fake_libusb_init_valid(&f);
    f.devices[0].matching_interface_count = 2;
    check_rejected_without_bulk(&f, CB62_PROBE_TOPOLOGY_REJECTED);
    fake_libusb_init_valid(&f);
    f.devices[0].bulk_endpoint_count = 3;
    check_rejected_without_bulk(&f, CB62_PROBE_TOPOLOGY_REJECTED);
}

static void test_duplicate_devices_rejected(void)
{
    struct fake_libusb f;
    fake_libusb_init_valid(&f);
    f.devices[1] = f.devices[0];
    f.device_count = 2;
    check_rejected_without_bulk(&f, CB62_PROBE_DEVICE_COUNT_REJECTED);
}

static void test_exact_accepted_trace(void)
{
    static const uint8_t expected[CB62_INQUIRY_REQUEST_SIZE] = {
        0x44, 0x4d, 0x43, 0x55, 0x04, 0x00, 0x00, 0x00,
        0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    };
    struct fake_libusb f;
    struct cb62_probe_observation obs = {0};
    fake_libusb_init_valid(&f);
    CHECK(cb62_probe_once(fake_libusb_ops(), &f, &obs) == CB62_PROBE_OK);
    CHECK(f.open_calls == 1);
    CHECK(f.out_calls == 1);
    CHECK(f.in_calls == 1);
    CHECK(f.close_calls == 1);
    CHECK(f.sent_len == sizeof(expected));
    CHECK(memcmp(f.sent, expected, sizeof(expected)) == 0);
    CHECK(obs.response.chip_id == CB62_EXPECTED_CHIP_ID);
}

static void test_short_out_stops_before_in(void)
{
    struct fake_libusb f;
    struct cb62_probe_observation obs = {0};
    fake_libusb_init_valid(&f);
    f.short_out = 1;
    CHECK(cb62_probe_once(fake_libusb_ops(), &f, &obs) == CB62_PROBE_OUT_FAILED);
    CHECK(f.out_calls == 1);
    CHECK(f.in_calls == 0);
    CHECK(f.close_calls == 1);
}

static void test_malformed_response_closes(void)
{
    struct fake_libusb f;
    struct cb62_probe_observation obs = {0};
    fake_libusb_init_valid(&f);
    memset(f.response, 0, sizeof(f.response));
    CHECK(cb62_probe_once(fake_libusb_ops(), &f, &obs) == CB62_PROBE_RESPONSE_REJECTED);
    CHECK(f.out_calls == 1);
    CHECK(f.in_calls == 1);
    CHECK(f.close_calls == 1);
}

int main(void)
{
    test_wrong_pid_rejected();
    test_wrong_strings_rejected();
    test_wrong_interface_rejected();
    test_wrong_endpoints_rejected();
    test_extra_interface_or_endpoint_rejected();
    test_duplicate_devices_rejected();
    test_exact_accepted_trace();
    test_short_out_stops_before_in();
    test_malformed_response_closes();
    if (failures != 0) return 1;
    puts("test_probe: PASS");
    return 0;
}
