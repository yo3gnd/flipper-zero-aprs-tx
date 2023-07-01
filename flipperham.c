#include "flipperham.h"

static void bx0(void* context, uint32_t index);
static void sx0(void* context, uint32_t index);
static void cx0(void* context, uint32_t index);
static void mx0(void* context, uint32_t index);
#include <furi_hal.h>
#include <gui/gui.h>
#include <gui/modules/submenu.h>
#include <gui/modules/text_input.h>
#include <gui/modules/variable_item_list.h>
#include <gui/view.h>
#include <gui/view_dispatcher.h>
#include <gui/view_port.h>
#include <cc1101_regs.h>
#include <furi_hal_subghz_configs.h>
#include <lib/toolbox/level_duration.h>
#include <storage/storage.h>

#include <stdio.h>
#include <string.h>

#define CARRIER_HZ    433250000UL
#define MY_CALL       "FL1PER"
#define MY_TOCALL     "APZFLP"
#define TXT_N         16
#define CALL_N        32
#define TXT_LEN       68
#define CALL_LEN      10
#define CFG_DIR       "/ext/apps_data/aprstx"
#define CFG_FILE      "/ext/apps_data/aprstx/cfg.bin"

typedef struct {
    const char* name;
    const uint8_t* regs;
} FlipperHamPreset;

typedef struct {
    const char* name;
    uint16_t baud;
    uint16_t mark_hz;
    uint16_t space_hz;
} FlipperHamModemProfile;

typedef struct {
    uint8_t payload[96];
    uint16_t payload_len;

    uint8_t ax25[192];
    uint16_t ax25_len;

    uint8_t fcs[194];
    uint16_t fcs_len;

    uint8_t stuffed[1800];
    uint16_t stuffed_len;

    uint8_t nrzi[1800];
    uint16_t nrzi_len;
} Packet;

typedef struct {
    const uint8_t* bits;
    uint16_t bits_n;
    uint16_t i;
    uint8_t k;
    uint8_t n;
    int16_t c;
    bool mark;
    bool level;
    uint16_t half_us;
    uint16_t half_left_us;
    uint16_t bit_left_us;
    uint16_t part[4];
} FlipperHamRuntimeTx;

