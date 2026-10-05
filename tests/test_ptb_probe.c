#include "../src/ptb_probe.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int failures;
#define CHECK(expr) do { if (!(expr)) { \
    fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #expr); \
    failures++; \
} } while (0)

struct scripted_usb {
    struct cb62_usb_device device;
    uint8_t ready_response[CB62_PTB_RESPONSE_SIZE];
    uint8_t data[CB62_PTB_FIXED_LENGTH];
    uint8_t sent[2][CB62_PTB_COMMAND_SIZE];
    unsigned open_calls, out_calls, in_calls, close_calls;
    unsigned fail_out_call, fail_in_call;
    unsigned short_out_call, short_in_call;
    size_t discover_count;
    int discover_result, open_result;
};

static void put_u32le(uint8_t *p, uint32_t value)
{
    p[0] = (uint8_t)value; p[1] = (uint8_t)(value >> 8);
    p[2] = (uint8_t)(value >> 16); p[3] = (uint8_t)(value >> 24);
}

static void init_valid(struct scripted_usb *usb)
{
    memset(usb, 0, sizeof(*usb));
    usb->discover_count = 1u;
    usb->device.vid = CB62_USB_VID;
    usb->device.pid = CB62_USB_PID;
    usb->device.interface_number = 0;
    usb->device.matching_interface_count = 1;
    usb->device.bulk_endpoint_count = 2;
    usb->device.interface_class = CB62_USB_INTERFACE_CLASS;
    usb->device.interface_subclass = CB62_USB_INTERFACE_SUBCLASS;
    usb->device.interface_protocol = CB62_USB_INTERFACE_PROTOCOL;
    usb->device.endpoint_out = CB62_USB_ENDPOINT_OUT;
    usb->device.endpoint_in = CB62_USB_ENDPOINT_IN;
    strcpy(usb->device.manufacturer, CB62_USB_MANUFACTURER);
    strcpy(usb->device.product, CB62_USB_PRODUCT);
    usb->data[0] = 0xe6; usb->data[1] = 0xdf;
    usb->data[2] = 0x32; usb->data[3] = 0x87;
    put_u32le(usb->ready_response + 0, CB62_PTB_STATUS_TOKEN);
    put_u32le(usb->ready_response + 4, CB62_PTB_RESPONSE_SUCCESS);
    put_u32le(usb->ready_response + 8, CB62_PTB_FIXED_LENGTH);
    put_u32le(usb->ready_response + 12,
              cb62_ptb_crc32(usb->data, sizeof(usb->data)));
}

static int discover(void *opaque, struct cb62_usb_device *out,
                    size_t capacity, size_t *count)
{
    struct scripted_usb *usb = opaque; *count = usb->discover_count;
    if (capacity && usb->discover_count) *out = usb->device;
    return usb->discover_result;
}
static int open_claim(void *opaque, const struct cb62_usb_device *device)
{
    struct scripted_usb *usb = opaque; (void)device; usb->open_calls++;
    return usb->open_result;
}
static int bulk_out(void *opaque, uint8_t ep, const uint8_t *data,
                    size_t len, size_t *transferred)
{
    struct scripted_usb *usb = opaque; (void)ep; usb->out_calls++;
    if (usb->out_calls <= 2u) memcpy(usb->sent[usb->out_calls - 1u], data, len);
    if (usb->fail_out_call == usb->out_calls) return -1;
    *transferred = usb->short_out_call == usb->out_calls ? len - 1u : len;
    return 0;
}
static int bulk_in(void *opaque, uint8_t ep, uint8_t *data,
                   size_t len, size_t *transferred)
{
    struct scripted_usb *usb = opaque; (void)ep; usb->in_calls++;
    if (usb->fail_in_call == usb->in_calls) return -1;
    if (usb->in_calls == 1u) memcpy(data, usb->ready_response, len);
    else memcpy(data, usb->data, len);
    *transferred = usb->short_in_call == usb->in_calls ? len - 1u : len;
    return 0;
}
static void close_device(void *opaque) { ((struct scripted_usb *)opaque)->close_calls++; }
static const struct cb62_usb_ops ops = {
    discover, open_claim, bulk_out, bulk_in, close_device
};

static void test_exact_success_trace(void)
{
    struct scripted_usb usb;
    struct cb62_ptb_probe_observation observation;
    uint8_t expected_ready[CB62_PTB_COMMAND_SIZE];
    uint8_t expected_send[CB62_PTB_COMMAND_SIZE];
    init_valid(&usb);
    CHECK(cb62_probe_fixed_ptb_once(&ops, &usb, &observation) ==
          CB62_PTB_PROBE_OK);
    cb62_build_fixed_ptb_ready_request(expected_ready);
    cb62_build_ptb_send_data_request(expected_send);
    CHECK(usb.open_calls == 1u && usb.out_calls == 2u && usb.in_calls == 2u &&
          usb.close_calls == 1u);
    CHECK(memcmp(usb.sent[0], expected_ready, sizeof(expected_ready)) == 0);
    CHECK(memcmp(usb.sent[1], expected_send, sizeof(expected_send)) == 0);
    CHECK(observation.data_transferred == CB62_PTB_FIXED_LENGTH);
    CHECK(observation.computed_crc32 == observation.ready.crc32);
}

