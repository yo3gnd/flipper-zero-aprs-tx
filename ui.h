#ifndef GUARD_UI_H_22881
#define GUARD_UI_H_22881

#include "app_state.h"


void cfgsave(FlipperHamApp* app);
void cfgload(FlipperHamApp* app);
void csavetxt(FlipperHamApp* app);


bool csplit(const char* s, char* out, uint8_t* ssid, bool* has_ssid);
bool cval(char* s);


void bfix(FlipperHamApp* app);
void stfix(FlipperHamApp* app);
void mfix(FlipperHamApp* app);
void cfix(FlipperHamApp* app);
void ffix(FlipperHamApp* app);

#endif
