#include <stdint.h>
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

static void packet_addr(uint8_t* out, const char* call, uint8_t ssid, uint8_t last)
{
    int i;

    for(i = 0; i < 6; i++) 
    {
        if(call[i]) out[i] = ((uint8_t)call[i]) << 1;
        else out[i] = ' ' << 1;
    }

    out[6] = 0x60 | ((ssid & 15) << 1) | (last ? 1 : 0);
}

static uint16_t packet_crc(const uint8_t* data, uint16_t len)
{
    uint16_t crc = 0xFFFF;
    uint16_t i;
    uint8_t j;

    for(i = 0; i < len; i++) {
        crc ^= data[i];
        for(j = 0; j < 8; j++) {
            if(crc & 1) crc = (crc >> 1) ^ 0x8408;
            else crc >>= 1;
        }
    }

    return crc ^ 0xFFFF;
}

static void packet_push_byte_bits(uint8_t* out, uint16_t* n, uint8_t v)
{
    uint8_t i;

    for(i = 0; i < 8; i++) {
        out[*n] = (v >> i) & 1;
        (*n)++;
    }
}

void packet_init(Packet* p)
{
    memset(p, 0, sizeof(Packet));
}

void packet_make_payload(Packet* p, const char* s)
{
    uint16_t i = 0;

    p->payload[0] = '>';
    p->payload_len = 1;

    while(s[i] && p->payload_len < sizeof(p->payload)) {
        p->payload[p->payload_len++] = (uint8_t)s[i];
        i++;
    }
}

void packet_make_ax25(Packet* p, const char* from, uint8_t from_ssid, const char* to, uint8_t to_ssid)
{
    uint16_t i;

    packet_addr(p->ax25 + 0, to, to_ssid, 0);
    packet_addr(p->ax25 + 7, from, from_ssid, 1);
    p->ax25[14] = 0x03;
    p->ax25[15] = 0xF0;
    p->ax25_len = 16;

    for(i = 0; i < p->payload_len && p->ax25_len < sizeof(p->ax25); i++) {
        p->ax25[p->ax25_len++] = p->payload[i];
    }
}

void packet_add_fcs(Packet* p)
{
    uint16_t crc;

    memcpy(p->fcs, p->ax25, p->ax25_len);
    p->fcs_len = p->ax25_len;

    crc = packet_crc(p->ax25, p->ax25_len);
    p->fcs[p->fcs_len++] = crc & 255;



    p->fcs[p->fcs_len++] = crc >> 8;
}

void packet_stuff(Packet* p)
{
    uint16_t i;
    uint16_t n = 0;
    uint8_t j;
    uint8_t c = 0;

    uint8_t bit;

    packet_push_byte_bits(p->stuffed, &n, 0x7E);

    for(i = 0; i < p->fcs_len; i++) {
        for(j = 0; j < 8; j++) {
            bit = (p->fcs[i] >> j) & 1;
            p->stuffed[n++] = bit;
            if(bit) {
                c++;
                if(c == 5) {
                    p->stuffed[n++] = 0;
                    c = 0;
                }
            } else {
                c = 0;
            }}}

    packet_push_byte_bits(p->stuffed, &n, 0x7E);
    p->stuffed_len = n;
}

void packet_nrzi(Packet* p)
{
    uint16_t i;
    uint8_t level = 1;

    p->nrzi_len = 0;

    for(i = 0; i < p->stuffed_len; i++) {
        if(p->stuffed[i] == 0) level ^= 1;
        p->nrzi[p->nrzi_len++] = level;
    }
}

void packet_do_all(Packet* p, const char* from, uint8_t from_ssid, const char* to, uint8_t to_ssid, const char* s)
{
    packet_init(p);
    packet_make_payload(p, s);
    packet_make_ax25(p, from, from_ssid, to, to_ssid);
    packet_add_fcs(p);
    packet_stuff(p);
    packet_nrzi(p);
}