static void test_bad_ready_stops_before_command3(void)
{
    struct scripted_usb usb;
    struct cb62_ptb_probe_observation observation;
    init_valid(&usb); usb.ready_response[4] = 1;
    CHECK(cb62_probe_fixed_ptb_once(&ops, &usb, &observation) ==
          CB62_PTB_PROBE_READY_REJECTED);
    CHECK(usb.out_calls == 1u && usb.in_calls == 1u && usb.close_calls == 1u);
}

static void test_discovery_identity_and_topology_fail_closed(void)
{
    struct scripted_usb usb;
    struct cb62_ptb_probe_observation observation;

    init_valid(&usb); usb.discover_count = 2u;
    CHECK(cb62_probe_fixed_ptb_once(&ops, &usb, &observation) ==
          CB62_PTB_PROBE_DEVICE_COUNT_REJECTED);
    CHECK(usb.open_calls == 0u && usb.out_calls == 0u && usb.in_calls == 0u);

    init_valid(&usb); usb.device.pid ^= 1u;
    CHECK(cb62_probe_fixed_ptb_once(&ops, &usb, &observation) ==
          CB62_PTB_PROBE_IDENTITY_REJECTED);
    CHECK(usb.open_calls == 0u && usb.out_calls == 0u && usb.in_calls == 0u);

    init_valid(&usb); usb.device.endpoint_in = 0x82u;
    CHECK(cb62_probe_fixed_ptb_once(&ops, &usb, &observation) ==
          CB62_PTB_PROBE_TOPOLOGY_REJECTED);
    CHECK(usb.open_calls == 0u && usb.out_calls == 0u && usb.in_calls == 0u);

    init_valid(&usb); usb.open_result = -1;
    CHECK(cb62_probe_fixed_ptb_once(&ops, &usb, &observation) ==
          CB62_PTB_PROBE_OPEN_FAILED);
    CHECK(usb.open_calls == 1u && usb.out_calls == 0u && usb.in_calls == 0u &&
          usb.close_calls == 0u);
}

static void test_short_transfers_fail_without_retry(void)
{
    struct scripted_usb usb;
    struct cb62_ptb_probe_observation observation;

    init_valid(&usb); usb.short_out_call = 1u;
    CHECK(cb62_probe_fixed_ptb_once(&ops, &usb, &observation) ==
          CB62_PTB_PROBE_READY_OUT_FAILED);
    CHECK(usb.out_calls == 1u && usb.in_calls == 0u && usb.close_calls == 1u);

    init_valid(&usb); usb.short_in_call = 1u;
    CHECK(cb62_probe_fixed_ptb_once(&ops, &usb, &observation) ==
          CB62_PTB_PROBE_READY_IN_FAILED);
    CHECK(usb.out_calls == 1u && usb.in_calls == 1u && usb.close_calls == 1u);

    init_valid(&usb); usb.short_out_call = 2u;
    CHECK(cb62_probe_fixed_ptb_once(&ops, &usb, &observation) ==
          CB62_PTB_PROBE_SEND_DATA_OUT_FAILED);
    CHECK(usb.out_calls == 2u && usb.in_calls == 1u && usb.close_calls == 1u);

    init_valid(&usb); usb.short_in_call = 2u;
    CHECK(cb62_probe_fixed_ptb_once(&ops, &usb, &observation) ==
          CB62_PTB_PROBE_DATA_IN_FAILED);
    CHECK(usb.out_calls == 2u && usb.in_calls == 2u && usb.close_calls == 1u);
}

static void test_second_stage_failures_do_not_retry(void)
{
    struct scripted_usb usb;
    struct cb62_ptb_probe_observation observation;
    init_valid(&usb); usb.fail_out_call = 2;
    CHECK(cb62_probe_fixed_ptb_once(&ops, &usb, &observation) ==
          CB62_PTB_PROBE_SEND_DATA_OUT_FAILED);
    CHECK(usb.out_calls == 2u && usb.in_calls == 1u);
    init_valid(&usb); usb.fail_in_call = 2;
    CHECK(cb62_probe_fixed_ptb_once(&ops, &usb, &observation) ==
          CB62_PTB_PROBE_DATA_IN_FAILED);
    CHECK(usb.out_calls == 2u && usb.in_calls == 2u);
}

static void test_crc_mismatch_is_rejected(void)
{
    struct scripted_usb usb;
    struct cb62_ptb_probe_observation observation;
    init_valid(&usb); usb.ready_response[12] ^= 1;
    CHECK(cb62_probe_fixed_ptb_once(&ops, &usb, &observation) ==
          CB62_PTB_PROBE_CRC_MISMATCH);
    CHECK(usb.out_calls == 2u && usb.in_calls == 2u);
}

int main(void)
{
    test_exact_success_trace();
    test_bad_ready_stops_before_command3();
    test_discovery_identity_and_topology_fail_closed();
    test_short_transfers_fail_without_retry();
    test_second_stage_failures_do_not_retry();
    test_crc_mismatch_is_rejected();
    if (failures) return 1;
    puts("test_ptb_probe: PASS");
    return 0;
}
