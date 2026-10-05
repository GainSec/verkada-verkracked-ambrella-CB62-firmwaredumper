#include "../src/resident_probe.h"

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
    uint8_t ready_response[CB62_RESIDENT_RESPONSE_SIZE];
    uint8_t *data;
    uint8_t sent[2][CB62_RESIDENT_COMMAND_SIZE];
    size_t data_offset;
    unsigned open_calls, out_calls, in_calls, close_calls;
    unsigned fail_out_call, fail_in_call;
    size_t first_data_chunk;
    int zero_data_transfer;
};

static void put_u32le(uint8_t *p, uint32_t value)
{
    p[0] = (uint8_t)value; p[1] = (uint8_t)(value >> 8);
    p[2] = (uint8_t)(value >> 16); p[3] = (uint8_t)(value >> 24);
}

static void init_valid(struct scripted_usb *usb)
{
    memset(usb, 0, sizeof(*usb));
    usb->data = calloc(CB62_RESIDENT_FIXED_LENGTH, 1u);
    CHECK(usb->data != NULL);
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
    usb->data[0] = 0x18; usb->data[3] = 0x14;
    usb->data[0x38] = 0x09; usb->data[0x39] = 0x03;
    put_u32le(usb->ready_response + 0, CB62_RESIDENT_STATUS_TOKEN);
    put_u32le(usb->ready_response + 4, CB62_RESIDENT_RESPONSE_SUCCESS);
    put_u32le(usb->ready_response + 8, CB62_RESIDENT_FIXED_LENGTH);
    put_u32le(usb->ready_response + 12,
              cb62_resident_crc32(usb->data, CB62_RESIDENT_FIXED_LENGTH));
}

static void destroy_script(struct scripted_usb *usb) { free(usb->data); }

static int discover(void *opaque, struct cb62_usb_device *out,
                    size_t capacity, size_t *count)
{
    struct scripted_usb *usb = opaque; *count = 1;
    if (capacity) *out = usb->device;
    return 0;
}
static int open_claim(void *opaque, const struct cb62_usb_device *device)
{
    struct scripted_usb *usb = opaque; (void)device; usb->open_calls++; return 0;
}
static int bulk_out(void *opaque, uint8_t ep, const uint8_t *data,
                    size_t len, size_t *transferred)
{
    struct scripted_usb *usb = opaque; (void)ep; usb->out_calls++;
    if (usb->out_calls <= 2u) memcpy(usb->sent[usb->out_calls - 1u], data, len);
    if (usb->fail_out_call == usb->out_calls) return -1;
    *transferred = len; return 0;
}
static int bulk_in(void *opaque, uint8_t ep, uint8_t *data,
                   size_t len, size_t *transferred)
{
    struct scripted_usb *usb = opaque; (void)ep; usb->in_calls++;
    if (usb->fail_in_call == usb->in_calls) return -1;
    if (usb->in_calls == 1u) memcpy(data, usb->ready_response, len);
    else {
        if (usb->zero_data_transfer) {
            *transferred = 0;
            return 0;
        }
        size_t n = len;
        if (usb->in_calls == 2u && usb->first_data_chunk != 0u &&
            usb->first_data_chunk < n)
            n = usb->first_data_chunk;
        memcpy(data, usb->data + usb->data_offset, n);
        usb->data_offset += n;
        *transferred = n;
        return 0;
    }
    *transferred = len; return 0;
}
static void close_device(void *opaque) { ((struct scripted_usb *)opaque)->close_calls++; }
static const struct cb62_usb_ops ops = {
    discover, open_claim, bulk_out, bulk_in, close_device
};

static void test_exact_success_trace(void)
{
    struct scripted_usb usb;
    struct cb62_resident_probe_observation *observation =
        calloc(1u, sizeof(*observation));
    uint8_t expected_ready[CB62_RESIDENT_COMMAND_SIZE];
    uint8_t expected_send[CB62_RESIDENT_COMMAND_SIZE];
    CHECK(observation != NULL); init_valid(&usb);
    CHECK(cb62_probe_fixed_resident_once(&ops, &usb, observation) ==
          CB62_RESIDENT_PROBE_OK);
    cb62_build_fixed_resident_ready_request(expected_ready);
    cb62_build_resident_send_data_request(expected_send);
    CHECK(usb.open_calls == 1u && usb.out_calls == 2u && usb.in_calls == 2u &&
          usb.close_calls == 1u);
    CHECK(memcmp(usb.sent[0], expected_ready, sizeof(expected_ready)) == 0);
    CHECK(memcmp(usb.sent[1], expected_send, sizeof(expected_send)) == 0);
    CHECK(observation->data_transferred == CB62_RESIDENT_FIXED_LENGTH);
    CHECK(observation->computed_crc32 == observation->ready.crc32);
    destroy_script(&usb); free(observation);
}

