#include <stdio.h>
#include <string.h>

#include "../aprs.h"

int main(void)
{
    int ok = 0;
    int bad = 0;
    char a[16];
    char b[16];
    char c[64];

    printf("== pos lat 1 ==\n");
    if(aprs_lat(a, sizeof(a), "0.02") == 8 && !strcmp(a, "0001.20N")) { ok++; printf("ok lat 1\n"); }
    else { bad++; printf("bad lat 1 got=%s\n", a); }

    printf("== pos lon 1 ==\n");
    if(aprs_lon(b, sizeof(b), "-0.04") == 9 && !strcmp(b, "00002.40W")) { ok++; printf("ok lon 1\n"); }
    else { bad++; printf("bad lon 1 got=%s\n", b); }

    printf("== pos lat 2 ==\n");
    if(aprs_lat(a, sizeof(a), "44.43778") == 8 && !strcmp(a, "4426.27N")) { ok++; printf("ok lat 2\n"); }
    else { bad++; printf("bad lat 2 got=%s\n", a); }

    printf("== pos lon 2 ==\n");
    if(aprs_lon(b, sizeof(b), "26.09778") == 9 && !strcmp(b, "02605.87E")) { ok++; printf("ok lon 2\n"); }
    else { bad++; printf("bad lon 2 got=%s\n", b); }

    printf("== pos full ==\n");
    if(aprs_pos(c, sizeof(c), "Null Island", "0.02", "-0.04") == 31 && !strcmp(c, "!0001.20N/00002.40W-Null Island")) { ok++; printf("ok pos full\n"); }
    else { bad++; printf("bad pos full got=%s\n", c); }

    printf("== clamp lat ==\n");
    if(aprs_ll_clamp(a, sizeof(a), "123.45", 0) > 0 && !strcmp(a, "90.00000")) { ok++; printf("ok clamp lat\n"); }
    else { bad++; printf("bad clamp lat got=%s\n", a); }

    printf("== clamp lon ==\n");
    if(aprs_ll_clamp(b, sizeof(b), "-222.75", 1) > 0 && !strcmp(b, "-180.00000")) { ok++; printf("ok clamp lon\n"); }
    else { bad++; printf("bad clamp lon got=%s\n", b); }

    printf("ok=%d bad=%d\n", ok, bad);
    if(bad) return 1;

    return 0;
}
