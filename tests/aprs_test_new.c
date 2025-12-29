#define UNITY_INCLUDE_PRINT_FORMATTED

#include "unity/unity.h"
#include "../src/aprs.h"

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

    UNITY_SET_DETAILS("aprs", "ll");
    TEST_ASSERT_EQUAL_INT(8, aprs_lat(a, sizeof(a), "0.02"));
    TEST_ASSERT_EQUAL_STRING("0001.20N", a);
    TEST_ASSERT_EQUAL_INT(9, aprs_lon(b, sizeof(b), "-0.04"));
    TEST_ASSERT_EQUAL_STRING("00002.40W", b);

    TEST_ASSERT_EQUAL_INT(8, aprs_lat(a, sizeof(a), "44.437461"));
    TEST_ASSERT_EQUAL_STRING("4426.25N", a);
    TEST_ASSERT_EQUAL_INT(9, aprs_lon(b, sizeof(b), "26.090215"));
    TEST_ASSERT_EQUAL_STRING("02605.41E", b);


    TEST_ASSERT_EQUAL_INT(33, aprs_pos(c, sizeof(c), "Cismigiu Park", "44.437461", "26.090215"));
    TEST_ASSERT_EQUAL_STRING("!4426.25N/02605.41E-Cismigiu Park", c);
    TEST_ASSERT_EQUAL_INT(31, aprs_pos(c, sizeof(c), "Null Island", "0.02", "-0.04"));
    TEST_ASSERT_EQUAL_STRING("!0001.20N/00002.40W-Null Island", c);
}

static void test_clamps_and_invalid(void)
{
    char a[64];
    char b[64];

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

    UNITY_SET_DETAILS("aprs", "payloads");
    TEST_ASSERT_EQUAL_INT((int)strlen(":BLN3     :pj6y bln"), aprs_bulletin(c, sizeof(c), 3, "pj6y bln"));
    TEST_ASSERT_EQUAL_STRING(":BLN3     :pj6y bln", c);

    TEST_ASSERT_EQUAL_INT((int)strlen(">yp0ta up"), aprs_status(c, sizeof(c), "yp0ta up"));
    TEST_ASSERT_EQUAL_STRING(">yp0ta up", c);
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

    UNITY_SET_DETAILS("aprs", "yo3gnd");
    TEST_ASSERT_EQUAL_INT((int)strlen(":YO8YL-5  :73"), aprs_message(msg, sizeof(msg), "YO8YL", 5, "73"));
    TEST_ASSERT_EQUAL_STRING(":YO8YL-5  :73", msg);
    TEST_ASSERT_TRUE(aprs_packet(&p, "YO3GND", 1, "APZFLP", 0, msg, NULL));
    TEST_ASSERT_EQUAL_UINT16(sizeof(ax_yo3gnd), p.ax25_len);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(ax_yo3gnd, p.ax25, sizeof(ax_yo3gnd));
    TEST_ASSERT_EQUAL_UINT16((uint16_t)strlen(":YO8YL-5  :73"), p.payload_len);
}

static void test_loopish_calls(void)
{
    const char *calls[6] = {"YO3GND", "YO8YL", "PJ6Y", "YP0TA", "JY1", "YO9LIG"};
    Packet p;
    char msg[160];
    int i;

    UNITY_SET_DETAILS("aprs", "loop");

    for (i = 0; i < 6; i++)
    {
        uint8_t sa = i & 15;
        const char *src = calls[i];
        const char *dst = calls[(i + 2) % 6];
        const char *txt = calls[(i + 3) % 6];
        int n;

        n = aprs_message(msg, sizeof(msg), dst, (i + 3) & 15, txt);
        TEST_ASSERT_TRUE(n > 0);
        TEST_ASSERT_TRUE(aprs_packet(&p, src, sa, "APZFLP", 0, msg, NULL));
        TEST_ASSERT_EQUAL_UINT16((uint16_t)n, p.payload_len);
        TEST_ASSERT_EQUAL_UINT8_ARRAY((uint8_t*)msg, p.payload, p.payload_len);
        TEST_ASSERT_EQUAL_HEX8(0x03, p.ax25[14]);
        TEST_ASSERT_EQUAL_HEX8(0xF0, p.ax25[15]);
        TEST_ASSERT_TRUE(p.fcs_len == p.ax25_len + 2);
        TEST_ASSERT_TRUE(p.stuffed_len > p.fcs_len);
        TEST_ASSERT_TRUE(p.nrzi_len == p.stuffed_len);
    }
}

int main(void)
{
    suiteSetUp();
    UnityBegin(__FILE__);
    RUN_TEST(test_lat_lon_and_pos);
    RUN_TEST(test_clamps_and_invalid);
    RUN_TEST(test_bulletin_and_status_strings);
    RUN_TEST(test_one_message_packet);
    RUN_TEST(test_loopish_calls);
    return suiteTearDown(UnityEnd());
}
