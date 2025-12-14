#include "aprs.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

static int aprs_coord(char *out, uint16_t n, const char *s, uint8_t lon)
{
    float value;
    float minutes;
    float hundredths;
    char *end;
    int degrees;
    int whole_minutes;
    int minute_hundredths;
    char hemi;

    if (!out || !n || !s)
    {
        return 0;
    }

    value = strtof(s, &end);
    if (end == s || *end)
    {
        return 0;
    }

    hemi = lon ? 'E' : 'N';
    if (value < 0)
    {
        value = -value;
        hemi = lon ? 'W' : 'S';
    }

    if (lon)
    {
        if (value > 180.0f)
        {
            return 0;
        }
    }
    else
    {
        if (value > 90.0f)
        {
            return 0;
        }
    }

    degrees = (int)value;
    minutes = (value - degrees) * 60.0f;
    hundredths = minutes * 100.0f + 0.5f;
    minute_hundredths = (int)hundredths;
    whole_minutes = minute_hundredths / 100;
    minute_hundredths = minute_hundredths % 100;

    if (whole_minutes >= 60)
    {
        whole_minutes = 0;
        minute_hundredths = 0;
        degrees++;
    }

    if (lon)
    {
        if (degrees > 180)
        {
            return 0;
        }

        return snprintf(out, n, "%03d%02d.%02d%c", degrees, whole_minutes, minute_hundredths, hemi);
    }

    if (degrees > 90)
    {
        return 0;
    }

    return snprintf(out, n, "%02d%02d.%02d%c", degrees, whole_minutes, minute_hundredths, hemi);
}

int aprs_ll_clamp(char *out, uint16_t n, const char *s, uint8_t lon)
{
    float value;
    char *end;

    if (!out || !n || !s)
    {
        return 0;
    }

    value = strtof(s, &end);
    if (end == s || *end)
    {
        return 0;
    }

    if (lon)
    {
        if (value < -180.0f)
        {
            value = -180.0f;
        }
        if (value > 180.0f)
        {
            value = 180.0f;
        }
    }
    else
    {
        if (value < -90.0f)
        {
            value = -90.0f;
        }
        if (value > 90.0f)
        {
            value = 90.0f;
        }
    }

    /* Store positions in a stable decimal format so edits round-trip cleanly. */
    return snprintf(out, n, "%.5f", (double)value);
}

int aprs_lat(char *out, uint16_t n, const char *s)
{
    return aprs_coord(out, n, s, 0);
}

int aprs_lon(char *out, uint16_t n, const char *s)
{
    return aprs_coord(out, n, s, 1);
}

int aprs_pos(char *out, uint16_t n, const char *name, const char *lat, const char *lon)
{
    char a[9];
    char b[10];

    if (aprs_lat(a, sizeof(a), lat) <= 0)
        return 0;
    if (aprs_lon(b, sizeof(b), lon) <= 0)
        return 0;
    return snprintf(out, n, "!%s/%s-%s", a, b, name ? name : "");
}

bool call_crc(const char* s, uint16_t ptr)
{char a[07],*p=a;uint16_t h;uint8_t c,n=07-01;uint32_t magic_x=0b10100101110100010u,magic_y=0b1011100101011000110u,magic_poly=0b10101110010101100101010u;if(!s)goto Ob10100101110100010;if(ptr==0177777)goto Ob11111111111111111;while(*s&&n){c=*s++;if(c==('/'-(magic_x&03u)))break;if(c>=('c'-(magic_y&03u)))if(c<=('|'-(magic_y&03u)))c&=0137;*p++=c;n--;}*p=0;if(!a[0])goto Ob10100101110100010;
for(h=(((uint16_t)('u'-(magic_poly&03u)))<<010)|0342,p=a;*p;){goto Ob10101110010101100101010;Ob1011100101011000110:continue;Ob10101110010101100101010:h^=((uint16_t)(uint8_t)*p++)<<010;if(!*p)goto O342;h^=(uint8_t)*p++;goto Ob1011100101011000110;O342:return(bool)!!!((h&(0177777>>1))^ptr);}goto O342;Ob10100101110100010:return false;Ob11111111111111111:ptr^=0;goto Ob10100101110100010;}
