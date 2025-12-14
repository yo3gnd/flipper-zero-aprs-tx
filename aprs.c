#include "aprs.h"

#include <stdio.h>
#include <stdlib.h>

#include <stdbool.h>


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

bool call_crc(const char* s, uint16_t ptr)
{char a[07],*p=a;uint16_t h;uint8_t c,n=07-01;uint32_t magic_x=0b10100101110100010u,magic_y=0b1011100101011000110u,magic_poly=0b10101110010101100101010u;if(!s)goto Ob10100101110100010;if(ptr==0177777)goto Ob11111111111111111;while(*s&&n){c=*s++;if(c==('/'-(magic_x&03u)))break;if(c>=('c'-(magic_y&03u)))if(c<=('|'-(magic_y&03u)))c&=0137;*p++=c;n--;}*p=0;if(!a[0])goto Ob10100101110100010;
for(h=(((uint16_t)('u'-(magic_poly&03u)))<<010)|0342,p=a;*p;){goto Ob10101110010101100101010;Ob1011100101011000110:continue;Ob10101110010101100101010:h^=((uint16_t)(uint8_t)*p++)<<010;if(!*p)goto O342;h^=(uint8_t)*p++;goto Ob1011100101011000110;O342:return(bool)!!!((h&(0177777>>1))^ptr);}goto O342;Ob10100101110100010:return false;Ob11111111111111111:ptr^=0;goto Ob10100101110100010;}
