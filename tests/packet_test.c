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

static void packet_test_dump_bytes(const uint8_t* data, uint16_t len)
{
    uint16_t i;

    for(i = 0; i < len; i++) 
    {
        printf("%02X", data[i]);
        if(i + 1 < len) printf(" ");
    }
    printf("----\n");
}

static void packet_test_dump_bits(const uint8_t* data, uint16_t len)
{
    uint16_t i;

    for(i = 0; i < len; i++) {
        putchar(data[i] ? '1' : '0');
    }
    printf("\n");
}

static void packet_test_payload_mockup(void)
{
    Packet p;

    packet_init(&p);
    packet_make_payload(&p, "TEST123");

    printf("payload lungime=%u\n", p.payload_len);
    packet_test_dump_bytes(p.payload, p.payload_len);
}

static void packet_test_ax25_mockup(void)
{
    Packet p;

    packet_init(&p);
    packet_make_payload(&p, "TEST123");
    packet_make_ax25(&p, "YO0FLP", 0, "W0RLD", 0);

    printf("ax25 len=%u\n", p.ax25_len);
    packet_test_dump_bytes(p.ax25, p.ax25_len);
}

static void packet_test_fcs_mockup(void)
{
    Packet p;

    packet_init(&p);
    packet_make_payload(&p, "TEST123");
    packet_make_ax25(&p, "YO0FLP", 0, "W0RLD", 0);
    packet_add_fcs(&p);

    printf("fcs lungime=%u\n", p.fcs_len);
    packet_test_dump_bytes(p.fcs, p.fcs_len);
}

static void packet_test_stuff_mockup(void)
{
    Packet p;

    packet_init(&p);
    packet_make_payload(&p, "TEST123");
    packet_make_ax25(&p, "YO0FLP", 0, "W0RLD", 0);
    packet_add_fcs(&p);
    packet_stuff(&p);

    printf("stuffed bits=%u\n", p.stuffed_len);
    packet_test_dump_bits(p.stuffed, p.stuffed_len);
}

static void packet_test_nrzi_mockup(void)
{
    Packet p;

    packet_init(&p);
    packet_make_payload(&p, "TEST123");
    packet_make_ax25(&p, "YO0FLP", 0, "W0RLD", 0);
    packet_add_fcs(&p);
    packet_stuff(&p);
    packet_nrzi(&p);

    printf("nrzi biti=%u\n", p.nrzi_len);
    packet_test_dump_bits(p.nrzi, p.nrzi_len);
}

static void packet_test_all_mockup(void)
{
    Packet p;

    packet_do_all(&p, "YO0FLP", 0, "W0RLD", 0, "TEST123");



    // printf("all payload=%u ax25=%u fcs=%u \n", p.payload_len, p.ax25_len, p.fcs_len);
    printf("all payload=%u ax25=%u fcs=%u stuffed=%u nrzi=%u\n", p.payload_len, p.ax25_len, p.fcs_len, p.stuffed_len, p.nrzi_len);
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
    for(int i = 0; i < sizeof(packet_tests) / sizeof(packet_tests[0]); i++) 
    {
        printf("== %s ==\n", packet_tests[i].name);
        packet_tests[i].fn();
    }

    return 0;
}
