#define UNITY_INCLUDE_PRINT_FORMATTED

#include "unity/unity.h"
#include "../src/aprs.h"

#include <stdio.h>
#include <stdint.h>
#include <string.h>

#ifndef RUN_TEST
#define RUN_TEST(func) UnityDefaultTestRun(func, #func, __LINE__)
#endif

void setUp(void) {}
void tearDown(void) {}
void suiteSetUp(void) {}
int suiteTearDown(int num_failures) { return num_failures; }

static void test_lat_lon_and_pos(void)
{
    char a[64];
    char b[64];
    char c[160];

    /* null first */
    UNITY_SET_DETAILS("aprs", "ll");
    TEST_ASSERT_EQUAL_INT(8, aprs_lat(a, sizeof(a), "0.02"));
    TEST_ASSERT_EQUAL_STRING("0001.20N", a);
    TEST_ASSERT_EQUAL_INT(9, aprs_lon(b, sizeof(b), "-0.04"));
    TEST_ASSERT_EQUAL_STRING("00002.40W", b);

    /* bucharest, yep, still here. */
    TEST_ASSERT_EQUAL_INT(8, aprs_lat(a, sizeof(a), "44.437461"));
    TEST_ASSERT_EQUAL_STRING("4426.25N", a);
    TEST_ASSERT_EQUAL_INT(9, aprs_lon(b, sizeof(b), "26.090215"));
    TEST_ASSERT_EQUAL_STRING("02605.41E", b);


    TEST_ASSERT_EQUAL_INT(33, aprs_pos(c, sizeof(c), "Cismigiu Park", "44.437461", "26.090215"));
    TEST_ASSERT_EQUAL_STRING("!4426.25N/02605.41E-Cismigiu Park", c);
    TEST_ASSERT_EQUAL_INT(31, aprs_pos(c, sizeof(c), "Null Island", "0.02", "-0.04"));
    TEST_ASSERT_EQUAL_STRING("!0001.20N/00002.40W-Null Island", c);
}

static void test_aprs101_pair(void)
{
    char a[64];
    char b[64];

    UNITY_SET_DETAILS("aprs", "101");
    TEST_ASSERT_EQUAL_INT(8, aprs_lat(a, sizeof(a), "49.058333333"));
    TEST_ASSERT_EQUAL_STRING("4903.50N", a);


    TEST_ASSERT_EQUAL_INT(9, aprs_lon(b, sizeof(b), "-72.029166667"));
    TEST_ASSERT_EQUAL_STRING("07201.75W", b);
}

static void test_clamps_and_invalid(void)
{
    char a[64];
    char b[64];

    /* no wild coords */
    UNITY_SET_DETAILS("aprs", "clamp");
    TEST_ASSERT_TRUE(aprs_ll_clamp(a, sizeof(a), "123.45", 0) > 0);
    TEST_ASSERT_EQUAL_STRING("90.00000", a);
    TEST_ASSERT_TRUE(aprs_ll_clamp(b, sizeof(b), "-222.75", 1) > 0);
    TEST_ASSERT_EQUAL_STRING("-180.00000", b);
    TEST_ASSERT_TRUE(aprs_ll_clamp(a, sizeof(a), "-91.00", 0) > 0);
    TEST_ASSERT_EQUAL_STRING("-90.00000", a);
    TEST_ASSERT_TRUE(aprs_ll_clamp(b, sizeof(b), "181.00", 1) > 0);
    TEST_ASSERT_EQUAL_STRING("180.00000", b);


    TEST_ASSERT_FALSE(aprs_lat(a, sizeof(a), "abc"));
    TEST_ASSERT_FALSE(aprs_lon(b, sizeof(b), "181.01"));
}

static void test_bulletin_and_status_strings(void)
{
    char c[160];

    // hello Saba
    UNITY_SET_DETAILS("aprs", "payloads");
    TEST_ASSERT_EQUAL_INT((int)strlen(":BLN3     :pj6y bln"), aprs_bulletin(c, sizeof(c), 3, "pj6y bln"));
    TEST_ASSERT_EQUAL_STRING(":BLN3     :pj6y bln", c);

    TEST_ASSERT_EQUAL_INT((int)strlen(">yp0ta up"), aprs_status(c, sizeof(c), "yp0ta up"));
    TEST_ASSERT_EQUAL_STRING(">yp0ta up", c);
}

