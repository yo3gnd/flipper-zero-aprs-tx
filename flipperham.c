#include "flipperham.h"
#include "packet.h"
#include "app_state.h"
#include "rf_gen.h"

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
bool csplit(const char* s, char* out, uint8_t* ssid, bool* has_ssid);
static void bfix(FlipperHamApp* app);
static void stfix(FlipperHamApp* app);
static void mfix(FlipperHamApp* app);
static void cfix(FlipperHamApp* app);
static void bkmenu(FlipperHamApp* app);
static void c2m(FlipperHamApp* app);
static void cloadtxt(FlipperHamApp* app);
static void csavetxt(FlipperHamApp* app);
static void bc(VariableItem* item);
static void sc(VariableItem* item);
static void se(void* context, uint32_t index);
static void smenu(FlipperHamApp* app);
static void rc(VariableItem* item);
static void pc(VariableItem* item);
static void dc(VariableItem* item);

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
    app->rf_m = 0; app->rf_d = 1;


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

    pf(app);
}

static void cfgsave(FlipperHamApp* app)
{
    Storage* storage;
    File* file;
    FlipperHamCfg* c;

    c = malloc(sizeof(FlipperHamCfg));
    if(!c) return;
    memset(c, 0, sizeof(FlipperHamCfg));

    c->encoding_index = app->encoding_index;
    c->rf_m = app->rf_m;
    c->rf_d = app->rf_d;

    memcpy(c->bulletin, app->bulletin, sizeof(c->bulletin));
    memcpy(c->status, app->status, sizeof(c->status));


    memcpy(c->message, app->message, sizeof(c->message));


    memcpy(c->bulletin_used, app->bulletin_used, sizeof(c->bulletin_used));
    memcpy(c->status_used, app->status_used, sizeof(c->status_used));
    memcpy(c->message_used, app->message_used, sizeof(c->message_used));

    c->bulletin_n = app->bulletin_n;
    c->status_n = app->status_n;

    c->message_n = app->message_n;

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

static void csavetxt(FlipperHamApp* app)
{
    Storage* storage;
    File* file;
    uint8_t i;
    uint16_t n;

    storage = furi_record_open(RECORD_STORAGE);
    file = storage_file_alloc(storage);

    storage_common_mkdir(storage, CALLBOOK_DIR);

    if(storage_file_open(file, CALLBOOK_FILE, FSAM_WRITE, FSOM_CREATE_ALWAYS))
    {
        for(i = 0; i < CALL_N; i++)
        {
            if(!app->calls_used[i]) continue;
            if(!app->calls[i][0]) continue;

            n = strlen(app->calls[i]);
            if(n) storage_file_write(file, app->calls[i], n);
            storage_file_write(file, "\n", 1);
        }
    }

    storage_file_close(file);
    storage_file_free(file);
    furi_record_close(RECORD_STORAGE);
}

static void cloadtxt(FlipperHamApp* app)
{
    Storage* storage;
    File* file;
    char a[CALL_LEN + 4];
    char c;
    uint8_t i;
    uint8_t j;

    memset(app->calls, 0, sizeof(app->calls));
    memset(app->calls_used, 0, sizeof(app->calls_used));
    app->calls_n = 0;

    storage = furi_record_open(RECORD_STORAGE);
    file = storage_file_alloc(storage);

    storage_common_mkdir(storage, CALLBOOK_DIR);

    if(!storage_file_open(file, CALLBOOK_FILE, FSAM_READ, FSOM_OPEN_EXISTING))
    {
        storage_file_free(file);
        furi_record_close(RECORD_STORAGE);
        snprintf(app->calls[0], sizeof(app->calls[0]), "FL1PER");
        snprintf(app->calls[1], sizeof(app->calls[1]), "YO3GND-12");
        app->calls_used[0] = 1;
        app->calls_used[1] = 1;
        app->calls_n = 2;
        csavetxt(app);
        return;
    }

    i = 0;
    j = 0;

    while(storage_file_read(file, &c, 1) == 1)
    {
        if(c == '\r') continue;

        if(c == '\n')
        {
            a[i] = 0;

            if(a[0] && j < CALL_N && cval(a))
            {
                i = 0;
                while(a[i]) {
                    app->calls[j][i] = a[i];
                    i++;
                }
                app->calls[j][i] = 0; app->calls_used[j] = 1;
                j++;
            }

            i = 0;
            continue;
        }

        if(i < sizeof(a) - 1) a[i++] = c;
    }

    if(i)
    {
        a[i] = 0;

        if(a[0] && j < CALL_N && cval(a))
        {
            i = 0;
            while(a[i]) {
                app->calls[j][i] = a[i];
                i++;
            }
            app->calls[j][i] = 0; app->calls_used[j] = 1;
            j++;
        }
    }

    storage_file_close(file);
    storage_file_free(file);
    furi_record_close(RECORD_STORAGE);

    cfix(app);
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
        cloadtxt(app);
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
        cloadtxt(app);
        return;
    }

    app->encoding_index = c->encoding_index;
    app->rf_m = c->rf_m;
    app->rf_d = c->rf_d;

    memcpy(app->bulletin, c->bulletin, sizeof(app->bulletin));
    memcpy(app->status, c->status, sizeof(app->status));
    memcpy(app->message, c->message, sizeof(app->message));

    memcpy(app->bulletin_used, c->bulletin_used, sizeof(app->bulletin_used));
    memcpy(app->status_used, c->status_used, sizeof(app->status_used));
    memcpy(app->message_used, c->message_used, sizeof(app->message_used));

    app->bulletin_n = c->bulletin_n;
    app->status_n = c->status_n;

    app->message_n = c->message_n;

    if(app->encoding_index >= (sizeof(flipperham_modem_profiles) / sizeof(flipperham_modem_profiles[0])))
        app->encoding_index = FlipperHamModemProfileDefault;

    pf(app);

    for(i = 0; i < TXT_N; i++)
    {
        app->bulletin[i][TXT_LEN - 1] = 0;
        app->status[i][TXT_LEN - 1] = 0;
        app->message[i][TXT_LEN - 1] = 0;
    }

    bfix(app);
    stfix(app);
    mfix(app);
    cloadtxt(app);

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

    if(app->book_call_index >= CALL_N) return false;
    if(!app->calls_used[app->book_call_index]) return false;
    if(!app->calls[app->book_call_index][0]) return false;
    if(!csplit(app->calls[app->book_call_index], a, &s, &d)) return false;

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
            if(x == app->book_call_index) continue;
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
                csavetxt(app);
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

static void bc(VariableItem* item)
{
    FlipperHamApp* app = variable_item_get_context(item);

    app->encoding_index = variable_item_get_current_value_index(item);
    if(app->encoding_index >= sizeof(flipperham_modem_profiles) / sizeof(flipperham_modem_profiles[0]))
        app->encoding_index = FlipperHamModemProfileDefault;

    variable_item_set_current_value_text(item, flipperham_modem_profiles[app->encoding_index].name);
    cfgsave(app);
}

static void pc(VariableItem* item)
{
    FlipperHamApp* app = variable_item_get_context(item);

    app->rf_m = variable_item_get_current_value_index(item);
    if(app->rf_m > 1) app->rf_m = 0;

    variable_item_set_current_value_text(item, app->rf_m ? "GFSK" : "2FSK");
    pf(app);
    cfgsave(app);
}

static void dc(VariableItem* item)
{
    FlipperHamApp* app = variable_item_get_context(item);

    app->rf_d = variable_item_get_current_value_index(item);
    if(app->rf_d > 1) app->rf_d = 1;

    variable_item_set_current_value_text(item, app->rf_d ? "5.0" : "2.5");
    pf(app);
    cfgsave(app);
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

    it = variable_item_list_add(app->settings_menu, "Baud", sizeof(flipperham_modem_profiles) / sizeof(flipperham_modem_profiles[0]), bc, app);
    variable_item_set_current_value_index(it, app->encoding_index);
    variable_item_set_current_value_text(it, flipperham_modem_profiles[app->encoding_index].name);

    it = variable_item_list_add(app->settings_menu, "CC1101 profile", 2, pc, app);
    variable_item_set_current_value_index(it, app->rf_m);
    variable_item_set_current_value_text(it, app->rf_m ? "GFSK" : "2FSK");

    it = variable_item_list_add(app->settings_menu, "Deviation", 2, dc, app);
    variable_item_set_current_value_index(it, app->rf_d);
    variable_item_set_current_value_text(it, app->rf_d ? "5.0" : "2.5");

    it = variable_item_list_add(app->settings_menu, "Repeat TX", 5, rc, app);
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
        app->message_index = 0xff;

        for(i = 0; i < TXT_N; i++)
            if(!app->message_used[i] || !app->message[i][0]) {
                app->message_index = i;
                break;
            }

        if(app->message_index == 0xff) return;

        app->m_edit[0] = 0;
    }
    else
    {
        i = index - FlipperHamMessageIndexBase;
        if(i >= TXT_N) return;

        app->message_index = i;
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
        app->tx_msg_index = i;

        view_dispatcher_switch_to_view(app->view_dispatcher, FlipperHamViewCall);
        return;
    }

    if(input_type != InputTypeLong) return;

    app->message_index = i;
    snprintf(app->m_edit, sizeof(app->m_edit), "%s", app->message[i]);
    app->txt = 3;
    text_input_reset(app->text_input);
    text_input_set_header_text(app->text_input, "Message");
    text_input_set_result_callback(app->text_input, msave, app, app->m_edit, sizeof(app->m_edit), true);
    view_dispatcher_switch_to_view(app->view_dispatcher, FlipperHamViewTextInput);
}

