#ifndef GUARD_QWAASASESWQ3321
#define GUARD_QWAASASESWQ3321

#include "flipperham.h"
#include "app_state.h"


extern const FlipperHamPreset flipperham_presets[4];
extern const FlipperHamModemProfile flipperham_modem_profiles[2];


void flipperham_radio_start(FlipperHamApp* app);
void pf(FlipperHamApp* app);


void txstart(FlipperHamApp* app);
void flipperham_radio_stop(FlipperHamApp* app);

#endif