static void test_bulletin_packet_pj6y(void)
{
    char c[160];
    Packet p;
    uint8_t want[355];
    uint16_t i;
    static const uint8_t ax_pj6y[] = {
        0x82, 0xA0, 0xB4, 0x8C, 0x98, 0xA0, 0x60, 0xA0, 0x94, 0x6C, 0xB2, 0x40,
        0x40, 0x61, 0x03, 0xF0, 0x3A, 0x42, 0x4C, 0x4E, 0x33, 0x20, 0x20, 0x20,
        0x20, 0x20, 0x3A, 0x70, 0x6A, 0x36, 0x79, 0x20, 0x62, 0x6C, 0x6E,
    };
    static const char *st_pj6y =
        "01111110010000010000010100101101001100010001100100000101000001100000010100101001"
        "00110110010011010000001000000010100001101100000000001111010111000100001000110010"
        "01110010110011000000010000000100000001000000010000000100010111000000111001010110"
        "011011001001111000000100010001100011011001110110100101010110011101111110";

    /* bulleting */
    UNITY_SET_DETAILS("aprs", "pj6y");
    TEST_ASSERT_EQUAL_INT((int)strlen(":BLN3     :pj6y bln"), aprs_bulletin(c, sizeof(c), 3, "pj6y bln"));
    TEST_ASSERT_TRUE(aprs_packet(&p, "PJ6Y", 0, "APZFLP", 0, c, NULL));
    TEST_ASSERT_EQUAL_UINT16(sizeof(ax_pj6y), p.ax25_len);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(ax_pj6y, p.ax25, sizeof(ax_pj6y));
    TEST_ASSERT_EQUAL_UINT16((uint16_t)strlen(st_pj6y), p.stuffed_len);


    for (i = 0; i < (uint16_t)strlen(st_pj6y); i++) want[i] = (uint8_t)(st_pj6y[i] == '1');
    TEST_ASSERT_EQUAL_UINT8_ARRAY(want, p.stuffed, p.stuffed_len);
}

static void test_status_packet_yp0ta(void)
{
    char c[160];
    Packet p;
    static const uint8_t ax_yp0ta[] = {
        0x82, 0xA0, 0xB4, 0x8C, 0x98, 0xA0, 0x60, 0xB2, 0xA0, 0x60, 0xA8, 0x82,
        0x40, 0x61, 0x03, 0xF0, 0x3E, 0x79, 0x70, 0x30, 0x74, 0x61, 0x20, 0x75,
        0x70,
    };

    /* status, barebones */
    UNITY_SET_DETAILS("aprs", "yp0ta");
    TEST_ASSERT_EQUAL_INT((int)strlen(">yp0ta up"), aprs_status(c, sizeof(c), "yp0ta up"));
    TEST_ASSERT_TRUE(aprs_packet(&p, "YP0TA", 0, "APZFLP", 0, c, NULL));
    TEST_ASSERT_EQUAL_UINT16(sizeof(ax_yp0ta), p.ax25_len);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(ax_yp0ta, p.ax25, sizeof(ax_yp0ta));
    TEST_ASSERT_EQUAL_HEX8(0x03, p.ax25[14]);
    TEST_ASSERT_EQUAL_HEX8(0xF0, p.ax25[15]);
}

static void test_one_message_packet(void)
{
    char msg[160];
    Packet p;
    static const uint8_t ax_yo3gnd[] = {
        0x82, 0xA0, 0xB4, 0x8C, 0x98, 0xA0, 0x60, 0xB2, 0x9E, 0x66, 0x8E, 0x9C,
        0x88, 0x63, 0x03, 0xF0, 0x3A, 0x59, 0x4F, 0x38, 0x59, 0x4C, 0x2D, 0x35,
        0x20, 0x20, 0x3A, 0x37, 0x33,
    };

    /* packet says 73 */
    UNITY_SET_DETAILS("aprs", "yo3gnd");
    TEST_ASSERT_EQUAL_INT((int)strlen(":YO8YL-5  :73"), aprs_message(msg, sizeof(msg), "YO8YL", 5, "73"));
    TEST_ASSERT_EQUAL_STRING(":YO8YL-5  :73", msg);
    TEST_ASSERT_TRUE(aprs_packet(&p, "YO3GND", 1, "APZFLP", 0, msg, NULL));
    TEST_ASSERT_EQUAL_UINT16(sizeof(ax_yo3gnd), p.ax25_len);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(ax_yo3gnd, p.ax25, sizeof(ax_yo3gnd));
    TEST_ASSERT_EQUAL_UINT16((uint16_t)strlen(":YO8YL-5  :73"), p.payload_len);
    TEST_ASSERT_EQUAL_HEX8(0x03, p.ax25[14]);
    TEST_ASSERT_EQUAL_HEX8(0xF0, p.ax25[15]);
}