static void msave(void* context)
{
    FlipperHamApp* app = context;
    uint8_t i;

    i = app->message_index;
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

    app->bulletin_index = 0xff;

    for(i = 0; i < TXT_N; i++)
        if(!app->bulletin_used[i]) {
            app->bulletin_index = i;
            break;
        }

    if(app->bulletin_index == 0xff) return;

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
        app->tx_msg_index = i;
        app->go_v = FlipperHamViewBulletin;
        app->send_requested = true;
        view_dispatcher_stop(app->view_dispatcher);
        return;
    }

    if(input_type != InputTypeLong) return;

    app->bulletin_index = i;
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

    i = app->bulletin_index;
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

    app->status_index = 0xff;

    for(i = 0; i < TXT_N; i++)
        if(!app->status_used[i]) {
            app->status_index = i;
            break;
        }

    if(app->status_index == 0xff) return;

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
        app->tx_msg_index = i;
        app->go_v = FlipperHamViewStatus;
        app->send_requested = true;
        view_dispatcher_stop(app->view_dispatcher);
        return;
    }

    if(input_type != InputTypeLong) return;

    app->status_index = i;
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

    i = app->status_index;
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
        app->edit_call_index = 0xff;

        for(i = 0; i < CALL_N; i++)
            if(!app->calls_used[i] || !app->calls[i][0]) {
                app->edit_call_index = i;
                break;
            }

        if(app->edit_call_index == 0xff) return;

        app->c_edit[0] = 0;
    }
    else
    {
        i = index - FlipperHamCallIndexBase;
        if(i >= CALL_N) return;

        app->edit_call_index = i;
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
        app->edit_call_index = 0xff;

        for(i = 0; i < CALL_N; i++)
            if(!app->calls_used[i] || !app->calls[i][0]) {
                app->edit_call_index = i;
                break;
            }

        if(app->edit_call_index == 0xff) return;

        app->c_edit[0] = 0;
        app->txt = 4;
        text_input_reset(app->text_input);
        text_input_set_header_text(app->text_input, "Callsign");
        text_input_set_result_callback(app->text_input, csave, app, app->c_edit, sizeof(app->c_edit), true);
        view_dispatcher_switch_to_view(app->view_dispatcher, FlipperHamViewTextInput);
        return;
    }

    index -= FlipperHamBookIndexBase;
    if(index >= CALL_N) return;

    app->book_call_index = index;
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
            app->dst_call_index = i;
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

    app->edit_call_index = i;
    snprintf(app->c_edit, sizeof(app->c_edit), "%s", app->calls[i]);
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

    i = app->edit_call_index;
    if(i >= CALL_N) return;

    if(!app->c_edit[0])
    {
        app->calls[i][0] = 0;
        app->calls_used[i] = 0;
    }
    else
    {
        if(!cval(app->c_edit))
        {
            if(app->txt == 4) view_dispatcher_switch_to_view(app->view_dispatcher, FlipperHamViewBook);
            else view_dispatcher_switch_to_view(app->view_dispatcher, FlipperHamViewCall);
            return;
        }

        snprintf(app->calls[i], sizeof(app->calls[i]), "%s", app->c_edit);
        app->calls_used[i] = 1;
    }

    cfix(app);
    cfgsave(app);
    csavetxt(app);
    cmenu(app);
    bkmenu(app);

    if(app->txt == 4) view_dispatcher_switch_to_view(app->view_dispatcher, FlipperHamViewBook);
    else view_dispatcher_switch_to_view(app->view_dispatcher, FlipperHamViewCall);
}

