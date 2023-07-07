#pragma once

#include <furi.h>
#include <stdint.h>


#define TXT_LEN       68
#define CFG_FILE      "/ext/apps_data/aprstx/cfg.bin"
#define CALL_N        32
#define MY_CALL       "FL1PER"
#define CARRIER_HZ    433250000UL

#define CFG_DIR       "/ext/apps_data/aprstx"
#define CALLBOOK_FILE "/ham/callbook.txt"
#define TXT_N         16
#define MY_TOCALL     "APZFLP"
#define CALLBOOK_DIR  "/ham"
#define CALL_LEN      10


typedef struct {
    const char* name;
    uint16_t baud;
    uint16_t mark_hz;
    uint16_t space_hz;
} FlipperHamModemProfile;


enum {
    FlipperHamModemProfileDefault = 1,
    FlipperHamPresetDefault = 2,
};


typedef struct {
    const char* name;
    const uint8_t* regs;
} FlipperHamPreset;

int32_t flipperham_app(void* p);
