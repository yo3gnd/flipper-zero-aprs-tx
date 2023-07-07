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


typedef struct
{
    uint8_t encoding_index;
    uint8_t rf_m;
    uint8_t rf_d;

    char bulletin[TXT_N][TXT_LEN];
    char status[TXT_N][TXT_LEN];
    char message[TXT_N][TXT_LEN];
    char calls[CALL_N][CALL_LEN];

    uint8_t bulletin_used[TXT_N];
    uint8_t status_used[TXT_N];
    uint8_t message_used[TXT_N];
    uint8_t calls_used[CALL_N];

    uint8_t bulletin_n;
    uint8_t status_n;
    uint8_t message_n;
    uint8_t calls_n;
} FlipperHamCfg;


enum
{
    FlipperHamViewMenu = 0,
    FlipperHamViewSend,
    FlipperHamViewSettings,
    FlipperHamViewBulletin,
    FlipperHamViewStatus,
    FlipperHamViewMessage,
    FlipperHamViewSsid,
    FlipperHamViewCall,
    FlipperHamViewBook,
    FlipperHamViewC2,
    FlipperHamViewTextInput,
};


enum
{
    FlipperHamMenuIndexSend = 0,
    FlipperHamMenuIndexSettings,
    FlipperHamMenuIndexCallbook,
};


enum
{
    FlipperHamSendIndexMessage = 0,
    FlipperHamSendIndexStatus,
    FlipperHamSendIndexBulletin,
};


enum
{
    FlipperHamBulletinIndexAdd = 0,
    FlipperHamBulletinIndexBase = 100,
};


enum
{
    FlipperHamStatusIndexAdd = 0,
    FlipperHamStatusIndexBase = 200,
};


enum
{
    FlipperHamCallIndexAdd = 0,
    FlipperHamCallIndexBase = 300,
};


enum
{
    FlipperHamMessageIndexAdd = 0,
    FlipperHamMessageIndexBase = 400,
};


enum
{
    FlipperHamBookIndexAdd = 0,
    FlipperHamBookIndexBase = 500,
};


enum
{
    FlipperHamC2IndexEdit = 0,
    FlipperHamC2IndexDelete,
    FlipperHamC2IndexCopy,
};


extern const FlipperHamPreset flipperham_presets[];
extern const FlipperHamModemProfile flipperham_modem_profiles[];

int32_t flipperham_app(void* p);
