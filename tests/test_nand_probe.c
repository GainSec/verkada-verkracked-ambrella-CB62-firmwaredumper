#include "../src/nand_probe.h"
#include "../src/nand_payload_blob.h"

#include <stdio.h>
#include <string.h>

struct script {
    struct cb62_usb_device dev;
    unsigned discover, open, out, in, close;
    uint8_t sent[4][CB62_NAND_MAX_TRANSFER];
    size_t sent_len[4];
    unsigned fail_out, fail_in;
};
static int failures;
#define CHECK(x) do { if (!(x)) { fprintf(stderr, "FAIL:%d: %s\n", __LINE__, #x); failures++; } } while (0)

static int discover(void *p, struct cb62_usb_device *out, size_t cap, size_t *n)
{ struct script *s=p; (void)cap; s->discover++; out[0]=s->dev; *n=1; return 0; }
static int open_claim(void *p, const struct cb62_usb_device *d)
{ (void)d; ((struct script *)p)->open++; return 0; }
static int bulk_out(void *p, uint8_t ep, const uint8_t *data, size_t len, size_t *done)
{ struct script *s=p; (void)ep; s->out++; if (s->fail_out==s->out) return -1; CHECK(s->out <= 4); memcpy(s->sent[s->out-1],data,len); s->sent_len[s->out-1]=len; *done=len; return 0; }
static int bulk_in(void *p, uint8_t ep, uint8_t *data, size_t len, size_t *done)
{ struct script *s=p; (void)ep; s->in++; if (s->fail_in==s->in) return -1; CHECK(len==16); memset(data,0,len); data[0]=0x50; data[1]=0x53; data[2]=0x52; data[3]=0x55; *done=len; return 0; }
static void close_dev(void *p) { ((struct script *)p)->close++; }
static const struct cb62_usb_ops ops={discover,open_claim,bulk_out,bulk_in,close_dev};

static void init(struct script *s)
{
    memset(s,0,sizeof(*s)); s->dev.vid=CB62_USB_VID; s->dev.pid=CB62_USB_PID;
    strcpy(s->dev.manufacturer,CB62_USB_MANUFACTURER); strcpy(s->dev.product,CB62_USB_PRODUCT);
    s->dev.matching_interface_count=1; s->dev.interface_number=0; s->dev.bulk_endpoint_count=2;
    s->dev.interface_class=0xff; s->dev.interface_subclass=0xff; s->dev.interface_protocol=0;
    s->dev.endpoint_out=1; s->dev.endpoint_in=0x81;
}

int main(void)
{
    struct script s; struct cb62_nand_observation o; init(&s);
    CHECK(cb62_upload_fixed_nand_page0_once(&ops,&s,&o)==CB62_NAND_OK);
    CHECK(s.open==1 && s.close==1 && s.out==4 && s.in==3);
    CHECK(s.sent_len[0]==32 && s.sent_len[1]==32);
    CHECK(s.sent_len[2]==cb62_nand_page0_payload_len);
    CHECK(memcmp(s.sent[2],cb62_nand_page0_payload,cb62_nand_page0_payload_len)==0);
    CHECK(s.sent_len[3]==32);
    for (unsigned fail=1; fail<=4; fail++) {
        init(&s); s.fail_out=fail;
        CHECK(cb62_upload_fixed_nand_page0_once(&ops,&s,&o)!=CB62_NAND_OK);
        CHECK(s.out==fail && s.close==1);
    }
    for (unsigned fail=1; fail<=3; fail++) {
        init(&s); s.fail_in=fail;
        CHECK(cb62_upload_fixed_nand_page0_once(&ops,&s,&o)!=CB62_NAND_OK);
        CHECK(s.in==fail && s.close==1);
    }
    if (failures) return 1;
    puts("test_nand_probe: PASS");
    return 0;
}