static void c2(void* context, uint32_t index)
{
    FlipperHamApp* app = context;

    if(app->book_call_index >= CALL_N) return;

    if(index == FlipperHamC2IndexEdit)
    {
        app->edit_call_index = app->book_call_index;
        snprintf(app->c_edit, sizeof(app->c_edit), "%s", app->calls[app->book_call_index]);
        app->txt = 4;
        text_input_reset(app->text_input);
        text_input_set_header_text(app->text_input, "Callsign");
        text_input_set_result_callback(app->text_input, csave, app, app->c_edit, sizeof(app->c_edit), true);
        view_dispatcher_switch_to_view(app->view_dispatcher, FlipperHamViewTextInput);
        return;
    }

    if(index == FlipperHamC2IndexDelete)
    {
        app->calls[app->book_call_index][0] = 0;
        app->calls_used[app->book_call_index] = 0;
        cfix(app);
        cfgsave(app);
        csavetxt(app);
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

bool csplit(const char* s, char* out, uint8_t* ssid, bool* has_ssid)
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
        app->encoding_index = FlipperHamModemProfileDefault;
        app->repeat_n = 1;
        app->repeat_i = 1;
        app->tx_msg_index = 0;
        app->tx_t = 0;
        app->status_index = 0;
        app->message_index = 0;
        app->dst_call_index = 0;
        app->d_s = 0;
        app->edit_call_index = 0;
        app->book_call_index = 0;
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
    app->done_w = false;

    for(i = 0; i < app->repeat_n; i++)
    {
        app->repeat_i = i + 1;

        txstart(app);
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
