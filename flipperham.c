#include "flipperham.h"
#include "packet.h"
#include "app_state.h"
#include "rf_gen.h"
#include "ui.h"

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

static void cloadtxt(FlipperHamApp* app);

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

void cfgsave(FlipperHamApp* app)
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

void csavetxt(FlipperHamApp* app)
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

void cfgload(FlipperHamApp* app)
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

void bfix(FlipperHamApp* app)
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

void stfix(FlipperHamApp* app)
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

void cfix(FlipperHamApp* app)
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

void mfix(FlipperHamApp* app)
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

bool cval(char* s)
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
