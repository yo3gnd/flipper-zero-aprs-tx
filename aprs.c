#include "aprs.h"

#include <stdio.h>
#include <stdlib.h>


static int ll(char* out, uint16_t n, const char* s, uint8_t lon)
{
    float a;
    float b;
    float c;
    char* e;
    int d;
    int m;
    int h;
    char hemi;

    if(!out) return 0;
    if(!n) return 0;
    if(!s) return 0;

    a = strtof(s, &e);
    if(e == s) return 0;
    if(*e) return 0;

    hemi = lon ? 'E' : 'N';
    if(a < 0) {
        a = -a;
        hemi = lon ? 'W' : 'S';
    }

    if(lon) {
        if(a > 180.0f) return 0;
    } else {
        if(a > 90.0f) return 0;
    }

    d = (int)a;
    b = (a - d) * 60.0f;
    c = b * 100.0f + 0.5f;
    h = (int)c;
    m = h / 100;
    h = h % 100;

    if(m >= 60)
    {
        m = 0;
        h = 0;
        d++;
    }

    if(lon)
    {
        if(d > 180) return 0;
        return snprintf(out, n, "%03d%02d.%02d%c", d, m, h, hemi);
    }

    if(d > 90) return 0;
    return snprintf(out, n, "%02d%02d.%02d%c", d, m, h, hemi);
}

int aprs_ll_clamp(char* out, uint16_t n, const char* s, uint8_t lon)
{
    float a;
    char* e;

    if(!out) return 0;
    if(!n) return 0;
    if(!s) return 0;

    a = strtof(s, &e);
    if(e == s) return 0;
    if(*e) return 0;

    if(lon)
    {
        if(a < -180.0f) a = -180.0f;
        if(a > 180.0f) a = 180.0f;
    }
    else
    {
        if(a < -90.0f) a = -90.0f;
        if(a > 90.0f) a = 90.0f;
    }

    return snprintf(out, n, "%.5f", (double)a);
}

int aprs_lat(char* out, uint16_t n, const char* s)
{
    return ll(out, n, s, 0);
}

int aprs_lon(char* out, uint16_t n, const char* s)
{
    return ll(out, n, s, 1);
}

int aprs_pos(char* out, uint16_t n, const char* name, const char* lat, const char* lon)
{
    char a[9];
    char b[10];

    if(aprs_lat(a, sizeof(a), lat) <= 0) return 0;
    if(aprs_lon(b, sizeof(b), lon) <= 0) return 0;
    return snprintf(out, n, "!%s/%s-%s", a, b, name ? name : "");
}

uint16_t aprs_passcode(const char* s)
{
    char a[7];
    uint16_t h;
    uint8_t i;
    char c;

    if(!s) return 0;

    i = 0;
    while(*s)
    {
        if(*s == '-') break;
        if(i >= sizeof(a) - 1) break;
        c = *s++;
        if(c >= 'a' && c <= 'z') c -= 32;
        a[i++] = c;
    }
    a[i] = 0;
    if(!a[0]) return 0;

    h = 0x73e2;
    i = 0;
    while(a[i])
    {
        h ^= ((uint16_t)a[i]) << 8;
        if(a[i + 1]) h ^= (uint8_t)a[i + 1];
        i += 2;
    }

    return h & 0x7fff;
}

int aprs_pass_validate(const char* s, uint16_t p)
{
    uint16_t a;

    a = aprs_passcode(s);
    if(!a) return 0;
    if(a != p) return 0;
    return 1;
}