static void test_bad_ready_stops_before_command3(void)
{
    struct scripted_usb usb;
    struct cb62_resident_probe_observation *observation =
        calloc(1u, sizeof(*observation));
    CHECK(observation != NULL); init_valid(&usb); usb.ready_response[4] = 1;
    CHECK(cb62_probe_fixed_resident_once(&ops, &usb, observation) ==
          CB62_RESIDENT_PROBE_READY_REJECTED);
    CHECK(usb.out_calls == 1u && usb.in_calls == 1u && usb.close_calls == 1u);
    destroy_script(&usb); free(observation);
}

static void test_second_stage_failures_do_not_retry(void)
{
    struct scripted_usb usb;
    struct cb62_resident_probe_observation *observation =
        calloc(1u, sizeof(*observation));
    CHECK(observation != NULL); init_valid(&usb); usb.fail_out_call = 2;
    CHECK(cb62_probe_fixed_resident_once(&ops, &usb, observation) ==
          CB62_RESIDENT_PROBE_SEND_DATA_OUT_FAILED);
    CHECK(usb.out_calls == 2u && usb.in_calls == 1u);
    destroy_script(&usb);
    init_valid(&usb); usb.fail_in_call = 2;
    CHECK(cb62_probe_fixed_resident_once(&ops, &usb, observation) ==
          CB62_RESIDENT_PROBE_DATA_IN_FAILED);
    CHECK(usb.out_calls == 2u && usb.in_calls == 2u);
    destroy_script(&usb); free(observation);
}

static void test_short_data_chunks_are_accumulated_without_command_retry(void)
{
    struct scripted_usb usb;
    struct cb62_resident_probe_observation *observation =
        calloc(1u, sizeof(*observation));
    CHECK(observation != NULL); init_valid(&usb); usb.first_data_chunk = 64u;
    CHECK(cb62_probe_fixed_resident_once(&ops, &usb, observation) ==
          CB62_RESIDENT_PROBE_OK);
    CHECK(usb.out_calls == 2u && usb.in_calls == 3u);
    CHECK(observation->data_transferred == CB62_RESIDENT_FIXED_LENGTH);
    CHECK(observation->computed_crc32 == observation->ready.crc32);
    destroy_script(&usb); free(observation);
}

static void test_zero_length_data_chunk_fails_without_looping(void)
{
    struct scripted_usb usb;
    struct cb62_resident_probe_observation *observation =
        calloc(1u, sizeof(*observation));
    CHECK(observation != NULL); init_valid(&usb); usb.zero_data_transfer = 1;
    CHECK(cb62_probe_fixed_resident_once(&ops, &usb, observation) ==
          CB62_RESIDENT_PROBE_DATA_IN_FAILED);
    CHECK(usb.out_calls == 2u && usb.in_calls == 2u);
    destroy_script(&usb); free(observation);
}

static void test_crc_mismatch_is_rejected(void)
{
    struct scripted_usb usb;
    struct cb62_resident_probe_observation *observation =
        calloc(1u, sizeof(*observation));
    CHECK(observation != NULL); init_valid(&usb); usb.ready_response[12] ^= 1;
    CHECK(cb62_probe_fixed_resident_once(&ops, &usb, observation) ==
          CB62_RESIDENT_PROBE_CRC_MISMATCH);
    CHECK(usb.out_calls == 2u && usb.in_calls == 2u);
    CHECK(observation->data_transferred == CB62_RESIDENT_FIXED_LENGTH);
    CHECK(cb62_resident_probe_has_complete_data(
              CB62_RESIDENT_PROBE_CRC_MISMATCH, observation));
    destroy_script(&usb); free(observation);
}

int main(void)
{
    test_exact_success_trace();
    test_bad_ready_stops_before_command3();
    test_second_stage_failures_do_not_retry();
    test_short_data_chunks_are_accumulated_without_command_retry();
    test_zero_length_data_chunk_fails_without_looping();
    test_crc_mismatch_is_rejected();
    if (failures) return 1;
    puts("test_resident_probe: PASS");
    return 0;
}