static void test_pos_packet_yo8yl(void)
{
    char msg[160];
    Packet p;
    uint8_t want[591];
    uint16_t i;
    static const uint8_t ax_yo8yl_pos[] = {
        0x82, 0xA0, 0xB4, 0x8C, 0x98, 0xA0, 0x60, 0xB2, 0x9E, 0x70, 0xB2, 0x98,
        0x40, 0x79, 0x03, 0xF0, 0x21, 0x34, 0x34, 0x32, 0x36, 0x2E, 0x32, 0x35,
        0x4E, 0x2F, 0x30, 0x32, 0x36, 0x30, 0x35, 0x2E, 0x34, 0x31, 0x45, 0x2D,
        0x43, 0x69, 0x73, 0x6D, 0x69, 0x67, 0x69, 0x75, 0x20, 0x50, 0x61, 0x72,
        0x6B,
    };
    static const char *st_yo8yl_pos =
        "01111110010000010000010100101101001100010001100100000101000001100100110101111001"
        "00001110010011010001100100000010100111101100000000001111100000100001011000010110"
        "00100110001101100011101000100110010101100011100101111010000001100010011000110110"
        "00000110010101100011101000010110010001100101000101011010011000010100101101100111"
        "01011011010010110111001101001011010101110000001000000101010000110010011101101011"
        "0010101010010010001111110";

    /* pos packet, full-ish */
    UNITY_SET_DETAILS("aprs", "yo8yl");
    TEST_ASSERT_EQUAL_INT((int)strlen("!4426.25N/02605.41E-Cismigiu Park"),
        aprs_pos(msg, sizeof(msg), "Cismigiu Park", "44.437461", "26.090215"));
    TEST_ASSERT_TRUE(aprs_packet(&p, "YO8YL", 12, "APZFLP", 0, msg, NULL));
    TEST_ASSERT_EQUAL_UINT16(sizeof(ax_yo8yl_pos), p.ax25_len);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(ax_yo8yl_pos, p.ax25, sizeof(ax_yo8yl_pos));
    TEST_ASSERT_EQUAL_UINT16((uint16_t)strlen(st_yo8yl_pos), p.stuffed_len);
    for (i = 0; i < (uint16_t)strlen(st_yo8yl_pos); i++) want[i] = (uint8_t)(st_yo8yl_pos[i] == '1');
    TEST_ASSERT_EQUAL_UINT8_ARRAY(want, p.stuffed, p.stuffed_len);
}

