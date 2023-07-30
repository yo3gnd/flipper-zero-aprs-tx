#pragma once

#include <furi.h>
#include <stdint.h>


#define TXT_LEN       68
#define CFG_FILE      "/ext/apps_data/aprstx/cfg.bin"
#define CALL_N        32
#define MY_CALL       "FL1PER"
#define CARRIER_HZ    433250000UL
#define FREQ_N        4
#define POS_LEN       16

#define CFG_DIR       "/ext/apps_data/aprstx"
#define CALLBOOK_FILE "/ham/callbook.txt"
#define TXT_N         16
#define MY_TOCALL     "APZFLP"
#define CALLBOOK_DIR  "/ham"
#define CALL_LEN      10

typedef struct
{
    uint8_t encoding_index;
    uint8_t rf_m;
    uint8_t rf_d;
    uint8_t tx_freq_index;
    uint8_t d_s;
    uint8_t repeat_n;

    char bulletin[TXT_N][TXT_LEN];
    char status[TXT_N][TXT_LEN];
    char message[TXT_N][TXT_LEN];
    char calls[CALL_N][CALL_LEN];
    char pos_name[TXT_N][TXT_LEN];
    char pos_lat[TXT_N][POS_LEN];
    char pos_lon[TXT_N][POS_LEN];
    uint32_t freq[FREQ_N];

    uint8_t bulletin_used[TXT_N];
    uint8_t status_used[TXT_N];
    uint8_t message_used[TXT_N];
    uint8_t calls_used[CALL_N];
    uint8_t pos_used[TXT_N];
    uint8_t freq_used[FREQ_N];

    uint8_t bulletin_n;
    uint8_t status_n;
    uint8_t message_n;
    uint8_t calls_n;
    uint8_t pos_n;
    uint8_t freq_n;
} FlipperHamCfg;


enum
{
    FlipperHamViewMenu = 0,
    FlipperHamViewSend,
    FlipperHamViewSettings,
    FlipperHamViewBulletin,
    FlipperHamViewStatus,
    FlipperHamViewMessage,
    FlipperHamViewPosition,
    FlipperHamViewSsid,
    FlipperHamViewCall,
    FlipperHamViewBook,
    FlipperHamViewC2,
    FlipperHamViewTextInput,
    FlipperHamViewFreq,
    FlipperHamViewFreqEdit,
    FlipperHamViewPosEdit,
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
    FlipperHamSendIndexPosition,
    FlipperHamSendIndexStatus,
    FlipperHamSendIndexBulletin,
};


enum
{
    FlipperHamSettingsIndexFreq = 0,
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
    FlipperHamPositionIndexAdd = 0,
    FlipperHamPositionIndexBase = 450,
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


enum
{
    FlipperHamFreqIndexAdd = 0,
    FlipperHamFreqIndexBase = 600,
};

int32_t flipperham_app(void* p);
