#include <stdint.h>
#include <stdio.h>
#include <string.h>

typedef struct {
    uint8_t payload[96];
    uint16_t payload_len;

    uint8_t ax25[192];
    uint16_t ax25_len;

    uint8_t fcs[194];
    uint16_t fcs_len;

    uint8_t stuffed[1800];
    uint16_t stuffed_len;

    uint8_t nrzi[1800];


    uint16_t nrzi_len;
} Packet;

void packet_init(Packet* p);
void packet_make_payload(Packet* p, const char* s);
void packet_make_ax25(Packet* p, const char* from, uint8_t from_ssid, const char* to, uint8_t to_ssid);
void packet_add_fcs(Packet* p);
void packet_stuff(Packet* p);


void packet_nrzi(Packet* p);

void packet_do_all(Packet* p, const char* from, uint8_t from_ssid, const char* to, uint8_t to_ssid, const char* s);

typedef struct 
{
    const char* name;
    void (*fn)(void);
} PacketTest;

static int testok = 0;
static int testbad = 0;

static void packet_test_u16(const char* name, uint16_t got, uint16_t want)
{
    if(got == want) 
    {
        testok++;
        printf("ok %s\n", name);
    }
    else
    {
        testbad++;
        printf("bad %s got=%u want=%u\n", name, got, want);
    }
}

static void packet_test_bytes(const char* name, const uint8_t* data, uint16_t len, const uint8_t* want, uint16_t wantlen)
{
    uint16_t i;

    if(len != wantlen) 
    {
        testbad++;
        printf("bad %s len=%u want=%u\n", name, len, wantlen);
        return;
    }

    for(i = 0; i < len; i++) 
    {
        if(data[i] != want[i]) 
        {
            testbad++;
            printf("bad %s la=%u got=%02X want=%02X\n", name, i, data[i], want[i]);
            return;
        }
    }

    testok++;
    printf("ok %s\n", name);
}

static void packet_test_bits(const char* name, const uint8_t* data, uint16_t len, const char* want)
{
    uint16_t i;
    uint16_t wantlen = (uint16_t)strlen(want);

    if(len != wantlen) {
        testbad++;
        printf("bad %s len=%u want=%u\n", name, len, wantlen);
        return;
    }

    for(i = 0; i < len; i++) {
        if(data[i] != (uint8_t)(want[i] == '1')) {
            testbad++;
            printf("bad %s la=%u got=%u want=%c\n", name, i, data[i], want[i]);
            return;
        }
    }

    testok++;
    printf("ok %s\n", name);
}

static void packet_test_payload_mockup(void)
{
    static const uint8_t want[] = {
        0x3E, 0x54, 0x45, 0x53, 0x54, 0x31, 0x32, 0x33,
    };
    Packet p;

    packet_init(&p);
    packet_make_payload(&p, "TEST123");

    packet_test_u16("payload lungime", p.payload_len, sizeof(want));
    packet_test_bytes("payload bytes", p.payload, p.payload_len, want, sizeof(want));
}

static void packet_test_ax25_mockup(void)
{
    static const uint8_t want[] = {
        0xAE, 0x60, 0xA4, 0x98, 0x88, 0x40, 0x60, 0xB2,
        0x9E, 0x60, 0x8C, 0x98, 0xA0, 0x61, 0x03, 0xF0,
        0x3E, 0x54, 0x45, 0x53, 0x54, 0x31, 0x32, 0x33,
    };
    Packet p;

    packet_init(&p);
    packet_make_payload(&p, "TEST123");
    packet_make_ax25(&p, "YO0FLP", 0, "W0RLD", 0);

    packet_test_u16("ax25 len", p.ax25_len, sizeof(want));
    packet_test_bytes("ax25 bytes", p.ax25, p.ax25_len, want, sizeof(want));
}

static void packet_test_fcs_mockup(void)
{
    static const uint8_t want[] = {
        0xAE, 0x60, 0xA4, 0x98, 0x88, 0x40, 0x60, 0xB2,
        0x9E, 0x60, 0x8C, 0x98, 0xA0, 0x61, 0x03, 0xF0,
        0x3E, 0x54, 0x45, 0x53, 0x54, 0x31, 0x32, 0x33,
        0x57, 0x51,
    };
    Packet p;

    packet_init(&p);
    packet_make_payload(&p, "TEST123");
    packet_make_ax25(&p, "YO0FLP", 0, "W0RLD", 0);
    packet_add_fcs(&p);

    packet_test_u16("fcs lungime", p.fcs_len, sizeof(want));
    packet_test_bytes("fcs bytes", p.fcs, p.fcs_len, want, sizeof(want));
}

static void packet_test_stuff_mockup(void)
{
    static const char* want =
        "011111100111010100000110001001010001100100010001000000100000011001001101"
        "011110010000011000110001000110010000010110000110110000000000111101111100"
        "000101010101000101100101000101010100011000100110011001100111010101000101"
        "001111110";
    Packet p;

    packet_init(&p);
    packet_make_payload(&p, "TEST123");
    packet_make_ax25(&p, "YO0FLP", 0, "W0RLD", 0);
    packet_add_fcs(&p);
    packet_stuff(&p);

    packet_test_u16("stuffed bits", p.stuffed_len, 225);
    packet_test_bits("stuffed data", p.stuffed, p.stuffed_len, want);
}

static void packet_test_nrzi_mockup(void)
{
    static const char* want =
        "000000010000110010101110100100110100010010110100101010010101000100100011"
        "000001001010111010001011010001001010110001010001110101010101111100000010"
        "101100110011010011101100101100110010111010010001000100010000110011010011"
        "011111110";
    Packet p;

    packet_init(&p);
    packet_make_payload(&p, "TEST123");
    packet_make_ax25(&p, "YO0FLP", 0, "W0RLD", 0);
    packet_add_fcs(&p);
    packet_stuff(&p);
    packet_nrzi(&p);

    packet_test_u16("nrzi biti", p.nrzi_len, 225);
    packet_test_bits("nrzi data", p.nrzi, p.nrzi_len, want);
}

static void packet_test_all_mockup(void)
{
    Packet p;

    packet_do_all(&p, "YO0FLP", 0, "W0RLD", 0, "TEST123");



    packet_test_u16("all payload", p.payload_len, 8);
    packet_test_u16("all ax25", p.ax25_len, 24);
    packet_test_u16("all fcs", p.fcs_len, 26);
    packet_test_u16("all stuffed", p.stuffed_len, 225);
    packet_test_u16("all nrzi", p.nrzi_len, 225);
}

static const PacketTest packet_tests[] = {
    { "payload", packet_test_payload_mockup },
    { "ax25", packet_test_ax25_mockup },
    { "fcs", packet_test_fcs_mockup },
    { "stuff", packet_test_stuff_mockup },
    { "nrzi", packet_test_nrzi_mockup },
    { "all", packet_test_all_mockup },
};

int main(void)
{
    uint16_t i;

    for(i = 0; i < sizeof(packet_tests) / sizeof(packet_tests[0]); i++) 
    {
        printf("== %s ==\n", packet_tests[i].name);
        packet_tests[i].fn();
    }

    printf("ok=%d bad=%d\n", testok, testbad);
    if(testbad) return 1;


    return 0;
}
