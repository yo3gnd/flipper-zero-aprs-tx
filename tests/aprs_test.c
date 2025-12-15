#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "../aprs.h"

int main(void)
{
    int ok;
    int bad;
    int i;
    int j;
    char a[64];
    char b[64];
    char c[160];
    char msg[160];
    Packet p;
    const char *calls[30] = {
        "YO3GND", "YO8YL", "PJ6Y", "YP0TA", "JY1", "YO9LIG", "FL1PER", "YO3KRM",
        "YQ4M", "FL1PPR", "N0CALL", "W1AW", "APRS", "DB1XYZ", "VK2ABC", "JA1ABC",
        "F4XYZ", "M0ABC", "KJ7ABC", "YU1AAV", "HB9ABC", "OE1XUU", "LZ1KRN", "SV1ABC",
        "ZL1XYZ", "EA7XYZ", "SP3ABC", "OK1ABC", "SM0ABC", "OH2ABC",
    };

    const uint8_t ax_pj6y[] = {
        0x82, 0xA0, 0xB4, 0x8C, 0x98, 0xA0, 0x60, 0xA0, 0x94, 0x6C, 0xB2, 0x40,
        0x40, 0x61, 0x03, 0xF0, 0x3A, 0x42, 0x4C, 0x4E, 0x33, 0x20, 0x20, 0x20,
        0x20, 0x20, 0x3A, 0x70, 0x6A, 0x36, 0x79, 0x20, 0x62, 0x6C, 0x6E,
    };
    const uint8_t ax_yp0ta[] = {
        0x82, 0xA0, 0xB4, 0x8C, 0x98, 0xA0, 0x60, 0xB2, 0xA0, 0x60, 0xA8, 0x82,
        0x40, 0x61, 0x03, 0xF0, 0x3E, 0x79, 0x70, 0x30, 0x74, 0x61, 0x20, 0x75,
        0x70,
    };
    const uint8_t ax_yo3gnd[] = {
        0x82, 0xA0, 0xB4, 0x8C, 0x98, 0xA0, 0x60, 0xB2, 0x9E, 0x66, 0x8E, 0x9C,
        0x88, 0x63, 0x03, 0xF0, 0x3A, 0x59, 0x4F, 0x38, 0x59, 0x4C, 0x2D, 0x35,
        0x20, 0x20, 0x3A, 0x37, 0x33,
    };
    const uint8_t ax_jy1[] = {
        0x82, 0xA0, 0xB4, 0x8C, 0x98, 0xA0, 0x60, 0x94, 0xB2, 0x62, 0x40, 0x40,
        0x40, 0x61, 0x03, 0xF0, 0x3A, 0x59, 0x4F, 0x39, 0x4C, 0x49, 0x47, 0x2D,
        0x32, 0x20, 0x3A, 0x6A, 0x79, 0x31, 0x20, 0x68, 0x69,
    };
    const uint8_t ax_yo9lig[] = {
        0x82, 0xA0, 0xB4, 0x8C, 0x98, 0xA0, 0x60, 0xB2, 0x9E, 0x72, 0x98, 0x92,
        0x8E, 0x61, 0x03, 0xF0, 0x3A, 0x59, 0x4F, 0x33, 0x47, 0x4E, 0x44, 0x2D,
        0x31, 0x20, 0x3A, 0x6C, 0x69, 0x67, 0x20, 0x68, 0x69,
    };
    const uint8_t ax_yo8yl_pos[] = {
        0x82, 0xA0, 0xB4, 0x8C, 0x98, 0xA0, 0x60, 0xB2, 0x9E, 0x70, 0xB2, 0x98,
        0x40, 0x79, 0x03, 0xF0, 0x21, 0x34, 0x34, 0x32, 0x36, 0x2E, 0x32, 0x35,
        0x4E, 0x2F, 0x30, 0x32, 0x36, 0x30, 0x35, 0x2E, 0x34, 0x31, 0x45, 0x2D,
        0x43, 0x69, 0x73, 0x6D, 0x69, 0x67, 0x69, 0x75, 0x20, 0x50, 0x61, 0x72,
        0x6B,
    };

    const char *st_pj6y =
        "01111110010000010000010100101101001100010001100100000101000001100000010100101001"
        "00110110010011010000001000000010100001101100000000001111010111000100001000110010"
        "01110010110011000000010000000100000001000000010000000100010111000000111001010110"
        "011011001001111000000100010001100011011001110110100101010110011101111110";

    const char *st_yp0ta =
        "01111110010000010000010100101101001100010001100100000101000001100100110100000101"
        "00000110000101010100000100000010100001101100000000001111011111000100111100000111"
        "0000011000010111010000110000001001010111000001110010011010011100101111110";

    const char *st_yo3gnd =
        "01111110010000010000010100101101001100010001100100000101000001100100110101111001"
        "01100110011100010011100100010001110001101100000000001111010111001001101011110010"
        "00011100100110100011001010110100101011000000010000000100010111001110110011001100"
        "111011100111011001111110";

    const char *st_jy1 =
        "01111110010000010000010100101101001100010001100100000101000001100010100101001101"
        "01000110000000100000001000000010100001101100000000001111010111001001101011110010"
        "10011100001100101001001011100010101101000100110000000100010111000101011010011110"
        "10001100000001000001011010010110011100111001011001111110";

    const char *st_yo9lig =
        "01111110010000010000010100101101001100010001100100000101000001100100110101111001"
        "01001110000110010100100101110001100001101100000000001111010111001001101011110010"
        "11001100111000100111001000100010101101001000110000000100010111000011011010010110"
        "11100110000001000001011010010110001000110100011001111110";

    const char *st_yo8yl_pos =
        "01111110010000010000010100101101001100010001100100000101000001100100110101111001"
        "00001110010011010001100100000010100111101100000000001111100000100001011000010110"
        "00100110001101100011101000100110010101100011100101111010000001100010011000110110"
        "00000110010101100011101000010110010001100101000101011010011000010100101101100111"
        "01011011010010110111001101001011010101110000001000000101010000110010011101101011"
        "0010101010010010001111110";

    ok = 0;
    bad = 0;

    if (aprs_lat(a, sizeof(a), "0.02") == 8 && !strcmp(a, "0001.20N")) ok++;
    else { bad++; printf("bad lat 1 %s\n", a); }
    if (aprs_lon(b, sizeof(b), "-0.04") == 9 && !strcmp(b, "00002.40W")) ok++;
    else { bad++; printf("bad lon 1 %s\n", b); }


    /* bucuresti <3 */
    if (aprs_lat(a, sizeof(a), "44.437461") == 8 && !strcmp(a, "4426.25N")) ok++;
    else { bad++; printf("bad lat 2 %s\n", a); }
    if (aprs_lon(b, sizeof(b), "26.090215") == 9 && !strcmp(b, "02605.41E")) ok++;
    else { bad++; printf("bad lon 2 %s\n", b); }
    if (aprs_pos(c, sizeof(c), "Cismigiu Park", "44.437461", "26.090215") == 33 &&
        !strcmp(c, "!4426.25N/02605.41E-Cismigiu Park")) ok++;
    else { bad++; printf("bad pos cismigiu %s\n", c); }


    if (aprs_lat(a, sizeof(a), "49.058333333") == 8 && !strcmp(a, "4903.50N")) ok++;
    else { bad++; printf("bad aprs101 lat %s\n", a); }
    if (aprs_lon(b, sizeof(b), "-72.029166667") == 9 && !strcmp(b, "07201.75W")) ok++;
    else { bad++; printf("bad aprs101 lon %s\n", b); }


    if (aprs_pos(c, sizeof(c), "Null Island", "0.02", "-0.04") == 31 &&
        !strcmp(c, "!0001.20N/00002.40W-Null Island")) ok++;
    else { bad++; printf("bad pos null %s\n", c); }


    if (aprs_ll_clamp(a, sizeof(a), "123.45", 0) > 0 && !strcmp(a, "90.00000")) ok++;
    else { bad++; printf("bad clamp lat %s\n", a); }
    if (aprs_ll_clamp(b, sizeof(b), "-222.75", 1) > 0 && !strcmp(b, "-180.00000")) ok++;
    else { bad++; printf("bad clamp lon %s\n", b); }
    if (aprs_ll_clamp(a, sizeof(a), "-91.00", 0) > 0 && !strcmp(a, "-90.00000")) ok++;
    else { bad++; printf("bad clamp lat 2 %s\n", a); }
    if (aprs_ll_clamp(b, sizeof(b), "181.00", 1) > 0 && !strcmp(b, "180.00000")) ok++;
    else { bad++; printf("bad clamp lon 2 %s\n", b); }
    if (!aprs_lat(a, sizeof(a), "abc")) ok++;
    else { bad++; printf("bad invalid lat %s\n", a); }
    if (!aprs_lon(b, sizeof(b), "181.01")) ok++;
    else { bad++; printf("bad invalid lon %s\n", b); }


    if (aprs_bulletin(c, sizeof(c), 3, "pj6y bln") == (int)strlen(":BLN3     :pj6y bln") &&
        !strcmp(c, ":BLN3     :pj6y bln")) ok++;
    else { bad++; printf("bad bln pj6y %s\n", c); }
    if (aprs_packet(&p, "PJ6Y", 0, "APZFLP", 0, c)) ok++;
    else { bad++; printf("bad packet pj6y\n"); }
    if (p.ax25_len == sizeof(ax_pj6y)) ok++;
    else { bad++; printf("bad len pj6y %u\n", (unsigned)p.ax25_len); }
    for (i = 0, j = 1; i < (int)sizeof(ax_pj6y); i++) if (p.ax25[i] != ax_pj6y[i]) { j = 0; break; }
    if (j) ok++;
    else { bad++; printf("bad ax pj6y\n"); }
    if (p.stuffed_len == strlen(st_pj6y)) ok++;
    else { bad++; printf("bad stuffed len pj6y %u\n", (unsigned)p.stuffed_len); }
    for (i = 0, j = 1; i < (int)strlen(st_pj6y); i++) if (p.stuffed[i] != (uint8_t)(st_pj6y[i] == '1')) { j = 0; break; }
    if (j) ok++;
    else { bad++; printf("bad stuffed pj6y\n"); }


    if (aprs_status(c, sizeof(c), "yp0ta up") == (int)strlen(">yp0ta up") && !strcmp(c, ">yp0ta up")) ok++;
    else { bad++; printf("bad st yp0ta %s\n", c); }
    if (aprs_packet(&p, "YP0TA", 0, "APZFLP", 0, c)) ok++;
    else { bad++; printf("bad packet yp0ta\n"); }
    if (p.ax25_len == sizeof(ax_yp0ta)) ok++;
    else { bad++; printf("bad len yp0ta %u\n", (unsigned)p.ax25_len); }
    for (i = 0, j = 1; i < (int)sizeof(ax_yp0ta); i++) if (p.ax25[i] != ax_yp0ta[i]) { j = 0; break; }
    if (j) ok++;
    else { bad++; printf("bad ax yp0ta\n"); }
    if (p.stuffed_len == strlen(st_yp0ta)) ok++;
    else { bad++; printf("bad stuffed len yp0ta %u\n", (unsigned)p.stuffed_len); }
    for (i = 0, j = 1; i < (int)strlen(st_yp0ta); i++) if (p.stuffed[i] != (uint8_t)(st_yp0ta[i] == '1')) { j = 0; break; }
    if (j) ok++;
    else { bad++; printf("bad stuffed yp0ta\n"); }


    if (aprs_message(msg, sizeof(msg), "YO8YL", 5, "73") == (int)strlen(":YO8YL-5  :73") &&
        !strcmp(msg, ":YO8YL-5  :73")) ok++;
    else { bad++; printf("bad msg yo3gnd yo8yl %s\n", msg); }
    if (aprs_packet(&p, "YO3GND", 1, "APZFLP", 0, msg)) ok++;
    else { bad++; printf("bad packet yo3gnd\n"); }
    if (p.ax25_len == sizeof(ax_yo3gnd)) ok++;
    else { bad++; printf("bad len yo3gnd %u\n", (unsigned)p.ax25_len); }
    for (i = 0, j = 1; i < (int)sizeof(ax_yo3gnd); i++) if (p.ax25[i] != ax_yo3gnd[i]) { j = 0; break; }
    if (j) ok++;
    else { bad++; printf("bad ax yo3gnd\n"); }
    if (p.stuffed_len == strlen(st_yo3gnd)) ok++;
    else { bad++; printf("bad stuffed len yo3gnd %u\n", (unsigned)p.stuffed_len); }
    for (i = 0, j = 1; i < (int)strlen(st_yo3gnd); i++) if (p.stuffed[i] != (uint8_t)(st_yo3gnd[i] == '1')) { j = 0; break; }
    if (j) ok++;
    else { bad++; printf("bad stuffed yo3gnd\n"); }


    if (aprs_message(msg, sizeof(msg), "YO9LIG", 2, "jy1 hi") == (int)strlen(":YO9LIG-2 :jy1 hi") &&
        !strcmp(msg, ":YO9LIG-2 :jy1 hi")) ok++;
    else { bad++; printf("bad msg jy1 yo9lig %s\n", msg); }
    if (aprs_packet(&p, "JY1", 0, "APZFLP", 0, msg)) ok++;
    else { bad++; printf("bad packet jy1\n"); }
    if (p.ax25_len == sizeof(ax_jy1)) ok++;
    else { bad++; printf("bad len jy1 %u\n", (unsigned)p.ax25_len); }
    for (i = 0, j = 1; i < (int)sizeof(ax_jy1); i++) if (p.ax25[i] != ax_jy1[i]) { j = 0; break; }
    if (j) ok++;
    else { bad++; printf("bad ax jy1\n"); }
    if (p.stuffed_len == strlen(st_jy1)) ok++;
    else { bad++; printf("bad stuffed len jy1 %u\n", (unsigned)p.stuffed_len); }
    for (i = 0, j = 1; i < (int)strlen(st_jy1); i++) if (p.stuffed[i] != (uint8_t)(st_jy1[i] == '1')) { j = 0; break; }
    if (j) ok++;
    else { bad++; printf("bad stuffed jy1\n"); }


    if (aprs_message(msg, sizeof(msg), "YO3GND", 1, "lig hi") == (int)strlen(":YO3GND-1 :lig hi") &&
        !strcmp(msg, ":YO3GND-1 :lig hi")) ok++;
    else { bad++; printf("bad msg yo9lig yo3gnd %s\n", msg); }
    if (aprs_packet(&p, "YO9LIG", 0, "APZFLP", 0, msg)) ok++;
    else { bad++; printf("bad packet yo9lig\n"); }
    if (p.ax25_len == sizeof(ax_yo9lig)) ok++;
    else { bad++; printf("bad len yo9lig %u\n", (unsigned)p.ax25_len); }
    for (i = 0, j = 1; i < (int)sizeof(ax_yo9lig); i++) if (p.ax25[i] != ax_yo9lig[i]) { j = 0; break; }
    if (j) ok++;
    else { bad++; printf("bad ax yo9lig\n"); }
    if (p.stuffed_len == strlen(st_yo9lig)) ok++;
    else { bad++; printf("bad stuffed len yo9lig %u\n", (unsigned)p.stuffed_len); }
    for (i = 0, j = 1; i < (int)strlen(st_yo9lig); i++) if (p.stuffed[i] != (uint8_t)(st_yo9lig[i] == '1')) { j = 0; break; }
    if (j) ok++;
    else { bad++; printf("bad stuffed yo9lig\n"); }


    if (aprs_pos(msg, sizeof(msg), "Cismigiu Park", "44.437461", "26.090215") ==
            (int)strlen("!4426.25N/02605.41E-Cismigiu Park") &&
        !strcmp(msg, "!4426.25N/02605.41E-Cismigiu Park")) ok++;
    else { bad++; printf("bad pos yo8yl %s\n", msg); }
    if (aprs_packet(&p, "YO8YL", 12, "APZFLP", 0, msg)) ok++;
    else { bad++; printf("bad packet yo8yl pos\n"); }
    if (p.ax25_len == sizeof(ax_yo8yl_pos)) ok++;
    else { bad++; printf("bad len yo8yl pos %u\n", (unsigned)p.ax25_len); }
    for (i = 0, j = 1; i < (int)sizeof(ax_yo8yl_pos); i++) if (p.ax25[i] != ax_yo8yl_pos[i]) { j = 0; break; }
    if (j) ok++;
    else { bad++; printf("bad ax yo8yl pos\n"); }
    if (p.stuffed_len == strlen(st_yo8yl_pos)) ok++;
    else { bad++; printf("bad stuffed len yo8yl pos %u\n", (unsigned)p.stuffed_len); }
    for (i = 0, j = 1; i < (int)strlen(st_yo8yl_pos); i++) if (p.stuffed[i] != (uint8_t)(st_yo8yl_pos[i] == '1')) { j = 0; break; }
    if (j) ok++;
    else { bad++; printf("bad stuffed yo8yl pos\n"); }


    for (i = 0; i < 30; i++)
    {
        uint8_t dst_ax[7];
        uint8_t src_ax[7];
        uint8_t sa;
        uint8_t da;
        const char *src;
        const char *dst;
        const char *txt;
        int n;

        src = calls[i];
        dst = calls[(i + 7) % 30];
        txt = calls[(i + 11) % 30];
        sa = i & 15;
        da = (i + 3) & 15;

        n = aprs_message(msg, sizeof(msg), dst, da, txt);
        if (n > 0) ok++;
        else { bad++; printf("bad loop msg %d\n", i); continue; }

        if (aprs_packet(&p, src, sa, "APZFLP", 0, msg)) ok++;
        else { bad++; printf("bad loop packet %d\n", i); continue; }

        if (p.payload_len == (uint16_t)n && !memcmp(p.payload, msg, p.payload_len)) ok++;
        else { bad++; printf("bad loop payload %d\n", i); }

        for (j = 0; j < 6; j++) dst_ax[j] = ((uint8_t)"APZFLP"[j]) << 1;
        dst_ax[6] = 0x60;

        for (j = 0; j < 6; j++)
        {
            if (j < (int)strlen(src)) src_ax[j] = ((uint8_t)src[j]) << 1;
            else src_ax[j] = ' ' << 1;
        }
        src_ax[6] = 0x60 | ((sa & 15) << 1) | 1;

        for (j = 0; j < 7; j++) if (p.ax25[j] != dst_ax[j]) break;
        if (j == 7) ok++;
        else { bad++; printf("bad loop dst ax %d\n", i); }

        for (j = 0; j < 7; j++) if (p.ax25[7 + j] != src_ax[j]) break;
        if (j == 7) ok++;
        else { bad++; printf("bad loop src ax %d\n", i); }

        if (p.ax25[14] == 0x03 && p.ax25[15] == 0xF0) ok++;
        else { bad++; printf("bad loop ctlpid %d\n", i); }

        if (p.fcs_len == p.ax25_len + 2) ok++;
        else { bad++; printf("bad loop fcslen %d\n", i); }

        if (p.stuffed_len > p.fcs_len && p.nrzi_len == p.stuffed_len) ok++;
        else { bad++; printf("bad loop stream len %d\n", i); }

        if (p.stuffed[0] == 0 && p.stuffed[1] == 1 && p.stuffed[2] == 1 &&
            p.stuffed[p.stuffed_len - 1] == 0) ok++;
        else { bad++; printf("bad loop flags %d\n", i); }
    }


    printf("ok=%d bad=%d\n", ok, bad);
    if (bad) return 1;
    return 0;
}