static void test_loopish_calls(void)
{
    const char *win[] = {
        "YO3GND", "YO8YL", "YO9LIG", "YO3KRM", "PJ6Y", "YP0TA",

        "DL0LOL", "GB0OTY", "GB4NGS", "HA6RID", "SO2NIC", "3Z3Z3Z", "YO0F",
        "YP3PE", "VB2CAP", "K0K", "K4B", "K3T", "DU0W0",

        "OO0F", "OR4L", "GB0NGO", "G4SSY", "G1SUS", "TM5OON", "VA6INA",
        "VC7F", "K3K", "K4T", "N9E", "W4L", "K1D", "K1L", "W3B",

        "GB4LLS", "GB0NPC", "TM0GUS", "TM4RIO", "VB4LLS", "W4P", "N1O", "W0W",

        "TM0WO",
    };
    const char *lose[] = {
        "DC0NORRIS", "OE01MIKU", "SQ6FMBY", "HF0FURRY", "SP0FENNEC", "HF0AAAAAA",
        "SP6ZOLW", "VC2NORRIS", "VB2CATGIRL", "DU0LINGO",

        "DF4CEPALM", "PD6SENPAI", "OE51DEEYE", "7S0BRUH", "YR1CKROLL", "EG1TBDICS",
        "VB4TESTING", "VB2THEGAME", "VB2KILOWAT", "VC7ZALGO", "VC7HORSE",

        "DL0NGCAT", "PD33ZDOGE", "TM0NKAS", "VB1FLIP", "VB3BACON", "VB6WOKE",
        "VC7BEPIS", "VC9FEMBOY", "VC2STONKS", "VC3PWEOR", "DZ2NUTS",

        "TM1SSOU", "TM0RBIN", "VB4LIGMA", "VB3YEET", "VC9CATGIRL",
        "VC3DEEZ", "VB3HARAMBE", "VC3RIKROLL", "VB6DANK",
    };
    Packet p;
    char msg[160];
    char payload[160];
    int i;
    int win_n;
    int lose_n;

    /* these should be fine */
    UNITY_SET_DETAILS("aprs", "loop");
    win_n = (int)(sizeof(win) / sizeof(win[0]));
    lose_n = (int)(sizeof(lose) / sizeof(lose[0]));

    for (i = 0; i < win_n; i++)
    {
        uint8_t sa = i & 15;
        const char *src = win[i];
        const char *dst = win[(i + 7) % win_n];
        const char *txt = win[(i + 11) % win_n];
        int n;

        n = aprs_message(msg, sizeof(msg), dst, (i + 3) & 7, txt);
        TEST_ASSERT_TRUE(n > 0);
        TEST_ASSERT_TRUE(aprs_packet(&p, src, sa, "APZFLP", 0, msg, NULL));
        TEST_ASSERT_EQUAL_UINT16((uint16_t)n, p.payload_len);
        TEST_ASSERT_EQUAL_UINT8_ARRAY((uint8_t*)msg, p.payload, p.payload_len);
        TEST_ASSERT_EQUAL_HEX8(0x03, p.ax25[14]);
        TEST_ASSERT_EQUAL_HEX8(0xF0, p.ax25[15]);
        TEST_ASSERT_TRUE(p.fcs_len == p.ax25_len + 2);
        TEST_ASSERT_TRUE(p.stuffed_len > p.fcs_len);
        TEST_ASSERT_TRUE(p.nrzi_len == p.stuffed_len);
        TEST_ASSERT_EQUAL_UINT8(0, p.stuffed[0]);
        TEST_ASSERT_EQUAL_UINT8(1, p.stuffed[1]);
        TEST_ASSERT_EQUAL_UINT8(1, p.stuffed[2]);
        TEST_ASSERT_EQUAL_UINT8(0, p.stuffed[p.stuffed_len - 1]);

        TEST_ASSERT_EQUAL_HEX8(0x82, p.ax25[0]);
        TEST_ASSERT_EQUAL_HEX8(0xA0, p.ax25[1]);
    }


    /* these should not */
    for (i = 0; i < lose_n; i++)
    {
        snprintf(payload, sizeof(payload), ">%s", lose[i]);
        TEST_ASSERT_EQUAL_INT(0, aprs_message(msg, sizeof(msg), lose[i], (i + 3) & 7, "NOPE"));
        TEST_ASSERT_FALSE(aprs_packet(&p, lose[i], i & 15, "APZFLP", 0, payload, NULL));
        TEST_ASSERT_FALSE(aprs_packet(&p, "YO3GND", i & 15, lose[i], 0, payload, NULL));
    }
}

int main(void)
{
    suiteSetUp();
    UnityBegin(__FILE__);
    RUN_TEST(test_lat_lon_and_pos);
    RUN_TEST(test_aprs101_pair);
    RUN_TEST(test_clamps_and_invalid);
    RUN_TEST(test_bulletin_and_status_strings);
    RUN_TEST(test_bulletin_packet_pj6y);
    RUN_TEST(test_status_packet_yp0ta);
    RUN_TEST(test_one_message_packet);
    RUN_TEST(test_pos_packet_yo8yl);
    RUN_TEST(test_loopish_calls);
    return suiteTearDown(UnityEnd());
}