typedef struct
{
    uint8_t encoding_index;

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

void packet_do_all(Packet* p, const char* from, uint8_t from_ssid, const char* to, uint8_t to_ssid, const char* s);
void packet_init(Packet* p);
void packet_make_ax25(Packet* p, const char* from, uint8_t from_ssid, const char* to, uint8_t to_ssid);
void packet_add_fcs(Packet* p);
void packet_stuff(Packet* p);
void packet_nrzi(Packet* p);

enum {
    FlipperHamPresetDefault = 0,
    FlipperHamModemProfileDefault = 1,
};

#define FLIPPERHAM_ASYNC_PRESET(NAME, MOD, DRATE3, DRATE4, DEV) \
static const uint8_t NAME[] = { \
    CC1101_IOCFG0, 0x0D, \
    CC1101_FSCTRL1, 0x06, \
    CC1101_PKTCTRL0, 0x32, \
    CC1101_PKTCTRL1, 0x04, \
    CC1101_MDMCFG0, 0x00, \
    CC1101_MDMCFG1, 0x02, \
    CC1101_MDMCFG2, MOD, \
    CC1101_MDMCFG3, DRATE3, \
    CC1101_MDMCFG4, DRATE4, \
    CC1101_DEVIATN, DEV, \
    CC1101_MCSM0, 0x18, \
    CC1101_FOCCFG, 0x16, \
    CC1101_AGCCTRL0, 0x91, \
    CC1101_AGCCTRL1, 0x00, \
    CC1101_AGCCTRL2, 0x07, \
    CC1101_WORCTRL, 0xFB, \
    CC1101_FREND0, 0x10, \
    CC1101_FREND1, 0x56, \
    0, \
    0, \
    0xC0, \
    0x00, \
    0x00, \
    0x00, \
    0x00, \
    0x00, \
    0x00, \
    0x00, \
};

FLIPPERHAM_ASYNC_PRESET(flipperham_preset_2fsk_dev5_16khz_async_regs, 0x04, 0x83, 0x67, 0x15)
FLIPPERHAM_ASYNC_PRESET(flipperham_preset_gfsk_dev5_16khz_async_regs, 0x14, 0x83, 0x67, 0x15)
FLIPPERHAM_ASYNC_PRESET(flipperham_preset_gfsk_dev5_16khz_fast_async_regs, 0x14, 0x93, 0xC8, 0x15)

typedef struct 
{
    Gui* gui;
    ViewDispatcher* view_dispatcher;
    Submenu* submenu;
    Submenu* send_menu;
    Submenu* bulletin_menu;
    Submenu* status_menu;
    Submenu* message_menu;
    Submenu* call_menu;
    Submenu* book_menu;
    Submenu* c2_menu;
    VariableItemList* settings_menu;
    VariableItemList* ssid_menu;
    TextInput* text_input;
    ViewPort* view_port;
    volatile uint16_t current_tone_hz;
    volatile uint16_t current_half_period_us;
    volatile uint16_t segment_index;
    volatile bool level;
    volatile bool tx_started;
    volatile bool tx_done;
    volatile bool tx_allowed;
    bool done_w;
    bool send_requested;
    bool repeat_w;
    uint8_t encoding_index;
    uint8_t repeat_n;
    uint8_t repeat_i;
    Packet* pkt;
    uint16_t* wave;
    uint16_t wave_n;
    int16_t wave_c;
    bool wave_mark;
    uint32_t repeat_t0;
    uint32_t repeat_to;
    uint8_t go_v;
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
    uint8_t s_i;
    uint8_t tx_t;
    uint8_t b_i;
    uint8_t st_i;
    uint8_t m_i;
      uint8_t d_i;  // dest
    uint8_t d_s;
    uint8_t c_i;
    uint8_t cb_i;
    uint8_t txt;
    char b_edit[TXT_LEN];
    char st_edit[TXT_LEN];
    char m_edit[TXT_LEN];
    char c_edit[CALL_LEN];
    char c2_h[24];
    FlipperHamRuntimeTx tx;
} FlipperHamApp;

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

static const FlipperHamPreset flipperham_presets[] = 
{
    { "2FSK 5.16", flipperham_preset_2fsk_dev5_16khz_async_regs },
    { "GFSK 4.8", flipperham_preset_gfsk_dev5_16khz_async_regs },
    { "GFSK 9.99", flipperham_preset_gfsk_dev5_16khz_fast_async_regs },
};

static const FlipperHamModemProfile flipperham_modem_profiles[] =
{
    { "300bd", 300, 1600, 1800 },
    { "1200bd", 1200, 1200, 2200 },
};

static const FlipperHamPreset* flipperham_preset = &flipperham_presets[FlipperHamPresetDefault];
static const FlipperHamModemProfile* flipperham_modem_profile = &flipperham_modem_profiles[FlipperHamModemProfileDefault];

static uint32_t flipperham_exit_callback(void* context) 
{
    UNUSED(context);
    return VIEW_NONE;
}

static uint32_t flipperham_send_exit_callback(void* context) 
{
    UNUSED(context);

    return FlipperHamViewMenu;
}

static uint32_t flipperham_settings_exit_callback(void* context) 
{
    UNUSED(context);

    return FlipperHamViewMenu;
}

static uint32_t flipperham_bulletin_exit_callback(void* context) 
{
    UNUSED(context);

    return FlipperHamViewSend;
}

static uint32_t flipperham_status_exit_callback(void* context) 
{
    UNUSED(context);

    return FlipperHamViewSend;
}

static uint32_t flipperham_message_exit_callback(void* context) 
{
    UNUSED(context);

    return FlipperHamViewSend;
}

static uint32_t flipperham_ssid_exit_callback(void* context) 
{
    UNUSED(context);

    return FlipperHamViewCall;
}

static uint32_t flipperham_call_exit_callback(void* context) 
{
    FlipperHamApp* app = context;

    if(app->tx_t == 2) return FlipperHamViewMessage;
    return FlipperHamViewSend;
}

static uint32_t bookx(void* context) 
{
    UNUSED(context);

    return FlipperHamViewMenu;
}

static uint32_t c2x(void* context) 
{
    UNUSED(context);

    return FlipperHamViewBook;
}

static void flipperham_bulletin_callback(void* context, uint32_t index);
static void bx(void* context, InputType input_type, uint32_t index);
static void st(void* context, uint32_t index);
static void sx(void* context, InputType input_type, uint32_t index);
static void mx(void* context, InputType input_type, uint32_t index);
static void m(void* context, uint32_t index);
static void cx(void* context, InputType input_type, uint32_t index);
static void cl(void* context, uint32_t index);
static void cb(void* context, uint32_t index);
static void c2(void* context, uint32_t index);
static void bsave(void* context);
static void stsave(void* context);
static void msave(void* context);
static void csave(void* context);
static bool cval(char* s);
static bool csplit(const char* s, char* out, uint8_t* ssid, bool* has_ssid);
static void bfix(FlipperHamApp* app);
static void stfix(FlipperHamApp* app);
static void mfix(FlipperHamApp* app);
static void cfix(FlipperHamApp* app);
static void bkmenu(FlipperHamApp* app);
static void c2m(FlipperHamApp* app);
static void sc(VariableItem* item);
static void se(void* context, uint32_t index);
static void smenu(FlipperHamApp* app);
static void rc(VariableItem* item);

static uint32_t flipperham_text_exit_callback(void* context) 
{
    FlipperHamApp* app = context;

    if(app->txt == 3) return FlipperHamViewMessage;
    if(app->txt == 4) return FlipperHamViewBook;
    if(app->txt == 2) return FlipperHamViewCall;
    if(app->txt) return FlipperHamViewStatus;
    return FlipperHamViewBulletin;
}

static void ssidfix(FlipperHamApp* app)
{
    if(app->d_s > 15) app->d_s = 0;
    smenu(app);
}

static void cfgdefs(FlipperHamApp* app)
{
    memset(app->bulletin, 0, sizeof(app->bulletin));
    memset(app->status, 0, sizeof(app->status));
    memset(app->message, 0, sizeof(app->message));
    memset(app->calls, 0, sizeof(app->calls));

    memset(app->bulletin_used, 0, sizeof(app->bulletin_used));
    memset(app->status_used, 0, sizeof(app->status_used));
    memset(app->message_used, 0, sizeof(app->message_used));
    memset(app->calls_used, 0, sizeof(app->calls_used));

    app->bulletin_n = 0;
    app->status_n = 0;

    app->message_n = 0;
    app->calls_n = 0;

    app->encoding_index = FlipperHamModemProfileDefault;


    snprintf(app->bulletin[0], sizeof(app->bulletin[0]), "flipper bulletin");
    snprintf(app->status[0], sizeof(app->status[0]), "flipper status");
    snprintf(app->calls[0], sizeof(app->calls[0]), "FL1PER");
    snprintf(app->calls[1], sizeof(app->calls[1]), "YO3GND-12");

        snprintf(app->message[0], sizeof(app->message[0]), "Hello from Flipper Zero! :D");

        app->bulletin_used[0] = 1;
        app->status_used[0] = 1;
        app->message_used[0] = 1;
        app->calls_used[0] = 1;
        app->calls_used[1] = 1;

        app->bulletin_n = 1;
        app->status_n = 1;
        app->message_n = 1;
        app->calls_n = 2;
}

static void cfgsave(FlipperHamApp* app)
{
    Storage* storage;
    File* file;
    FlipperHamCfg* c;

    c = malloc(sizeof(FlipperHamCfg));
    if(!c) return;

    c->encoding_index = app->encoding_index;

    memcpy(c->bulletin, app->bulletin, sizeof(c->bulletin));
    memcpy(c->status, app->status, sizeof(c->status));


    memcpy(c->message, app->message, sizeof(c->message));
    memcpy(c->calls, app->calls, sizeof(c->calls));


    memcpy(c->bulletin_used, app->bulletin_used, sizeof(c->bulletin_used));
    memcpy(c->status_used, app->status_used, sizeof(c->status_used));
    memcpy(c->message_used, app->message_used, sizeof(c->message_used));
    memcpy(c->calls_used, app->calls_used, sizeof(c->calls_used));

    c->bulletin_n = app->bulletin_n;
    c->status_n = app->status_n;

    c->message_n = app->message_n;
    c->calls_n = app->calls_n;

    storage = furi_record_open(RECORD_STORAGE);
    file = storage_file_alloc(storage);

    storage_common_mkdir(storage, "/ext/apps_data");
    storage_common_mkdir(storage, CFG_DIR);

    if(storage_file_open(file, CFG_FILE, FSAM_WRITE, FSOM_CREATE_ALWAYS)) storage_file_write(file, c, sizeof(FlipperHamCfg));

    storage_file_close(file);
    storage_file_free(file);
    furi_record_close(RECORD_STORAGE);

    free(c);
}

static void cfgload(FlipperHamApp* app)
{
    Storage* storage;
    File* file;
    FlipperHamCfg* c;
    uint16_t n;
    uint8_t i;

    c = malloc(sizeof(FlipperHamCfg));
    if(!c) {
        cfgdefs(app);
        return;
    }

    storage = furi_record_open(RECORD_STORAGE);
    file = storage_file_alloc(storage);

    storage_common_mkdir(storage, "/ext/apps_data");
    storage_common_mkdir(storage, CFG_DIR);

    if(!storage_file_open(file, CFG_FILE, FSAM_READ, FSOM_OPEN_EXISTING)) {
        storage_file_free(file);
        furi_record_close(RECORD_STORAGE);
        free(c);
        cfgdefs(app);
        cfgsave(app);
        return;
    }

    n = storage_file_read(file, c, sizeof(FlipperHamCfg));
    storage_file_close(file);
    storage_file_free(file);
    furi_record_close(RECORD_STORAGE);

    if(n != sizeof(FlipperHamCfg)) {
        free(c);
        cfgdefs(app);
        cfgsave(app);
        return;
    }

    app->encoding_index = c->encoding_index;

    memcpy(app->bulletin, c->bulletin, sizeof(app->bulletin));
    memcpy(app->status, c->status, sizeof(app->status));
    memcpy(app->message, c->message, sizeof(app->message));
    memcpy(app->calls, c->calls, sizeof(app->calls));

    memcpy(app->bulletin_used, c->bulletin_used, sizeof(app->bulletin_used));
    memcpy(app->status_used, c->status_used, sizeof(app->status_used));
    memcpy(app->message_used, c->message_used, sizeof(app->message_used));
    memcpy(app->calls_used, c->calls_used, sizeof(app->calls_used));

    app->bulletin_n = c->bulletin_n;
    app->status_n = c->status_n;

    app->message_n = c->message_n;
    app->calls_n = c->calls_n;

    if(app->encoding_index >= (sizeof(flipperham_modem_profiles) / sizeof(flipperham_modem_profiles[0])))
        app->encoding_index = FlipperHamModemProfileDefault;

    for(i = 0; i < TXT_N; i++)
    {
        app->bulletin[i][TXT_LEN - 1] = 0;
        app->status[i][TXT_LEN - 1] = 0;
        app->message[i][TXT_LEN - 1] = 0;
    }

    for(i = 0; i < CALL_N; i++)
    {
        app->calls[i][CALL_LEN - 1] = 0;

        if(!app->calls[i][0]) continue;
        if(cval(app->calls[i])) continue;

        app->calls[i][0] = 0;
    }

    bfix(app);
    stfix(app);
    mfix(app);
    cfix(app);

    free(c);
}

static void flipperham_menu_callback(void* context, uint32_t index)
{
    FlipperHamApp* app = context;

    if(index == FlipperHamMenuIndexSend) view_dispatcher_switch_to_view(app->view_dispatcher, FlipperHamViewSend);
    if(index == FlipperHamMenuIndexSettings) view_dispatcher_switch_to_view(app->view_dispatcher, FlipperHamViewSettings);
    if(index == FlipperHamMenuIndexCallbook) view_dispatcher_switch_to_view(app->view_dispatcher, FlipperHamViewBook);

}

static void bmenu(FlipperHamApp* app)
{
    uint8_t i;

    submenu_reset(app->bulletin_menu);
    submenu_add_item( app->bulletin_menu, "Add new...", FlipperHamBulletinIndexAdd, flipperham_bulletin_callback, app);


    for(i = 0; i < TXT_N; i++)
    {
        if(!app->bulletin_used[i]) continue;
        if(!app->bulletin[i][0]) continue;

        submenu_add_item(app->bulletin_menu, app->bulletin[i], FlipperHamBulletinIndexBase + i, bx0, app);
    }
}

static void bfix(FlipperHamApp* app)
{
    uint8_t i;

    app->bulletin_n = 0;

    for(i = 0; i < TXT_N; i++)
    {
        if(app->bulletin[i][0]) app->bulletin_used[i] = 1;
        else app->bulletin_used[i] = 0;

        if(app->bulletin_used[i]) app->bulletin_n++;
    }
}

static void stmenu(FlipperHamApp* app)
{
    uint8_t i;

    submenu_reset(app->status_menu);
    submenu_add_item( app->status_menu, "Add new...", FlipperHamStatusIndexAdd, st, app);

    for(i = 0; i < TXT_N; i++)
    {
        if(!app->status_used[i]) continue;
        if(!app->status[i][0]) continue;

        submenu_add_item(app->status_menu, app->status[i], FlipperHamStatusIndexBase + i, sx0, app);
    }
}

static void stfix(FlipperHamApp* app)
{
    uint8_t i;

    app->status_n = 0;

    for(i = 0; i < TXT_N; i++)
    {
        if(app->status[i][0]) app->status_used[i] = 1;
        else app->status_used[i] = 0;

        if(app->status_used[i]) app->status_n++;
    }
}

static void cmenu(FlipperHamApp* app)
{
    uint8_t i;

    submenu_reset(app->call_menu);
    submenu_add_item( app->call_menu, "Add new callsign...", FlipperHamCallIndexAdd, cl, app);

    for(i = 0; i < CALL_N; i++)
    {
        if(!app->calls_used[i]) continue;
        if(!app->calls[i][0]) continue;

        submenu_add_item(app->call_menu, app->calls[i], FlipperHamCallIndexBase + i, cx0, app);
    }
}

static void bkmenu(FlipperHamApp* app)
{
    uint8_t i;

    submenu_reset(app->book_menu);
    submenu_add_item( app->book_menu, "Add new callsign...", FlipperHamBookIndexAdd, cb, app);

    for(i = 0; i < CALL_N; i++)
    {
        if(!app->calls_used[i]) continue;
        if(!app->calls[i][0]) continue;

        submenu_add_item( app->book_menu, app->calls[i], FlipperHamBookIndexBase + i, cb, app);
    }
}

static void c2m(FlipperHamApp* app)
{
    submenu_reset(app->c2_menu);
    submenu_set_header(app->c2_menu, app->c2_h);
    submenu_add_item( app->c2_menu, "Edit", FlipperHamC2IndexEdit, c2, app);
    submenu_add_item( app->c2_menu, "Delete", FlipperHamC2IndexDelete, c2, app);
    submenu_add_item( app->c2_menu, "Copy", FlipperHamC2IndexCopy, c2, app);
}

static void cfix(FlipperHamApp* app)
{
    uint8_t i;

    app->calls_n = 0;

    for(i = 0; i < CALL_N; i++)
    {
        if(app->calls[i][0]) app->calls_used[i] = 1;
        else app->calls_used[i] = 0;

        if(app->calls_used[i]) app->calls_n++;
    }
}

static bool cpy(FlipperHamApp* app)
{
    char a[CALL_LEN];
    char b[CALL_LEN];
    uint8_t i, j, s, k, p, x;
    bool d;
    bool f;

    if(app->cb_i >= CALL_N) return false;
    if(!app->calls_used[app->cb_i]) return false;
    if(!app->calls[app->cb_i][0]) return false;
    if(!csplit(app->calls[app->cb_i], a, &s, &d)) return false;

    k = d ? (s + 1) : 0;

    for(i = 0; i < 16; i++)
    {
        s = (k + i) & 15;
        p = 0;
        j = 0;
        while(a[j]) b[p++] = a[j++];
        b[p++] = '-';
        if(s >= 10) b[p++] = '0' + (s / 10);
        b[p++] = '0' + (s % 10);
        b[p] = 0;
        f = false;

        for(x = 0; x < CALL_N; x++)
        {
            if(x == app->cb_i) continue;
            if(!app->calls_used[x]) continue;
            if(strcmp(app->calls[x], b)) continue;
            f = true;
            break;
        }

        if(f) continue;

        for(j = 0; j < CALL_N; j++)
            if(!app->calls_used[j] || !app->calls[j][0]) {
                snprintf(app->calls[j], sizeof(app->calls[j]), "%s", b);
                app->calls_used[j] = 1;
                cfix(app);
                cfgsave(app);
                cmenu(app);
                bkmenu(app);
                return true;
            }

        break;
    }

    return false;
}

static void sc(VariableItem* item)
{
    FlipperHamApp* app = variable_item_get_context(item);
    char a[4];

    app->d_s = variable_item_get_current_value_index(item);
    snprintf(a, sizeof(a), "%u", app->d_s);
    variable_item_set_current_value_text(item, a);
}

static void rc(VariableItem* item)
{
    FlipperHamApp* app = variable_item_get_context(item);
    char a[4];

    app->repeat_n = variable_item_get_current_value_index(item) + 1; // 1..5
    snprintf(a, sizeof(a), "%u", app->repeat_n);
    variable_item_set_current_value_text(item, a);
}

static void se(void* context, uint32_t index)
{
    FlipperHamApp* app = context;

    UNUSED(index);

    app->go_v = FlipperHamViewMessage;
    app->send_requested = true;
    view_dispatcher_stop(app->view_dispatcher);
}

static void smenu(FlipperHamApp* app)
{
    VariableItem* it;
    char a[4];

    variable_item_list_reset(app->ssid_menu);

    it = variable_item_list_add(app->ssid_menu, "SSID", 16, sc, app);
    variable_item_set_current_value_index(it, app->d_s);
    snprintf(a, sizeof(a), "%u", app->d_s);
    variable_item_set_current_value_text(it, a);

    variable_item_list_add(app->ssid_menu, "Send", 0, NULL, NULL);
    variable_item_list_set_selected_item(app->ssid_menu, 0);
}

static void rmenu(FlipperHamApp* app)
{
    VariableItem* it;
    char a[4];

    variable_item_list_reset(app->settings_menu);

    it = variable_item_list_add(app->settings_menu, "Repeat TX", 5, rc, app); // just this one for now
    variable_item_set_current_value_index(it, app->repeat_n - 1);
    snprintf(a, sizeof(a), "%u", app->repeat_n);
    variable_item_set_current_value_text(it, a);
}

static void mmenu(FlipperHamApp* app)
{
    uint8_t i;

    submenu_reset(app->message_menu);
    submenu_add_item( app->message_menu, "Add new...", FlipperHamMessageIndexAdd, m, app);

    for(i = 0; i < TXT_N; i++)
    {
        if(!app->message_used[i]) continue;
        if(!app->message[i][0]) continue;

        submenu_add_item(app->message_menu, app->message[i], FlipperHamMessageIndexBase + i, mx0, app);
    }
}

static void mfix(FlipperHamApp* app)
{
    uint8_t i;

    app->message_n = 0;

    for(i = 0; i < TXT_N; i++)
    {
        if(app->message[i][0]) app->message_used[i] = 1;
        else app->message_used[i] = 0;

        if(app->message_used[i]) app->message_n++;
    }
}

static void m(void* context, uint32_t index)
{
    FlipperHamApp* app = context;
    uint8_t i;

    if(index == FlipperHamMessageIndexAdd)
    {
        app->m_i = 0xff;

        for(i = 0; i < TXT_N; i++)
            if(!app->message_used[i] || !app->message[i][0]) {
                app->m_i = i;
                break;
            }

        if(app->m_i == 0xff) return;

        app->m_edit[0] = 0;
    }
    else
    {
        i = index - FlipperHamMessageIndexBase;
        if(i >= TXT_N) return;

        app->m_i = i;
        snprintf(app->m_edit, sizeof(app->m_edit), "%s", app->message[i]);
    }

    app->txt = 3;

    text_input_reset(app->text_input);
    text_input_set_header_text(app->text_input, "Message");
    text_input_set_result_callback(app->text_input, msave, app, app->m_edit, sizeof(app->m_edit), true);

    view_dispatcher_switch_to_view(app->view_dispatcher, FlipperHamViewTextInput);
}

static void mx(void* context, InputType input_type, uint32_t index)
{
    FlipperHamApp* app = context;
    uint8_t i;

    i = index - FlipperHamMessageIndexBase;
    if(i >= TXT_N) return;

    if(input_type == InputTypeShort)
    {
        app->tx_t = 2;
        app->s_i = i;

        view_dispatcher_switch_to_view(app->view_dispatcher, FlipperHamViewCall);
        return;
    }

    if(input_type != InputTypeLong) return;

    app->m_i = i; snprintf(app->m_edit, sizeof(app->m_edit), "%s", app->message[i]); app->txt = 3;

    text_input_reset(app->text_input);
    text_input_set_header_text(app->text_input, "Message");
    text_input_set_result_callback(app->text_input, msave, app, app->m_edit, sizeof(app->m_edit), true);



    view_dispatcher_switch_to_view(app->view_dispatcher, FlipperHamViewTextInput);
}

static void msave(void* context)
{
    FlipperHamApp* app = context;
    uint8_t i;

    i = app->m_i;
    if(i >= TXT_N) return;

    if(!app->m_edit[0])
    {
        app->message[i][0] = 0;
        app->message_used[i] = 0;
    }
    else
    {
        snprintf(app->message[i], sizeof(app->message[i]), "%s", app->m_edit);
        app->message_used[i] = 1;
    }

    mfix(app);
    cfgsave(app);
    mmenu(app);

    view_dispatcher_switch_to_view(app->view_dispatcher, FlipperHamViewMessage);
}

static void flipperham_send_callback(void* context, uint32_t index)
{
    FlipperHamApp* app = context;

    if(index == FlipperHamSendIndexMessage)
        view_dispatcher_switch_to_view(app->view_dispatcher, FlipperHamViewMessage);

    if(index == FlipperHamSendIndexBulletin)
        view_dispatcher_switch_to_view(app->view_dispatcher, FlipperHamViewBulletin);


    if(index == FlipperHamSendIndexStatus)
        view_dispatcher_switch_to_view(app->view_dispatcher, FlipperHamViewStatus);
}

static void flipperham_bulletin_callback(void* context, uint32_t index)
{
    FlipperHamApp* app = context;
    uint8_t i;

    if(index != FlipperHamBulletinIndexAdd) return;

    app->b_i = 0xff;

    for(i = 0; i < TXT_N; i++)
        if(!app->bulletin_used[i]) {
            app->b_i = i;
            break;
        }

    if(app->b_i == 0xff) return;

    app->b_edit[0] = 0;
    app->txt = 0;

    text_input_reset(app->text_input);
    text_input_set_header_text(app->text_input, "Bulletin");
    text_input_set_result_callback(app->text_input, bsave, app, app->b_edit, sizeof(app->b_edit), true);

    view_dispatcher_switch_to_view(app->view_dispatcher, FlipperHamViewTextInput);
}

static void bx(void* context, InputType input_type, uint32_t index)
{
    FlipperHamApp* app = context;
    uint8_t i;

    i = index - FlipperHamBulletinIndexBase;
    if(i >= TXT_N) return;

    if(input_type == InputTypeShort)
    {
        app->tx_t = 0;
        app->s_i = i;
        app->go_v = FlipperHamViewBulletin;
        app->send_requested = true;
        view_dispatcher_stop(app->view_dispatcher);
        return;
    }

    if(input_type != InputTypeLong) return;

    app->b_i = i;
    snprintf(app->b_edit, sizeof(app->b_edit), "%s", app->bulletin[i]);
    app->txt = 0;

    text_input_reset(app->text_input);
    text_input_set_header_text(app->text_input, "Bulletin");
    text_input_set_result_callback(app->text_input, bsave, app, app->b_edit, sizeof(app->b_edit), true);

    view_dispatcher_switch_to_view(app->view_dispatcher, FlipperHamViewTextInput);
}

static void bsave(void* context)
{
    FlipperHamApp* app = context;
    uint8_t i;

    i = app->b_i;
    if(i >= TXT_N) return;

    if(!app->b_edit[0])
    {
        app->bulletin[i][0] = 0;
        app->bulletin_used[i] = 0;
    }
    else
    {
        snprintf(app->bulletin[i], sizeof(app->bulletin[i]), "%s", app->b_edit);
        app->bulletin_used[i] = 1;
    }

    bfix(app);
    cfgsave(app);
    bmenu(app);

    view_dispatcher_switch_to_view(app->view_dispatcher, FlipperHamViewBulletin);
}

static void st(void* context, uint32_t index)
{
    FlipperHamApp* app = context;
    uint8_t i;

    if(index != FlipperHamStatusIndexAdd) return;

    app->st_i = 0xff;

    for(i = 0; i < TXT_N; i++)
        if(!app->status_used[i]) {
            app->st_i = i;
            break;
        }

    if(app->st_i == 0xff) return;

    app->st_edit[0] = 0;

    app->txt = 1;

    text_input_reset(app->text_input);
    text_input_set_header_text(app->text_input, "Status");
    text_input_set_result_callback(app->text_input, stsave, app, app->st_edit, sizeof(app->st_edit), true);

    view_dispatcher_switch_to_view(app->view_dispatcher, FlipperHamViewTextInput);
}

static void sx(void* context, InputType input_type, uint32_t index)
{
    FlipperHamApp* app = context;
    uint8_t i;

    i = index - FlipperHamStatusIndexBase;
    if(i >= TXT_N) return;

    if(input_type == InputTypeShort)
    {
        app->tx_t = 1;
        app->s_i = i;
        app->go_v = FlipperHamViewStatus;
        app->send_requested = true;
        view_dispatcher_stop(app->view_dispatcher);
        return;
    }

    if(input_type != InputTypeLong) return;

    app->st_i = i;
    snprintf(app->st_edit, sizeof(app->st_edit), "%s", app->status[i]);
    app->txt = 1;

    text_input_reset(app->text_input);
    text_input_set_header_text(app->text_input, "Status");
    text_input_set_result_callback(app->text_input, stsave, app, app->st_edit, sizeof(app->st_edit), true);

    view_dispatcher_switch_to_view(app->view_dispatcher, FlipperHamViewTextInput);
}

static void stsave(void* context)
{
    FlipperHamApp* app = context;
    uint8_t i;

    i = app->st_i;
    if(i >= TXT_N) return;

    if(!app->st_edit[0])
    {
        app->status[i][0] = 0;
        app->status_used[i] = 0;
    }
    else
    {
        snprintf(app->status[i], sizeof(app->status[i]), "%s", app->st_edit);
        app->status_used[i] = 1;
    }

    stfix(app);
    cfgsave(app);
    stmenu(app);

    view_dispatcher_switch_to_view(app->view_dispatcher, FlipperHamViewStatus);
}

static void cl(void* context, uint32_t index)
{
    FlipperHamApp* app = context;
    uint8_t i;

    if(index == FlipperHamCallIndexAdd)
    {
        app->c_i = 0xff;

        for(i = 0; i < CALL_N; i++)
            if(!app->calls_used[i] || !app->calls[i][0]) {
                app->c_i = i;
                break;
            }

        if(app->c_i == 0xff) return;

        app->c_edit[0] = 0;
    }
    else
    {
        i = index - FlipperHamCallIndexBase;
        if(i >= CALL_N) return;

        app->c_i = i;
        snprintf(app->c_edit, sizeof(app->c_edit), "%s", app->calls[i]);
    }

    app->txt = 2;

    text_input_reset(app->text_input);
    text_input_set_header_text(app->text_input, "Callsign");
    text_input_set_result_callback(app->text_input, csave, app, app->c_edit, sizeof(app->c_edit), true);

    view_dispatcher_switch_to_view(app->view_dispatcher, FlipperHamViewTextInput);
}

static void cb(void* context, uint32_t index)
{
    FlipperHamApp* app = context;
    uint8_t i;

    if(index == FlipperHamBookIndexAdd)
    {
        app->c_i = 0xff;

        for(i = 0; i < CALL_N; i++)
            if(!app->calls_used[i] || !app->calls[i][0]) {
                app->c_i = i;
                break;
            }

        if(app->c_i == 0xff) return;

        app->c_edit[0] = 0; app->txt = 4;
        text_input_reset(app->text_input);
        text_input_set_header_text(app->text_input, "Callsign");
        text_input_set_result_callback(app->text_input, csave, app, app->c_edit, sizeof(app->c_edit), true);
        view_dispatcher_switch_to_view(app->view_dispatcher, FlipperHamViewTextInput);
        return;
    }

    index -= FlipperHamBookIndexBase;
    if(index >= CALL_N) return;

    app->cb_i = index;
    snprintf(app->c2_h, sizeof(app->c2_h), "%s", app->calls[index]);
    c2m(app);
    view_dispatcher_switch_to_view(app->view_dispatcher, FlipperHamViewC2);
}

static void cx(void* context, InputType input_type, uint32_t index)
{
    FlipperHamApp* app = context;
    uint8_t i;
    uint8_t s;
    bool d;
    char a[CALL_LEN];

    i = index - FlipperHamCallIndexBase;
    if(i >= CALL_N) return;

    if(input_type == InputTypeShort)
    {
        if(app->tx_t == 2)
        {
            app->d_i = i;
            app->go_v = FlipperHamViewMessage;
            if(csplit(app->calls[i], a, &s, &d))
            {
                if(d)
                {
                    app->d_s = s;
                    app->send_requested = true;
                    view_dispatcher_stop(app->view_dispatcher);
                }
                else
                {
                    app->d_s = 0;
                    ssidfix(app);
                    view_dispatcher_switch_to_view(app->view_dispatcher, FlipperHamViewSsid);
                }
            }


            return;
        }
    }

    if(input_type != InputTypeLong) return;

    app->c_i = i; snprintf(app->c_edit, sizeof(app->c_edit), "%s", app->calls[i]);
    app->txt = 2;

    text_input_reset(app->text_input);
    text_input_set_header_text(app->text_input, "Callsign");
    text_input_set_result_callback(app->text_input, csave, app, app->c_edit, sizeof(app->c_edit), true);

    view_dispatcher_switch_to_view(app->view_dispatcher, FlipperHamViewTextInput);
}

static void csave(void* context)
{
    FlipperHamApp* app = context;
    uint8_t i;

    i = app->c_i;
    if(i >= CALL_N) return;

    if(!app->c_edit[0])
    {
        app->calls[i][0] = 0;
        app->calls_used[i] = 0;
    }
    else
    {
        if(!cval(app->c_edit)) {
            if(app->txt == 4) view_dispatcher_switch_to_view(app->view_dispatcher, FlipperHamViewBook);
            else view_dispatcher_switch_to_view(app->view_dispatcher, FlipperHamViewCall);
            return;
        }

        snprintf(app->calls[i], sizeof(app->calls[i]), "%s", app->c_edit);
        app->calls_used[i] = 1;
    }

    cfix(app);
    cfgsave(app);
    cmenu(app);
    bkmenu(app);

    if(app->txt == 4) view_dispatcher_switch_to_view(app->view_dispatcher, FlipperHamViewBook);
    else view_dispatcher_switch_to_view(app->view_dispatcher, FlipperHamViewCall);
}

static void c2(void* context, uint32_t index)
{
    FlipperHamApp* app = context;

    if(app->cb_i >= CALL_N) return;

    if(index == FlipperHamC2IndexEdit)
    {
        app->c_i = app->cb_i; snprintf(app->c_edit, sizeof(app->c_edit), "%s", app->calls[app->cb_i]); app->txt = 4;
        text_input_reset(app->text_input);
        text_input_set_header_text(app->text_input, "Callsign");
        text_input_set_result_callback(app->text_input, csave, app, app->c_edit, sizeof(app->c_edit), true);
        view_dispatcher_switch_to_view(app->view_dispatcher, FlipperHamViewTextInput);
        return;
    }

    if(index == FlipperHamC2IndexDelete)
    {
        app->calls[app->cb_i][0] = 0;
        app->calls_used[app->cb_i] = 0;
        cfix(app);
        cfgsave(app);
        cmenu(app);
        bkmenu(app);
        view_dispatcher_switch_to_view(app->view_dispatcher, FlipperHamViewBook);
        return;
    }

    if(index != FlipperHamC2IndexCopy) return;

    if(cpy(app))
    {
        view_dispatcher_switch_to_view(app->view_dispatcher, FlipperHamViewBook);
        return;
    }

    snprintf(app->c2_h, sizeof(app->c2_h), "No SSID free");
    c2m(app);
    view_dispatcher_switch_to_view(app->view_dispatcher, FlipperHamViewC2);
}

static bool csplit(const char* s, char* out, uint8_t* ssid, bool* has_ssid)
{
    char a[CALL_LEN];
    uint8_t i;
    uint8_t j;
    uint8_t k;
    uint8_t n;
    bool dash;

    i = 0;
    j = 0;
    n = 0;
    dash = false;

    while(s[i] && s[i] != '-' && s[i] != '_') {
        char c = s[i];

        if(c >= 'a' && c <= 'z') c -= 32;
        if(!((c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9'))) return false;
        if(j >= CALL_LEN - 1) return false;

        a[j++] = c;
        i++;
    }

    if(s[i] == '_') dash = true;
    if(s[i] == '-') dash = true;

    if(!dash && j <= 6)
    {
        if(!j) return false;
        a[j] = 0;
        snprintf(out, CALL_LEN, "%s", a);
        *has_ssid = false;
        *ssid = 0;
        return true;
    }

    if(!dash)
    {
        k = j;
        while(k && a[k - 1] >= '0' && a[k - 1] <= '9') k--;
        if(k == j) return false;
        if(k > 6) return false;
        if(j - k > 2) return false;
        if(!k) return false;

        n = 0;
        for(i = k; i < j; i++) n = (n * 10) + (a[i] - '0');
        if(n > 15) return false;

        a[k] = 0;
        snprintf(out, CALL_LEN, "%s", a);
        *has_ssid = true;
        *ssid = n;
        return true;
    }

    if(j > 6) return false;
    if(!j) return false;
    i++;
    if(!s[i]) return false;

    n = 0;
    k = 0;

    while(s[i]) {
        char c = s[i];

        if(c >= 'a' && c <= 'z') c -= 32;
        if(c < '0' || c > '9') return false;
        n = (n * 10) + (c - '0');
        k++;
        i++;
    }

    if(!k) return false;
    if(k > 2) return false;
    if(n > 15) return false;

    a[j] = 0;
    snprintf(out, CALL_LEN, "%s", a);
    *has_ssid = true;
    *ssid = n;
    return true;
}

static bool cval(char* s)
{
    char a[CALL_LEN];
    uint8_t b;
    uint8_t i;
    uint8_t j;
    bool d;

    if(!csplit(s, a, &b, &d)) return false;

    if(d)
    {
        i = 0;
        j = 0;
        while(a[i]) s[j++] = a[i++];
        s[j++] = '-';
        if(b >= 10) s[j++] = '0' + (b / 10);
        s[j++] = '0' + (b % 10);
        s[j] = 0;
    }
    else snprintf(s, CALL_LEN, "%s", a);

    return true;
}

static void flipperham_draw_callback(Canvas* canvas, void* context) 
{
    FlipperHamApp* app = context;
    char a[16];
    uint32_t n, w, m;

    canvas_clear(canvas);
    canvas_set_font(canvas, FontPrimary);

    if(!app->tx_allowed) 
    {
        canvas_draw_str_aligned(canvas, 64, 32, AlignCenter, AlignCenter, "TX blocked");
        return;
    }

    if(app->done_w) {
        canvas_draw_str_aligned(canvas, 64, 32, AlignCenter, AlignCenter, "Done");
        return;
    }

    if(app->repeat_n >= 4) canvas_draw_str_aligned(canvas, 64, 24, AlignCenter, AlignCenter, "Sending...");
    else if(app->repeat_n > 1) canvas_draw_str_aligned(canvas, 64, 26, AlignCenter, AlignCenter, "Sending...");
    else canvas_draw_str_aligned(canvas, 64, 32, AlignCenter, AlignCenter, "Sending...");

    if(app->repeat_n > 1)
    {
        snprintf(a, sizeof(a), "%u/%u", app->repeat_i, app->repeat_n);
        canvas_set_font(canvas, FontSecondary);
        if(app->repeat_n >= 4) canvas_draw_str_aligned(canvas, 64, 42, AlignCenter, AlignCenter, a);
        else canvas_draw_str_aligned(canvas, 64, 38, AlignCenter, AlignCenter, a);
        canvas_set_font(canvas, FontPrimary);
    }

    if(app->repeat_n >= 4)
    {
        n = furi_get_tick() - app->repeat_t0;
        m = (app->repeat_n >= 5) ? 15000 : 8000;
        n = (n > m) ? m : n;

        /* bar frame */
        canvas_draw_box(canvas, 24, 31, 80, 1);
        canvas_draw_box(canvas, 24, 35, 80, 1);
        canvas_draw_box(canvas, 23, 32, 1, 3);
        canvas_draw_box(canvas, 104, 32, 1, 3);

        /* leave corners dead so it looks round-ish */
        w = (n * 80UL) / m;
        if(w) canvas_draw_box(canvas, 24, 32, w, 3);
    }
}

static uint16_t flipperham_segment_tone_hz(uint16_t duration_us)
{
    if(duration_us >= 300) {
        return flipperham_modem_profile->mark_hz;
    }

    return flipperham_modem_profile->space_hz;
}

static void flipperham_load_first_segment(FlipperHamApp* app) 
{
    app->segment_index = 0;
    app->level = true;
    app->tx_done = false;
    app->current_half_period_us = 417;
    app->current_tone_hz = flipperham_segment_tone_hz(app->current_half_period_us);
}

static bool add(FlipperHamApp* app, uint16_t a)
{
    int32_t b;

    if(!app->wave) return false;
    /* cap generated edges */
    if(app->wave_n >= 4096) return false;

    b = a + app->wave_c;
    a = (b + 16) / 33;
    app->wave_c = b - ((int32_t)a * 33);
    if(!a) return true;

    app->wave[app->wave_n++] = a;
    return true;
}

static bool put(FlipperHamApp* app, uint8_t bit)
{
    if(bit == 0) app->wave_mark = !app->wave_mark;

    if(app->wave_mark)
    {
        if(!add(app, 13750)) return false;
        if(!add(app, 13750)) return false;
    }
    else
    {
        if(!add(app, 7500)) return false;
        if(!add(app, 7500)) return false;
        if(!add(app, 7500)) return false;
        if(!add(app, 5000)) return false;
    }

    return true;
}

static bool flag3(FlipperHamApp* app)
{
    static const uint8_t a[] = {0, 1, 1, 1, 1, 1, 1, 0};
    uint8_t i;

    for(i = 0; i < sizeof(a); i++) {
        if(!put(app, a[i])) return false;
    }

    return true;
}

static void txstart(FlipperHamApp* app)
{
    char a[96];
    char b;
    char c[CALL_LEN];
    char d2[CALL_LEN];
    uint8_t j;
    uint16_t i;
    uint8_t s;
    bool d;

    app->tx_done = false;
    app->segment_index = 0;
    app->level = true;
    app->wave_n = 0;
    app->wave_c = 0;
    app->wave_mark = true;
    app->tx.bits = NULL;
    app->tx.bits_n = 0;
    app->tx.i = 0;
    app->tx.k = 0;
    app->tx.n = 0;
    app->tx.c = 0;
    app->tx.mark = true;
    app->tx.level = true;
    app->tx.half_us = 0;

      app->tx.half_left_us = 0; // no
      app->tx.bit_left_us = 0;
      app->tx.part[0] = 0;
      app->tx.part[1] = 0;
      app->tx.part[2] = 0;
      app->tx.part[3] = 0;

    if(!app->pkt) return;
    if(!app->wave) return;
    if(app->s_i >= TXT_N) return;

    /* bulleting message */
    if(app->tx_t == 0)
    {
        if(!app->bulletin_used[app->s_i]) return;
        if(!app->bulletin[app->s_i][0]) return;

        b = '0';
        if(app->s_i < 10) b = '0' + app->s_i;
        else if(app->s_i < 16) b = 'A' + (app->s_i - 10);

        snprintf(a, sizeof(a), ":BLN%c     :%s", b, app->bulletin[app->s_i]);
    }
    /* status message */
    else if(app->tx_t == 1)
    {
        if(!app->status_used[app->s_i]) return;
        if(!app->status[app->s_i][0]) return;

        snprintf(a, sizeof(a), ">%s", app->status[app->s_i]);
    }
    /* type: aprs direct */
    else
    {
        if(app->d_i >= CALL_N) return;
        if(!app->message_used[app->s_i]) return;
        if(!app->message[app->s_i][0]) return;
        if(!app->calls_used[app->d_i]) return;
        if(!app->calls[app->d_i][0]) return;

        if(!csplit(app->calls[app->d_i], c, &s, &d)) return;
        if(!d) s = app->d_s;

        j = 0;
        while(c[j]) {
            d2[j] = c[j];
            j++;
        }
        d2[j++] = '-';
        if(s >= 10) d2[j++] = '0' + (s / 10);
        d2[j++] = '0' + (s % 10);
        d2[j] = 0;
        snprintf(a, sizeof(a), ":%-9s:%s", d2, app->message[app->s_i]);
    }

    packet_init(app->pkt);
    snprintf((char*)app->pkt->payload, sizeof(app->pkt->payload), "%s", a);
    app->pkt->payload_len = strlen((char*)app->pkt->payload);
    packet_make_ax25(app->pkt, MY_CALL, 0, MY_TOCALL, 0);
    packet_add_fcs(app->pkt);
    packet_stuff(app->pkt);
    packet_nrzi(app->pkt);

    // 50ms mark
    for(i = 0; i < 60 && put(app, 1); i++);

    // preamble
    for(i = 0; i < 50; i++) if(!flag3(app)) break;

    // skip the flag at head and tail, add our own
    for(i = 8; i + 8 < app->pkt->stuffed_len; i++) {
        if(!put(app, app->pkt->stuffed[i])) break;
    }

    // post
    for(i = 0; i < 3; i++) if(!flag3(app)) break;

    if(app->wave_n) {
        app->current_half_period_us = app->wave[0];
        app->current_tone_hz = flipperham_segment_tone_hz(app->current_half_period_us);
    } else {
        app->tx_done = true;
    }
}

static LevelDuration edge_yield(void* context) 
{
    FlipperHamApp* app = context;
    LevelDuration ld;
    uint16_t a;

    if(app->tx_done) return level_duration_reset();

    /*
    a = app->current_half_period_us;
    app->current_tone_hz = flipperham_segment_tone_hz(a);
    ld = level_duration_make(app->level, a);
    */

    if(app->segment_index >= app->wave_n) {
        app->tx_done = true;
        return level_duration_reset();
    }

    a = app->wave[app->segment_index];

    app->current_half_period_us = a;
    app->current_tone_hz = flipperham_segment_tone_hz(a);
    ld = level_duration_make(app->level, a);

    app->level = !app->level;
    app->segment_index++;

    if(app->segment_index >= app->wave_n) app->tx_done = true;

    return ld;
}

static void flipperham_radio_start(FlipperHamApp* app) 
{
    furi_hal_subghz_reset();
    furi_hal_subghz_idle();
    furi_hal_subghz_load_custom_preset((uint8_t*)flipperham_preset->regs);
    furi_hal_subghz_set_frequency_and_path(CARRIER_HZ);
    furi_hal_subghz_flush_tx();

    if(!furi_hal_subghz_tx()) {
        app->tx_allowed = false;
        app->tx_done = true;
        return;
    }

    app->tx_allowed = furi_hal_subghz_start_async_tx(edge_yield, app);
    app->tx_started = app->tx_allowed;
    if(!app->tx_allowed) app->tx_done = true;
}

static void flipperham_radio_stop(FlipperHamApp* app) 
{
    if(app->tx_started) 
        furi_hal_subghz_stop_async_tx();

    furi_hal_subghz_sleep();
}

static FlipperHamApp* flipperham_app_alloc(void) {
    FlipperHamApp* app = malloc(sizeof(FlipperHamApp));

    app->gui = furi_record_open(RECORD_GUI);
    app->view_dispatcher = view_dispatcher_alloc();
    app->submenu = submenu_alloc();
    app->send_menu = submenu_alloc();
    app->bulletin_menu = submenu_alloc();
    app->status_menu = submenu_alloc();
    app->message_menu = submenu_alloc();
    app->call_menu = submenu_alloc();
    app->book_menu = submenu_alloc();
    app->c2_menu = submenu_alloc();
    app->settings_menu = variable_item_list_alloc();
    app->ssid_menu = variable_item_list_alloc();
    app->text_input = text_input_alloc();

        app->view_port = NULL;
        app->tx_started = false;
        app->tx_allowed = true;
        app->tx_done = false;
        app->done_w = false;
        app->send_requested = false;
        app->repeat_w = false;
        app->encoding_index = FlipperHamModemProfileDefault;
        app->repeat_n = 1;
        app->repeat_i = 1;
        app->s_i = 0;
        app->tx_t = 0;
        app->st_i = 0;
        app->m_i = 0;
        app->d_i = 0;
        app->d_s = 0;
        app->c_i = 0;
        app->cb_i = 0;
        app->c2_h[0] = 0;
        app->go_v = FlipperHamViewMenu;
        app->txt = 0;
        app->pkt = malloc(sizeof(Packet));
        app->wave = malloc(sizeof(uint16_t) * 4096);

        cfgload(app);

    view_dispatcher_enable_queue( app->view_dispatcher);
    view_dispatcher_attach_to_gui(app->view_dispatcher, app->gui, ViewDispatcherTypeFullscreen);

    submenu_add_item( app->submenu, "Send", FlipperHamMenuIndexSend, flipperham_menu_callback, app);
    submenu_add_item( app->submenu, "Settings", FlipperHamMenuIndexSettings, flipperham_menu_callback, app);
    submenu_add_item( app->submenu, "Callbook", FlipperHamMenuIndexCallbook, flipperham_menu_callback, app);


    submenu_add_item( app->send_menu, "Message", FlipperHamSendIndexMessage, flipperham_send_callback, app);
    submenu_add_item( app->send_menu, "Status", FlipperHamSendIndexStatus, flipperham_send_callback, app);
    submenu_add_item( app->send_menu, "Bulletin", FlipperHamSendIndexBulletin, flipperham_send_callback, app);


    bmenu(app);
    stmenu(app);
    mmenu(app);
    cmenu(app);
    bkmenu(app);
    rmenu(app);
    ssidfix(app);


    view_set_previous_callback(submenu_get_view(app->submenu), flipperham_exit_callback);
    view_set_previous_callback(submenu_get_view(app->send_menu), flipperham_send_exit_callback);
    view_set_previous_callback(variable_item_list_get_view(app->settings_menu), flipperham_settings_exit_callback);
    view_set_previous_callback(submenu_get_view(app->bulletin_menu), flipperham_bulletin_exit_callback);
    view_set_previous_callback(submenu_get_view(app->status_menu), flipperham_status_exit_callback);
    view_set_previous_callback(submenu_get_view(app->message_menu), flipperham_message_exit_callback);
    view_set_previous_callback(submenu_get_view(app->call_menu), flipperham_call_exit_callback);
    view_set_previous_callback(submenu_get_view(app->book_menu), bookx);
    view_set_previous_callback(submenu_get_view(app->c2_menu), c2x);
    view_set_previous_callback(variable_item_list_get_view(app->ssid_menu), flipperham_ssid_exit_callback);
    view_set_previous_callback(text_input_get_view(app->text_input), flipperham_text_exit_callback);
    variable_item_list_set_enter_callback(app->ssid_menu, se, app);


    view_dispatcher_add_view( app->view_dispatcher, FlipperHamViewMenu, submenu_get_view(app->submenu));
    view_dispatcher_add_view( app->view_dispatcher, FlipperHamViewSend, submenu_get_view(app->send_menu));
    view_dispatcher_add_view( app->view_dispatcher, FlipperHamViewSettings, variable_item_list_get_view(app->settings_menu));
    view_dispatcher_add_view( app->view_dispatcher, FlipperHamViewBulletin, submenu_get_view(app->bulletin_menu));
    view_dispatcher_add_view( app->view_dispatcher, FlipperHamViewStatus, submenu_get_view(app->status_menu));
    view_dispatcher_add_view( app->view_dispatcher, FlipperHamViewMessage, submenu_get_view(app->message_menu));
    view_dispatcher_add_view( app->view_dispatcher, FlipperHamViewCall, submenu_get_view(app->call_menu));
    view_dispatcher_add_view( app->view_dispatcher, FlipperHamViewBook, submenu_get_view(app->book_menu));
    view_dispatcher_add_view( app->view_dispatcher, FlipperHamViewC2, submenu_get_view(app->c2_menu));
    view_dispatcher_add_view( app->view_dispatcher, FlipperHamViewSsid, variable_item_list_get_view(app->ssid_menu));
    view_dispatcher_add_view( app->view_dispatcher, FlipperHamViewTextInput, text_input_get_view(app->text_input));
    view_dispatcher_switch_to_view(app->view_dispatcher, FlipperHamViewMenu);

    flipperham_load_first_segment(app);

    return app;
}

static void flipperham_menu_free(FlipperHamApp* app) 
{
    if(app->view_dispatcher) {
        view_dispatcher_remove_view(app->view_dispatcher, FlipperHamViewMenu);
        view_dispatcher_remove_view(app->view_dispatcher, FlipperHamViewSend);
        view_dispatcher_remove_view(app->view_dispatcher, FlipperHamViewSettings);
        view_dispatcher_remove_view(app->view_dispatcher, FlipperHamViewBulletin);
        view_dispatcher_remove_view(app->view_dispatcher, FlipperHamViewStatus);
        view_dispatcher_remove_view(app->view_dispatcher, FlipperHamViewMessage);
        view_dispatcher_remove_view(app->view_dispatcher, FlipperHamViewCall);
        view_dispatcher_remove_view(app->view_dispatcher, FlipperHamViewBook);
        view_dispatcher_remove_view(app->view_dispatcher, FlipperHamViewC2);
        view_dispatcher_remove_view(app->view_dispatcher, FlipperHamViewSsid);
        view_dispatcher_remove_view(app->view_dispatcher, FlipperHamViewTextInput);
        view_dispatcher_free(app->view_dispatcher);
        app->view_dispatcher = NULL;
    }

    if(app->submenu) 
    {
        submenu_free(app->submenu);
        app->submenu = NULL;
    }

    if(app->send_menu) 
    {
        submenu_free(app->send_menu);
        app->send_menu = NULL;
    }

    if(app->settings_menu)
    {
        variable_item_list_free(app->settings_menu);
        app->settings_menu = NULL;
    }

    if(app->bulletin_menu) 
    {
        submenu_free(app->bulletin_menu);
        app->bulletin_menu = NULL;
    }

    if(app->status_menu) 
    {
        submenu_free(app->status_menu);
        app->status_menu = NULL;
    }

    if(app->message_menu) 
    {
        submenu_free(app->message_menu);
        app->message_menu = NULL;
    }

    if(app->call_menu) 
    {
        submenu_free(app->call_menu);
        app->call_menu = NULL;
    }

    if(app->book_menu) 
    {
        submenu_free(app->book_menu);
        app->book_menu = NULL;
    }

    if(app->c2_menu) 
    {
        submenu_free(app->c2_menu);
        app->c2_menu = NULL;
    }

    if(app->ssid_menu)
    {
        variable_item_list_free(app->ssid_menu);
        app->ssid_menu = NULL;
    }

    if(app->text_input)
    {
        text_input_free(app->text_input);
        app->text_input = NULL;
    }
}

static void flipperham_status_view_alloc(FlipperHamApp* app) 
{
    app->view_port = view_port_alloc();
    view_port_draw_callback_set(app->view_port, flipperham_draw_callback, app);
    gui_add_view_port(app->gui, app->view_port, GuiLayerFullscreen);
}

static void flipperham_status_view_free(FlipperHamApp* app)
{
    if(!app->view_port) {
        return;
    }

    gui_remove_view_port(app->gui, app->view_port);
    view_port_free(app->view_port);
    app->view_port = NULL;
}

static void gblink(void)
{
    uint8_t i;

    for(i = 0; i < 2; i++)
    {
        furi_hal_light_set(LightGreen, 255); furi_delay_ms(50);
        furi_hal_light_set(LightGreen, 0); if(i + 1 < 2) furi_delay_ms(50);
    }
}

static void flipperham_app_free(FlipperHamApp* app) 
{
    if(!app) return;

    flipperham_status_view_free(app);
    flipperham_menu_free(app);
    if(app->pkt) free(app->pkt);
    if(app->wave) free(app->wave);
    if(app->gui) furi_record_close(RECORD_GUI);
    free(app);
}

static void flipperham_send_hardcoded_message(FlipperHamApp* app) 
{
    static const uint32_t a[] = {0, 2000, 4000, 8000, 15000};
    uint8_t i;
    uint32_t b;

    flipperham_status_view_alloc(app);
    app->repeat_i = 0;
    app->repeat_t0 = furi_get_tick();
    app->repeat_to = 0;
    app->repeat_w = false;
    app->done_w = false;

    for(i = 0; i < app->repeat_n; i++)
    {
        app->repeat_i = i + 1;
        app->repeat_w = false;

        txstart(app);
        /* flipperham_load_first_segment(app); // old table */
        app->tx_started = false;
        app->tx_allowed = true;
        app->done_w = false;

        view_port_update(app->view_port);
        furi_delay_ms(100);

        furi_hal_power_suppress_charge_enter();
        flipperham_radio_start(app);

        while(!app->tx_done) {
            view_port_update(app->view_port);
            furi_delay_ms(50);
        }

        while(app->tx_started && !furi_hal_subghz_is_async_tx_complete()) {
            view_port_update(app->view_port);
            furi_delay_ms(20);
        }

            flipperham_radio_stop(app); // do it before
        furi_hal_power_suppress_charge_exit();
        gblink();

        if(i + 1 >= app->repeat_n) break;

        app->repeat_w = true;
        app->repeat_to = a[i + 1];
        app->tx_done = false;

        while(1)
        {
            b = furi_get_tick() - app->repeat_t0;
            if(b >= app->repeat_to) break;

            view_port_update(app->view_port);
            furi_delay_ms(50);
        }
    }

    app->repeat_w = false;
    app->done_w = true;
    app->tx_done = true;
    view_port_update(app->view_port);
    furi_delay_ms(750);
    furi_hal_light_set(LightGreen, 0);

    flipperham_status_view_free(app);
}

static void bx0(void* context, uint32_t index) { bx(context, InputTypeShort, index); }
static void sx0(void* context, uint32_t index) { sx(context, InputTypeShort, index); }
static void cx0(void* context, uint32_t index) { cx(context, InputTypeShort, index); }
static void mx0(void* context, uint32_t index) { mx(context, InputTypeShort, index); }


int32_t flipperham_app(void* p)
{
    UNUSED(p);

    FlipperHamApp* app = flipperham_app_alloc();

    while(1) 
    {
        app->send_requested = false;
        view_dispatcher_switch_to_view(app->view_dispatcher, app->go_v);
        view_dispatcher_run(app->view_dispatcher);



        if(!app->send_requested) break;

        flipperham_send_hardcoded_message(app);
    }

    flipperham_app_free(app);
    return 0;
}
