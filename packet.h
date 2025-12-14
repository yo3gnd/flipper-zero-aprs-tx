#pragma once

#include <stdint.h>

typedef struct
{
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

void packet_make_ax25(Packet *p, const char *from, uint8_t from_ssid, const char *to,
                      uint8_t to_ssid);
void packet_add_fcs(Packet *p);
void packet_init(Packet *p);

void packet_stuff(Packet *p);
void packet_nrzi(Packet *p);

void packet_make_payload(Packet *p, const char *s);
void packet_do_all(Packet *p, const char *from, uint8_t from_ssid, const char *to, uint8_t to_ssid,
                   const char *s);
