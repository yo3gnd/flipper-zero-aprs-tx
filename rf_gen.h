#ifndef GUARD_QWAASASESWQ3321
#define GUARD_QWAASASESWQ3321

#include "flipperham.h"
#include "app_state.h"


typedef struct {
    const char* name;
    const uint8_t* regs;
} FlipperHamPreset;


enum {
    FlipperHamModemProfileDefault = 1,
    FlipperHamPresetDefault = 2,
};


typedef struct {
    const char* name;
    uint16_t baud;
    uint16_t mark_hz;
    uint16_t space_hz;
} FlipperHamModemProfile;


extern const FlipperHamPreset flipperham_presets[4];
extern const FlipperHamModemProfile flipperham_modem_profiles[2];


void flipperham_radio_start(FlipperHamApp* app);
void pf(FlipperHamApp* app);
uint32_t txf(FlipperHamApp* app);


void txstart(FlipperHamApp* app);
void flipperham_radio_stop(FlipperHamApp* app);

#endif
