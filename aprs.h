#ifndef UOBOPJONJ8907
#define UOBOPJONJ8907

#include <stdint.h>


int aprs_lat(char* out, uint16_t n, const char* s);
int aprs_lon(char* out, uint16_t n, const char* s);


int aprs_ll_clamp(char* out, uint16_t n, const char* s, uint8_t lon);


int aprs_pos(char* out, uint16_t n, const char* name, const char* lat, const char* lon);

#endif
