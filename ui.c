#include "flipperham.h"
#include "aprs.h"
#include "ui.h"
#include "rf_gen.h"

static void bx0(void* context, uint32_t index);
static void sx0(void* context, uint32_t index);
static void cx0(void* context, uint32_t index);
static void fq0(void* context, uint32_t index);
static void mx0(void* context, uint32_t index);
static void px0(void* context, uint32_t index);
#include <furi_hal.h>
#include <gui/view.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static uint32_t flipperham_exit_callback(void* context);
static uint32_t flipperham_send_exit_callback(void* context);
static uint32_t flipperham_settings_exit_callback(void* context);
static uint32_t flipperham_bulletin_exit_callback(void* context);
static uint32_t flipperham_status_exit_callback(void* context);
static uint32_t flipperham_message_exit_callback(void* context);
static uint32_t flipperham_position_exit_callback(void* context);
static uint32_t flipperham_ssid_exit_callback(void* context);
static uint32_t flipperham_call_exit_callback(void* context);
static uint32_t flipperham_freq_exit_callback(void* context);
static uint32_t flipperham_freq_edit_exit_callback(void* context);
static uint32_t flipperham_pos_edit_exit_callback(void* context);
static uint32_t flipperham_ham_exit_callback(void* context);
static uint32_t flipperham_ham_tx_exit_callback(void* context);
static uint32_t bookx(void* context);
static uint32_t c2x(void* context);
static uint32_t flipperham_text_exit_callback(void* context);


static void flipperham_bulletin_callback(void* context, uint32_t index);
static void bx(void* context, InputType input_type, uint32_t index);
static void st(void* context, uint32_t index);
static void sx(void* context, InputType input_type, uint32_t index);
static void mx(void* context, InputType input_type, uint32_t index);
static void m(void* context, uint32_t index);
static void p(void* context, uint32_t index);
static void px(void* context, InputType input_type, uint32_t index);
static void cx(void* context, InputType input_type, uint32_t index);
static void cl(void* context, uint32_t index);
static void cb(void* context, uint32_t index);
static void c2(void* context, uint32_t index);
static void fq(void* context, InputType input_type, uint32_t index);
static void bsave(void* context);
static void stsave(void* context);
static void msave(void* context);
static void psave(void* context);
static void csave(void* context);
static void fsave(void* context);


static void bmenu(FlipperHamApp* app);
static void stmenu(FlipperHamApp* app);
static void mmenu(FlipperHamApp* app);
static void pmenu(FlipperHamApp* app);
static void cmenu(FlipperHamApp* app);
static void bkmenu(FlipperHamApp* app);
static void c2m(FlipperHamApp* app);
static void smenu(FlipperHamApp* app);
static void rmenu(FlipperHamApp* app);
static void hmenu(FlipperHamApp* app);
static void htmenu(FlipperHamApp* app);
static void fmenu(FlipperHamApp* app);
static void femenu(FlipperHamApp* app);
static void pemenu(FlipperHamApp* app);
static void ssidfix(FlipperHamApp* app);
static void sc(VariableItem* item);
static void bc(VariableItem* item);
static void pc(VariableItem* item);
static void dc(VariableItem* item);
static void rc(VariableItem* item);
static void hc(VariableItem* item);
static void hsc(VariableItem* item);
static void fc(VariableItem* item);
static void se(void* context, uint32_t index);
static void re(void* context, uint32_t index);
static void hre(void* context, uint32_t index);
static void hte(void* context, uint32_t index);
static void fe(void* context, uint32_t index);
static void pe(void* context, uint32_t index);
static uint32_t fstep(uint32_t a, int8_t d);
static uint32_t fminhz(void);
static uint32_t fmaxhz(void);
static bool fband(uint32_t a, uint32_t* lo, uint32_t* hi);
static uint32_t fnextband(uint32_t a);
static void fsh(char* o, uint16_t n, uint32_t a);
static bool fin(const char* s, uint32_t* out);


static void flipperham_draw_callback(Canvas* canvas, void* context);
static void stin(InputEvent* event, void* context);
static void flipperham_status_view_alloc(FlipperHamApp* app);
static void flipperham_status_view_free(FlipperHamApp* app);
static void flipperham_menu_free(FlipperHamApp* app);
static FlipperHamApp* flipperham_app_alloc(void);
static void flipperham_app_free(FlipperHamApp* app);
static void flipperham_send_hardcoded_message(FlipperHamApp* app);
static void flipperham_menu_callback(void* context, uint32_t index);
static void flipperham_send_callback(void* context, uint32_t index);
static bool cpy(FlipperHamApp* app);
static void gblink(void);
static uint32_t rt4(FlipperHamApp* app);


static void flipperham_draw_callback(Canvas* canvas, void* context) 
{
    FlipperHamApp* app = context;
    char a[16];
    uint32_t n, w, m, x;

    canvas_clear(canvas);
    canvas_set_font(canvas, FontBigNumbers);
    snprintf(a, sizeof(a), "%06lu", (unsigned long)(txf(app) / 1000UL));
    canvas_draw_str_aligned(canvas, 64, 11, AlignCenter, AlignCenter, a);
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
        x = rt4(app);
        n = furi_get_tick() - app->repeat_t0;
        m = (app->repeat_n >= 5) ? 15000 : 8000;
        m *= x;
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
                app->bk_sel = FlipperHamBookIndexBase + j;
                app->book_call_index = j;
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

static uint32_t flipperham_position_exit_callback(void* context)
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

static uint32_t flipperham_freq_exit_callback(void* context)
{
    UNUSED(context);

    return FlipperHamViewSettings;
}

static uint32_t flipperham_freq_edit_exit_callback(void* context)
{
    UNUSED(context);

    return FlipperHamViewFreq;
}

static uint32_t flipperham_pos_edit_exit_callback(void* context)
{
    UNUSED(context);

    return FlipperHamViewPosition;
}

static uint32_t flipperham_ham_exit_callback(void* context)
{
    UNUSED(context);

    return FlipperHamViewMenu;
}

static uint32_t flipperham_ham_tx_exit_callback(void* context)
{
    UNUSED(context);

    return FlipperHamViewHam;
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

static uint32_t flipperham_text_exit_callback(void* context)
{
    FlipperHamApp* app = context;

    return app->txt_v;
}


static void flipperham_menu_callback(void* context, uint32_t index)
{
    FlipperHamApp* app = context;

    if(index == FlipperHamMenuIndexSend) view_dispatcher_switch_to_view(app->view_dispatcher, FlipperHamViewSend);
    if(index == FlipperHamMenuIndexSettings) view_dispatcher_switch_to_view(app->view_dispatcher, FlipperHamViewSettings);
    if(index == FlipperHamMenuIndexCallbook) {
        bkmenu(app);
        view_dispatcher_switch_to_view(app->view_dispatcher, FlipperHamViewBook);
    }
    if(index == FlipperHamMenuIndexHam) view_dispatcher_switch_to_view(app->view_dispatcher, FlipperHamViewHam);
}

static void flipperham_send_callback(void* context, uint32_t index)
{
    FlipperHamApp* app = context;

    if(index == FlipperHamSendIndexMessage)
    {
        mmenu(app);
        view_dispatcher_switch_to_view(app->view_dispatcher, FlipperHamViewMessage);
    }

    if(index == FlipperHamSendIndexPosition)
    {
        pmenu(app);
        view_dispatcher_switch_to_view(app->view_dispatcher, FlipperHamViewPosition);
    }

    if(index == FlipperHamSendIndexBulletin)
    {
        bmenu(app);
        view_dispatcher_switch_to_view(app->view_dispatcher, FlipperHamViewBulletin);
    }

    if(index == FlipperHamSendIndexStatus)
    {
        stmenu(app);
        view_dispatcher_switch_to_view(app->view_dispatcher, FlipperHamViewStatus);
    }
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

    submenu_set_selected_item(app->bulletin_menu, FlipperHamBulletinIndexAdd);
    if(app->b_sel >= FlipperHamBulletinIndexBase)
    {
        i = app->b_sel - FlipperHamBulletinIndexBase;
        if(i < TXT_N) if(app->bulletin_used[i]) if(app->bulletin[i][0])
            submenu_set_selected_item(app->bulletin_menu, app->b_sel);
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

    submenu_set_selected_item(app->status_menu, FlipperHamStatusIndexAdd);
    if(app->st_sel >= FlipperHamStatusIndexBase)
    {
        i = app->st_sel - FlipperHamStatusIndexBase;
        if(i < TXT_N) if(app->status_used[i]) if(app->status[i][0])
            submenu_set_selected_item(app->status_menu, app->st_sel);
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

        submenu_set_selected_item(app->call_menu, FlipperHamCallIndexAdd);
        if(app->c_sel >= FlipperHamCallIndexBase)
        {
            i = app->c_sel - FlipperHamCallIndexBase;
            if(i < CALL_N) if(app->calls_used[i]) if(app->calls[i][0])
                submenu_set_selected_item(app->call_menu, app->c_sel);
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

    submenu_set_selected_item(app->book_menu, FlipperHamBookIndexAdd);
    if(app->bk_sel >= FlipperHamBookIndexBase)
    {
        i = app->bk_sel - FlipperHamBookIndexBase;
        if(i < CALL_N) if(app->calls_used[i]) if(app->calls[i][0])
            submenu_set_selected_item(app->book_menu, app->bk_sel);
    }
}

static void c2m(FlipperHamApp* app)
{
    submenu_reset(app->c2_menu);
    submenu_set_header(app->c2_menu, app->c2_h);
    submenu_add_item( app->c2_menu, "Edit", FlipperHamC2IndexEdit, c2, app);
    submenu_add_item( app->c2_menu, "Delete", FlipperHamC2IndexDelete, c2, app);
    submenu_add_item( app->c2_menu, "Copy", FlipperHamC2IndexCopy, c2, app);
    submenu_set_selected_item(app->c2_menu, app->c2_sel);
}

static uint32_t fstep(uint32_t a, int8_t d)
{
    uint32_t b;
    uint32_t c;
    uint32_t e;

    b = a;
    if(!fband(b, &c, &e))
    {
        c = fminhz();
        e = fmaxhz();
        if(b < c) b = c;
        if(b > e) b = e;
        if(!fband(b, &c, &e)) return b;
    }

    if(d > 0)
    {
        if(b >= e) return e;
        b += 2500UL;
        if(b > e) b = e;
    }
    else
    {
        if(b <= c) return c;
        b -= 2500UL;
        if(b < c) b = c;
    }

    return b;
}

static uint32_t fminhz(void)
{
    uint32_t a;

    for(a = 0; a < 1000000000UL; a += 2500UL)
        if(furi_hal_subghz_is_frequency_valid(a)) return a;

    return CARRIER_HZ;
}

static uint32_t fmaxhz(void)
{
    uint32_t a;

    for(a = 1000000000UL - 1; a > 2500UL; a -= 2500UL)
        if(furi_hal_subghz_is_frequency_valid(a)) return a;

    return CARRIER_HZ;
}

static bool fband(uint32_t a, uint32_t* lo, uint32_t* hi)
{
    uint32_t b;

    if(!furi_hal_subghz_is_frequency_valid(a)) return false;

    b = a;
    while(b >= 2500UL)
    {
        if(!furi_hal_subghz_is_frequency_valid(b - 2500UL)) break;
        b -= 2500UL;
    }
    *lo = b;

    b = a;
    while(b <= 1000000000UL - 2500UL)
    {
        if(!furi_hal_subghz_is_frequency_valid(b + 2500UL)) break;
        b += 2500UL;
    }
    *hi = b;

    return true;
}

static uint32_t fnextband(uint32_t a)
{
    uint32_t c;
    uint32_t e;
    uint32_t b;
    uint32_t mx;

    mx = fmaxhz();
    if(!fband(a, &c, &e)) return fminhz();

    b = e;
    while(b <= mx - 2500UL)
    {
        b += 2500UL;
        if(furi_hal_subghz_is_frequency_valid(b)) return b;
    }

    return fminhz();
}

static void fsh(char* o, uint16_t n, uint32_t a)
{
    if(!o) return;
    if(!n) return;

    snprintf(o, n, "%06lu.%1lu", (unsigned long)(a / 1000UL), (unsigned long)((a % 1000UL) / 100UL));
}

static void fsh2(char* o, uint16_t n, uint32_t a)
{
    if(!o) return;
      if(!n) return;

    snprintf(o, n, "%06lu", (unsigned long)(a / 1000UL));
}

static bool fin(const char* s, uint32_t* out)
{
    char* e;
    uint32_t a;
    uint32_t b;

    if(!s) return false;
    if(!out) return false;
    if(!s[0]) return false;

    a = strtoul(s, &e, 10);
    if(e == s) return false;
    if(*e) return false;

    b = a;
    if(furi_hal_subghz_is_frequency_valid(b)) {
        *out = b;
        return true;
    }

    if(a <= 1000000UL)
    {
        b = a * 1000UL;
        if(furi_hal_subghz_is_frequency_valid(b)) {
            *out = b;
            return true;
        }
    }

    if(a <= 1000UL)
    {
        b = a * 1000000UL;
        if(furi_hal_subghz_is_frequency_valid(b)) {
            *out = b;
            return true;
        }
    }

    return false;
}

static void fmenu(FlipperHamApp* app)
{
    uint8_t i;

    submenu_reset(app->freq_menu);

    if(app->freq_n < FREQ_N)
        submenu_add_item(app->freq_menu, "Add new...", FlipperHamFreqIndexAdd, fq0, app);

    for(i = 0; i < FREQ_N; i++)
    {
        if(!app->freq_used[i]) continue;
        if(app->tx_freq_index == i) snprintf(app->freq_s[i], sizeof(app->freq_s[i]), "%06lu.%1lu *", (unsigned long)(app->freq[i] / 1000UL), (unsigned long)((app->freq[i] % 1000UL) / 100UL));
        else fsh(app->freq_s[i], sizeof(app->freq_s[i]), app->freq[i]);
        submenu_add_item(app->freq_menu, app->freq_s[i], FlipperHamFreqIndexBase + i, fq0, app);
    }

    if(app->freq_n < FREQ_N) submenu_set_selected_item(app->freq_menu, FlipperHamFreqIndexAdd);
    else submenu_set_selected_item(app->freq_menu, FlipperHamFreqIndexBase + app->tx_freq_index);

    if(app->f_sel >= FlipperHamFreqIndexBase)
    {
        i = app->f_sel - FlipperHamFreqIndexBase;
        if(i < FREQ_N) if(app->freq_used[i])
            submenu_set_selected_item(app->freq_menu, app->f_sel);
    }
}

static void femenu(FlipperHamApp* app)
{
    VariableItem* it;

    variable_item_list_reset(app->freq_edit_menu);

    it = variable_item_list_add(app->freq_edit_menu, "Frequency", 201, fc, app);
    variable_item_set_current_value_index(it, 100);
    fsh2(app->f_edit, sizeof(app->f_edit), app->freq_edit_hz);
    variable_item_set_current_value_text(it, app->f_edit);

    it = variable_item_list_add(app->freq_edit_menu, "Enter frequency", 1, NULL, NULL);
    variable_item_set_current_value_index(it, 0);
    variable_item_set_current_value_text(it, app->f_bad ? "bad" : "");

    variable_item_list_add(app->freq_edit_menu, "Save", 1, NULL, NULL);
    variable_item_list_add(app->freq_edit_menu, "Select for TX", 1, NULL, NULL);

    if(app->freq_n > 1) if(app->freq_index < FREQ_N) if(app->freq_used[app->freq_index])
        variable_item_list_add(app->freq_edit_menu, "Delete", 1, NULL, NULL);
    variable_item_list_set_selected_item(app->freq_edit_menu, 0);
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

static void hmenu(FlipperHamApp* app)
{
    VariableItem* it;

    variable_item_list_reset(app->ham_menu);
    if(!app->ham_n) return;
    if(app->ham_index >= app->ham_n) app->ham_index = 0;

    it = variable_item_list_add(app->ham_menu, "Callsign", app->ham_n, hc, app);
    variable_item_set_current_value_index(it, app->ham_index);
    variable_item_set_current_value_text(it, app->ham_calls[app->ham_index]);

    variable_item_list_add(app->ham_menu, "73 de YO3GND", 1, NULL, NULL);
    variable_item_list_set_selected_item(app->ham_menu, app->h_sel);
}

static void htmenu(FlipperHamApp* app)
{
    VariableItem* it;
    char a[4];

    variable_item_list_reset(app->ham_tx_menu);
    if(!app->ham_n) return;
    if(app->ham_tx_index >= HAM_N) app->ham_tx_index = 0;

    it = variable_item_list_add(app->ham_tx_menu, "SSID", 16, hsc, app);
    variable_item_set_current_value_index(it, app->ham_ssid[app->ham_tx_index]);
    snprintf(a, sizeof(a), "%u", app->ham_ssid[app->ham_tx_index]);
    variable_item_set_current_value_text(it, a);

    variable_item_list_add(app->ham_tx_menu, "Select for TX", 0, NULL, NULL);
    variable_item_list_set_selected_item(app->ham_tx_menu, app->ht_sel);
}


static void rmenu(FlipperHamApp* app)
{
    VariableItem* it;
    char a[16];

    variable_item_list_reset(app->settings_menu);

    it = variable_item_list_add(app->settings_menu, "Frequency", 1, NULL, NULL);
    fsh(a, sizeof(a), txf(app));
    variable_item_set_current_value_index(it, 0);
    variable_item_set_current_value_text(it, a);

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

    submenu_set_selected_item(app->message_menu, FlipperHamMessageIndexAdd);
    if(app->m_sel >= FlipperHamMessageIndexBase)
    {
        i = app->m_sel - FlipperHamMessageIndexBase;
        if(i < TXT_N) if(app->message_used[i]) if(app->message[i][0])
            submenu_set_selected_item(app->message_menu, app->m_sel);
    }
}

static void pmenu(FlipperHamApp* app)
{
    uint8_t i;

    submenu_reset(app->position_menu);
    submenu_add_item( app->position_menu, "Add new...", FlipperHamPositionIndexAdd, p, app);

    for(i = 0; i < TXT_N; i++)
    {
        if(!app->pos_used[i]) continue;
        if(!app->pos_name[i][0]) continue;

        submenu_add_item(app->position_menu, app->pos_name[i], FlipperHamPositionIndexBase + i, px0, app);
    }

    submenu_set_selected_item(app->position_menu, FlipperHamPositionIndexAdd);
    if(app->p_sel >= FlipperHamPositionIndexBase)
    {
        i = app->p_sel - FlipperHamPositionIndexBase;
        if(i < TXT_N) if(app->pos_used[i]) if(app->pos_name[i][0])
            submenu_set_selected_item(app->position_menu, app->p_sel);
    }
}

static void pemenu(FlipperHamApp* app)
{
    VariableItem* it;

    variable_item_list_reset(app->pos_edit_menu);

    it = variable_item_list_add(app->pos_edit_menu, "Name", 1, NULL, NULL);
    variable_item_set_current_value_index(it, 0);
    variable_item_set_current_value_text(it, app->p_name_edit);

    it = variable_item_list_add(app->pos_edit_menu, "Latitude", 1, NULL, NULL);
    variable_item_set_current_value_index(it, 0);
    variable_item_set_current_value_text(it, app->p_lat_edit);

    it = variable_item_list_add(app->pos_edit_menu, "Longitude", 1, NULL, NULL);
    variable_item_set_current_value_index(it, 0);
    variable_item_set_current_value_text(it, app->p_lon_edit);

    if(app->pos_n > 1) if(app->pos_index < TXT_N) if(app->pos_used[app->pos_index])
        variable_item_list_add(app->pos_edit_menu, "Delete", 1, NULL, NULL);

    variable_item_list_set_selected_item(app->pos_edit_menu, 0);
}

static void ssidfix(FlipperHamApp* app)
{
    if(app->d_s > 15) app->d_s = 0;
    smenu(app);
}


static void sc(VariableItem* item)
{
    FlipperHamApp* app = variable_item_get_context(item);
    char a[4];

    app->d_s = variable_item_get_current_value_index(item);
    snprintf(a, sizeof(a), "%u", app->d_s);
    variable_item_set_current_value_text(item, a);
}

static void hc(VariableItem* item)
{
    FlipperHamApp* app = variable_item_get_context(item);

    app->ham_index = variable_item_get_current_value_index(item);
    if(app->ham_n) if(app->ham_index >= app->ham_n) app->ham_index = 0;
    variable_item_set_current_value_text(item, app->ham_calls[app->ham_index]);
}

static void hsc(VariableItem* item)
{
    FlipperHamApp* app = variable_item_get_context(item);
    char a[4];

    app->ham_ssid[app->ham_tx_index] = variable_item_get_current_value_index(item);
    snprintf(a, sizeof(a), "%u", app->ham_ssid[app->ham_tx_index]);
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

    app->repeat_n = variable_item_get_current_value_index(item) + 1;
    snprintf(a, sizeof(a), "%u", app->repeat_n);
    variable_item_set_current_value_text(item, a);
    cfgsave(app);
}

static void fc(VariableItem* item)
{
    FlipperHamApp* app = variable_item_get_context(item);
    uint8_t a;

    app->f_bad = false;
    a = variable_item_get_current_value_index(item);

    while(a > 100)
    {
        app->freq_edit_hz = fstep(app->freq_edit_hz, 1);
        a--;
    }

    while(a < 100)
    {
        app->freq_edit_hz = fstep(app->freq_edit_hz, -1);
        a++;
    }

    fsh2(app->f_edit, sizeof(app->f_edit), app->freq_edit_hz);
    variable_item_set_current_value_text(item, app->f_edit);
    variable_item_set_current_value_index(item, 100);
}

static void se(void* context, uint32_t index)
{
    FlipperHamApp* app = context;

    UNUSED(index);
    app->c_sel = FlipperHamCallIndexBase + app->dst_call_index;
    app->m_sel = FlipperHamMessageIndexBase + app->tx_msg_index;

    app->go_v = FlipperHamViewMessage;
    app->send_requested = true;
    view_dispatcher_stop(app->view_dispatcher);
}

static void re(void* context, uint32_t index)
{
    FlipperHamApp* app = context;

    if(index != FlipperHamSettingsIndexFreq) return;

    fmenu(app);
    view_dispatcher_switch_to_view(app->view_dispatcher, FlipperHamViewFreq);
}

static void hre(void* context, uint32_t index)
{
    FlipperHamApp* app = context;

    app->h_sel = index;
    if(index == 0)
    {
        app->ham_tx_index = app->ham_index;
        htmenu(app);
        view_dispatcher_switch_to_view(app->view_dispatcher, FlipperHamViewHamTx);
        return;
    }
}

static void hte(void* context, uint32_t index)
{
    FlipperHamApp* app = context;

    app->ht_sel = index;
    if(index == 1)
    {
        app->ham_index = app->ham_tx_index;
        hmenu(app);
        view_dispatcher_switch_to_view(app->view_dispatcher, FlipperHamViewHam);
        return;
    }
}

static void fe(void* context, uint32_t index)
{
    FlipperHamApp* app = context;
    bool a;

    a = false;
    if(app->freq_n > 1) if(app->freq_index < FREQ_N) if(app->freq_used[app->freq_index]) a = true;

    if(index == 0)
    {
        app->f_sel = FlipperHamFreqIndexBase + app->freq_index;
        app->f_bad = false;
        app->freq_edit_hz = fnextband(app->freq_edit_hz);
        femenu(app);
        view_dispatcher_switch_to_view(app->view_dispatcher, FlipperHamViewFreqEdit);
        return;
    }

    if(index == 1)
    {
        app->f_sel = FlipperHamFreqIndexBase + app->freq_index;
        app->f_edit[0] = 0;
        app->txt = 5;
        app->txt_v = FlipperHamViewFreqEdit;
        text_input_reset(app->text_input);
        text_input_set_header_text(app->text_input, "Frequency");
        text_input_set_result_callback(app->text_input, fsave, app, app->f_edit, sizeof(app->f_edit), false);
        view_dispatcher_switch_to_view(app->view_dispatcher, FlipperHamViewTextInput);
        return;
    }

    if(index == 2)
    {
        if(app->f_bad) return;
        if(app->freq_index >= FREQ_N) return;
        if(!furi_hal_subghz_is_frequency_valid(app->freq_edit_hz)) return;

        app->f_sel = FlipperHamFreqIndexBase + app->freq_index;
        app->freq[app->freq_index] = app->freq_edit_hz;
        app->freq_used[app->freq_index] = 1;
        ffix(app);
        cfgsave(app);
        fmenu(app);
        submenu_set_selected_item(app->freq_menu, FlipperHamFreqIndexBase + app->freq_index);
        rmenu(app);
        view_dispatcher_switch_to_view(app->view_dispatcher, FlipperHamViewFreq);
        return;
    }

    if(index == 3)
    {
        if(app->f_bad) return;
        if(app->freq_index >= FREQ_N) return;
        if(!furi_hal_subghz_is_frequency_valid(app->freq_edit_hz)) return;

        app->f_sel = FlipperHamFreqIndexBase + app->freq_index;
        app->freq[app->freq_index] = app->freq_edit_hz;
        app->freq_used[app->freq_index] = 1;
        app->tx_freq_index = app->freq_index;
        ffix(app);
        cfgsave(app);
        fmenu(app);
        submenu_set_selected_item(app->freq_menu, FlipperHamFreqIndexBase + app->freq_index);
        rmenu(app);
        view_dispatcher_switch_to_view(app->view_dispatcher, FlipperHamViewFreq);
        return;
    }

    if(a && index == 4)
    {
        app->f_sel = FlipperHamFreqIndexAdd;
        app->freq[app->freq_index] = 0;
        app->freq_used[app->freq_index] = 0;
        ffix(app);
        cfgsave(app);
        fmenu(app);
        rmenu(app);
        view_dispatcher_switch_to_view(app->view_dispatcher, FlipperHamViewFreq);
        return;
    }
}

static void pe(void* context, uint32_t index)
{
    FlipperHamApp* app = context;
    bool a;

    a = false;
    if(app->pos_n > 1) if(app->pos_index < TXT_N) if(app->pos_used[app->pos_index]) a = true;

    if(index == 0)
    {
        app->txt = 6;
        app->txt_v = FlipperHamViewPosEdit;
        text_input_reset(app->text_input);
        text_input_set_header_text(app->text_input, "Name");
        text_input_set_result_callback(app->text_input, psave, app, app->p_name_edit, sizeof(app->p_name_edit), false);
        view_dispatcher_switch_to_view(app->view_dispatcher, FlipperHamViewTextInput);
        return;
    }

    if(index == 1)
    {
        app->txt = 7;
        app->txt_v = FlipperHamViewPosEdit;
        text_input_reset(app->text_input);
        text_input_set_header_text(app->text_input, "Latitude");
        text_input_set_result_callback(app->text_input, psave, app, app->p_lat_edit, sizeof(app->p_lat_edit), false);
        view_dispatcher_switch_to_view(app->view_dispatcher, FlipperHamViewTextInput);
        return;
    }

    if(index == 2)
    {
        app->txt = 8;
        app->txt_v = FlipperHamViewPosEdit;
        text_input_reset(app->text_input);
        text_input_set_header_text(app->text_input, "Longitude");
        text_input_set_result_callback(app->text_input, psave, app, app->p_lon_edit, sizeof(app->p_lon_edit), false);
        view_dispatcher_switch_to_view(app->view_dispatcher, FlipperHamViewTextInput);
        return;
    }

    if(a && index == 3)
    {
        uint8_t i;

        app->pos_name[app->pos_index][0] = 0;
        app->pos_lat[app->pos_index][0] = 0;
        app->pos_lon[app->pos_index][0] = 0;
        app->pos_used[app->pos_index] = 0;
        app->p_sel = FlipperHamPositionIndexAdd;

        for(i = app->pos_index; i < TXT_N; i++)
            if(app->pos_used[i]) if(app->pos_name[i][0]) {
                app->p_sel = FlipperHamPositionIndexBase + i;
                break;
            }
        if(app->p_sel == FlipperHamPositionIndexAdd)
            for(i = app->pos_index; i > 0; i--)
                if(app->pos_used[i - 1]) if(app->pos_name[i - 1][0]) {
                    app->p_sel = FlipperHamPositionIndexBase + i - 1;
                    break;
                }

        pfix(app);
        cfgsave(app);
        pmenu(app);
        view_dispatcher_switch_to_view(app->view_dispatcher, FlipperHamViewPosition);
    }
}

static void fq(void* context, InputType input_type, uint32_t index)
{
    FlipperHamApp* app = context;
    uint8_t a;

    if(index == FlipperHamFreqIndexAdd)
    {
        app->f_sel = FlipperHamFreqIndexAdd;
        app->freq_index = 0xff;
        app->f_bad = false;

        for(a = 0; a < FREQ_N; a++)
            if(!app->freq_used[a]) {
                app->freq_index = a;
                break;
            }

        if(app->freq_index == 0xff) return;

        app->freq_edit_hz = txf(app);
        femenu(app);
        view_dispatcher_switch_to_view(app->view_dispatcher, FlipperHamViewFreqEdit);
        return;
    }

    a = index - FlipperHamFreqIndexBase;
    if(a >= FREQ_N) return;
    if(!app->freq_used[a]) return;

    app->f_sel = index;

    if(input_type == InputTypeLong)
    {
        app->tx_freq_index = a;
        app->f_bad = false;
        cfgsave(app);
        fmenu(app);
        rmenu(app);
        submenu_set_selected_item(app->freq_menu, FlipperHamFreqIndexBase + a);
        return;
    }

    if(input_type != InputTypeShort) return;

    app->freq_index = a;
    app->freq_edit_hz = app->freq[a];
    app->f_bad = false;
    femenu(app);
    view_dispatcher_switch_to_view(app->view_dispatcher, FlipperHamViewFreqEdit);
}

static void fsave(void* context)
{
    FlipperHamApp* app = context;
    uint32_t a;

    if(fin(app->f_edit, &a)) {
        app->freq_edit_hz = a;
        app->f_bad = false;
    }
    else app->f_bad = true;

    femenu(app);
    view_dispatcher_switch_to_view(app->view_dispatcher, FlipperHamViewFreqEdit);
}


static void m(void* context, uint32_t index)
{
    FlipperHamApp* app = context;
    uint8_t i;

    if(index == FlipperHamMessageIndexAdd)
    {
        app->m_sel = FlipperHamMessageIndexAdd;
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

        app->m_sel = index;
        app->message_index = i;
        snprintf(app->m_edit, sizeof(app->m_edit), "%s", app->message[i]);
    }

    app->txt = 3;
    app->txt_v = FlipperHamViewMessage;

    text_input_reset(app->text_input);
    text_input_set_header_text(app->text_input, "Message");
    text_input_set_result_callback(app->text_input, msave, app, app->m_edit, sizeof(app->m_edit), false);

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
        app->m_sel = index;
        if(app->dst_call_index < CALL_N) if(app->calls_used[app->dst_call_index]) if(app->calls[app->dst_call_index][0])
            app->c_sel = FlipperHamCallIndexBase + app->dst_call_index;
        cmenu(app);

        view_dispatcher_switch_to_view(app->view_dispatcher, FlipperHamViewCall);
        return;
    }

    if(input_type != InputTypeLong) return;

    app->m_sel = index;
    app->message_index = i;
    snprintf(app->m_edit, sizeof(app->m_edit), "%s", app->message[i]);
    app->txt = 3;
    app->txt_v = FlipperHamViewMessage;
    text_input_reset(app->text_input);
    text_input_set_header_text(app->text_input, "Message");
    text_input_set_result_callback(app->text_input, msave, app, app->m_edit, sizeof(app->m_edit), false);
    view_dispatcher_switch_to_view(app->view_dispatcher, FlipperHamViewTextInput);
}

static void msave(void* context)
{
    FlipperHamApp* app = context;
    uint8_t i, j;

    i = app->message_index;
    if(i >= TXT_N) return;

    if(!app->m_edit[0])
    {
        app->message[i][0] = 0;
        app->message_used[i] = 0;
        app->m_sel = FlipperHamMessageIndexAdd;
        for(j = i; j < TXT_N; j++)
            if(app->message_used[j]) if(app->message[j][0]) {
                app->m_sel = FlipperHamMessageIndexBase + j;
                break;
            }
        if(app->m_sel == FlipperHamMessageIndexAdd)
            for(j = i; j > 0; j--)
                if(app->message_used[j - 1]) if(app->message[j - 1][0]) {
                    app->m_sel = FlipperHamMessageIndexBase + j - 1;
                    break;
                }
    }
    else
    {
        snprintf(app->message[i], sizeof(app->message[i]), "%s", app->m_edit);
        app->message_used[i] = 1;
        app->m_sel = FlipperHamMessageIndexBase + i;
    }

    mfix(app);
    cfgsave(app);
    mmenu(app);

    view_dispatcher_switch_to_view(app->view_dispatcher, FlipperHamViewMessage);
}

static void p(void* context, uint32_t index)
{
    FlipperHamApp* app = context;
    uint8_t i;

    if(index != FlipperHamPositionIndexAdd) return;

    app->p_sel = FlipperHamPositionIndexAdd;
    app->pos_index = 0xff;

    for(i = 0; i < TXT_N; i++)
        if(!app->pos_used[i] || !app->pos_name[i][0]) {
            app->pos_index = i;
            break;
        }

    if(app->pos_index == 0xff) return;

    app->pos_name[app->pos_index][0] = 0;
    app->pos_lat[app->pos_index][0] = 0;
    app->pos_lon[app->pos_index][0] = 0;
    app->pos_used[app->pos_index] = 0;

    snprintf(app->p_name_edit, sizeof(app->p_name_edit), "Position");
    snprintf(app->p_lat_edit, sizeof(app->p_lat_edit), "0.00");
    snprintf(app->p_lon_edit, sizeof(app->p_lon_edit), "0.00");
    pemenu(app);
    view_dispatcher_switch_to_view(app->view_dispatcher, FlipperHamViewPosEdit);
}

static void px(void* context, InputType input_type, uint32_t index)
{
    FlipperHamApp* app = context;
    uint8_t i;

    i = index - FlipperHamPositionIndexBase;
    if(i >= TXT_N) return;

    if(input_type == InputTypeShort)
    {
        app->p_sel = index;
        app->tx_t = 3;
        app->tx_msg_index = i;
        app->go_v = FlipperHamViewPosition;
        app->send_requested = true;
        view_dispatcher_stop(app->view_dispatcher);
        return;
    }

    if(input_type != InputTypeLong) return;

    app->p_sel = index;
    app->pos_index = i;
    snprintf(app->p_name_edit, sizeof(app->p_name_edit), "%s", app->pos_name[i]);
    snprintf(app->p_lat_edit, sizeof(app->p_lat_edit), "%s", app->pos_lat[i]);
    snprintf(app->p_lon_edit, sizeof(app->p_lon_edit), "%s", app->pos_lon[i]);
    pemenu(app);
    view_dispatcher_switch_to_view(app->view_dispatcher, FlipperHamViewPosEdit);
}

static void psave(void* context)
{
    FlipperHamApp* app = context;
    uint8_t i, j;
    char a[POS_LEN];

    i = app->pos_index;
    if(i >= TXT_N) return;

    if(app->txt == 6) snprintf(app->pos_name[i], sizeof(app->pos_name[i]), "%s", app->p_name_edit);
    if(app->txt == 7)
    {
        if(aprs_ll_clamp(a, sizeof(a), app->p_lat_edit, 0)) snprintf(app->p_lat_edit, sizeof(app->p_lat_edit), "%s", a);
        else if(app->pos_lat[i][0]) snprintf(app->p_lat_edit, sizeof(app->p_lat_edit), "%s", app->pos_lat[i]);
        else snprintf(app->p_lat_edit, sizeof(app->p_lat_edit), "0.00000");
        snprintf(app->pos_lat[i], sizeof(app->pos_lat[i]), "%s", app->p_lat_edit);
    }
    if(app->txt == 8)
    {
        if(aprs_ll_clamp(a, sizeof(a), app->p_lon_edit, 1)) snprintf(app->p_lon_edit, sizeof(app->p_lon_edit), "%s", a);
        else if(app->pos_lon[i][0]) snprintf(app->p_lon_edit, sizeof(app->p_lon_edit), "%s", app->pos_lon[i]);
        else snprintf(app->p_lon_edit, sizeof(app->p_lon_edit), "0.00000");
        snprintf(app->pos_lon[i], sizeof(app->pos_lon[i]), "%s", app->p_lon_edit);
    }

    if(!app->pos_name[i][0])
    {
        app->pos_used[i] = 0;
        app->p_sel = FlipperHamPositionIndexAdd;
        for(j = i; j < TXT_N; j++)
            if(app->pos_used[j]) if(app->pos_name[j][0]) {
                app->p_sel = FlipperHamPositionIndexBase + j;
                break;
            }
        if(app->p_sel == FlipperHamPositionIndexAdd)
            for(j = i; j > 0; j--)
                if(app->pos_used[j - 1]) if(app->pos_name[j - 1][0]) {
                    app->p_sel = FlipperHamPositionIndexBase + j - 1;
                    break;
                }
    }
    else
    {
        app->pos_used[i] = 1;
        app->p_sel = FlipperHamPositionIndexBase + i;
    }

    pfix(app);
    cfgsave(app);
    pmenu(app);
    pemenu(app);
    view_dispatcher_switch_to_view(app->view_dispatcher, FlipperHamViewPosEdit);
}


static void flipperham_bulletin_callback(void* context, uint32_t index)
{
    FlipperHamApp* app = context;
    uint8_t i;

    if(index != FlipperHamBulletinIndexAdd) return;

    app->b_sel = FlipperHamBulletinIndexAdd;
    app->bulletin_index = 0xff;

    for(i = 0; i < TXT_N; i++)
        if(!app->bulletin_used[i]) {
            app->bulletin_index = i;
            break;
        }

    if(app->bulletin_index == 0xff) return;

    app->b_edit[0] = 0;
    app->txt = 0;
    app->txt_v = FlipperHamViewBulletin;

    text_input_reset(app->text_input);
    text_input_set_header_text(app->text_input, "Bulletin");
    text_input_set_result_callback(app->text_input, bsave, app, app->b_edit, sizeof(app->b_edit), false);

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
        app->b_sel = index;
        app->tx_t = 0;
        app->tx_msg_index = i;
        app->go_v = FlipperHamViewBulletin;
        app->send_requested = true;
        view_dispatcher_stop(app->view_dispatcher);
        return;
    }

    if(input_type != InputTypeLong) return;

    app->b_sel = index;
    app->bulletin_index = i;
    snprintf(app->b_edit, sizeof(app->b_edit), "%s", app->bulletin[i]);
    app->txt = 0;
    app->txt_v = FlipperHamViewBulletin;

    text_input_reset(app->text_input);
    text_input_set_header_text(app->text_input, "Bulletin");
    text_input_set_result_callback(app->text_input, bsave, app, app->b_edit, sizeof(app->b_edit), false);

    view_dispatcher_switch_to_view(app->view_dispatcher, FlipperHamViewTextInput);
}

static void bsave(void* context)
{
    FlipperHamApp* app = context;
    uint8_t i, j;

    i = app->bulletin_index;
    if(i >= TXT_N) return;

    if(!app->b_edit[0])
    {
        app->bulletin[i][0] = 0;
        app->bulletin_used[i] = 0;
        app->b_sel = FlipperHamBulletinIndexAdd;
        for(j = i; j < TXT_N; j++)
            if(app->bulletin_used[j]) if(app->bulletin[j][0]) {
                app->b_sel = FlipperHamBulletinIndexBase + j;
                break;
            }
        if(app->b_sel == FlipperHamBulletinIndexAdd)
            for(j = i; j > 0; j--)
                if(app->bulletin_used[j - 1]) if(app->bulletin[j - 1][0]) {
                    app->b_sel = FlipperHamBulletinIndexBase + j - 1;
                    break;
                }
    }
    else
    {
        snprintf(app->bulletin[i], sizeof(app->bulletin[i]), "%s", app->b_edit);
        app->bulletin_used[i] = 1;
        app->b_sel = FlipperHamBulletinIndexBase + i;
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

    app->st_sel = FlipperHamStatusIndexAdd;
    app->status_index = 0xff;

    for(i = 0; i < TXT_N; i++)
        if(!app->status_used[i]) {
            app->status_index = i;
            break;
        }

    if(app->status_index == 0xff) return;

    app->st_edit[0] = 0;
    app->txt = 1;
    app->txt_v = FlipperHamViewStatus;

    text_input_reset(app->text_input);
    text_input_set_header_text(app->text_input, "Status");
    text_input_set_result_callback(app->text_input, stsave, app, app->st_edit, sizeof(app->st_edit), false);

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
        app->st_sel = index;
        app->tx_t = 1;
        app->tx_msg_index = i;
        app->go_v = FlipperHamViewStatus;
        app->send_requested = true;
        view_dispatcher_stop(app->view_dispatcher);
        return;
    }

    if(input_type != InputTypeLong) return;

    app->st_sel = index;
    app->status_index = i;
    snprintf(app->st_edit, sizeof(app->st_edit), "%s", app->status[i]);
    app->txt = 1;
    app->txt_v = FlipperHamViewStatus;

    text_input_reset(app->text_input);
    text_input_set_header_text(app->text_input, "Status");
    text_input_set_result_callback(app->text_input, stsave, app, app->st_edit, sizeof(app->st_edit), false);

    view_dispatcher_switch_to_view(app->view_dispatcher, FlipperHamViewTextInput);
}

static void stsave(void* context)
{
    FlipperHamApp* app = context;
    uint8_t i, j;

    i = app->status_index;
    if(i >= TXT_N) return;

    if(!app->st_edit[0])
    {
        app->status[i][0] = 0;
        app->status_used[i] = 0;
        app->st_sel = FlipperHamStatusIndexAdd;
        for(j = i; j < TXT_N; j++)
            if(app->status_used[j]) if(app->status[j][0]) {
                app->st_sel = FlipperHamStatusIndexBase + j;
                break;
            }
        if(app->st_sel == FlipperHamStatusIndexAdd)
            for(j = i; j > 0; j--)
                if(app->status_used[j - 1]) if(app->status[j - 1][0]) {
                    app->st_sel = FlipperHamStatusIndexBase + j - 1;
                    break;
                }
    }
    else
    {
        snprintf(app->status[i], sizeof(app->status[i]), "%s", app->st_edit);
        app->status_used[i] = 1;
        app->st_sel = FlipperHamStatusIndexBase + i;
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
        app->c_sel = FlipperHamCallIndexAdd;
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

        app->c_sel = index;
        app->edit_call_index = i;
        snprintf(app->c_edit, sizeof(app->c_edit), "%s", app->calls[i]);
    }

    app->txt = 2;
    app->txt_v = FlipperHamViewCall;

    text_input_reset(app->text_input);
    text_input_set_header_text(app->text_input, "Callsign");
    text_input_set_result_callback(app->text_input, csave, app, app->c_edit, sizeof(app->c_edit), false);

    view_dispatcher_switch_to_view(app->view_dispatcher, FlipperHamViewTextInput);
}

static void cb(void* context, uint32_t index)
{
    FlipperHamApp* app = context;
    uint8_t i;

    if(index == FlipperHamBookIndexAdd)
    {
        app->bk_sel = FlipperHamBookIndexAdd;
        app->edit_call_index = 0xff;

        for(i = 0; i < CALL_N; i++)
            if(!app->calls_used[i] || !app->calls[i][0]) {
                app->edit_call_index = i;
                break;
            }

        if(app->edit_call_index == 0xff) return;

        app->c_edit[0] = 0;
        app->txt = 4;
        app->txt_v = FlipperHamViewBook;
        text_input_reset(app->text_input);
        text_input_set_header_text(app->text_input, "Callsign");
        text_input_set_result_callback(app->text_input, csave, app, app->c_edit, sizeof(app->c_edit), false);
        view_dispatcher_switch_to_view(app->view_dispatcher, FlipperHamViewTextInput);
        return;
    }

    index -= FlipperHamBookIndexBase;
    if(index >= CALL_N) return;

    app->bk_sel = FlipperHamBookIndexBase + index;
    app->book_call_index = index;
    app->c2_sel = FlipperHamC2IndexEdit;
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
        app->c_sel = index;
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
                    ssidfix(app);
                    view_dispatcher_switch_to_view(app->view_dispatcher, FlipperHamViewSsid);
                }
            }

            return;
        }
    }

    if(input_type != InputTypeLong) return;

    app->c_sel = index;
    app->edit_call_index = i;
    snprintf(app->c_edit, sizeof(app->c_edit), "%s", app->calls[i]);
    app->txt = 2;
    app->txt_v = FlipperHamViewCall;

    text_input_reset(app->text_input);
    text_input_set_header_text(app->text_input, "Callsign");
    text_input_set_result_callback(app->text_input, csave, app, app->c_edit, sizeof(app->c_edit), false);

    view_dispatcher_switch_to_view(app->view_dispatcher, FlipperHamViewTextInput);
}

static void csave(void* context)
{
    FlipperHamApp* app = context;
    uint8_t i, j;

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
            if(app->txt == 4) app->bk_sel = FlipperHamBookIndexBase + i;
            else app->c_sel = FlipperHamCallIndexBase + i;
            if(app->txt == 4) view_dispatcher_switch_to_view(app->view_dispatcher, FlipperHamViewBook);
            else view_dispatcher_switch_to_view(app->view_dispatcher, FlipperHamViewCall);
            return;
        }

        snprintf(app->calls[i], sizeof(app->calls[i]), "%s", app->c_edit);
        app->calls_used[i] = 1;
        if(app->txt == 4) app->bk_sel = FlipperHamBookIndexBase + i;
        else app->c_sel = FlipperHamCallIndexBase + i;
    }

    if(!app->c_edit[0])
    {
        if(app->txt == 4)
        {
            app->bk_sel = FlipperHamBookIndexAdd;
            for(j = i; j < CALL_N; j++)
                if(app->calls_used[j]) if(app->calls[j][0]) {
                    app->bk_sel = FlipperHamBookIndexBase + j;
                    break;
                }
            if(app->bk_sel == FlipperHamBookIndexAdd)
                for(j = i; j > 0; j--)
                    if(app->calls_used[j - 1]) if(app->calls[j - 1][0]) {
                        app->bk_sel = FlipperHamBookIndexBase + j - 1;
                        break;
                    }
        }
        else
        {
            app->c_sel = FlipperHamCallIndexAdd;
            for(j = i; j < CALL_N; j++)
                if(app->calls_used[j]) if(app->calls[j][0]) {
                    app->c_sel = FlipperHamCallIndexBase + j;
                    break;
                }
            if(app->c_sel == FlipperHamCallIndexAdd)
                for(j = i; j > 0; j--)
                    if(app->calls_used[j - 1]) if(app->calls[j - 1][0]) {
                        app->c_sel = FlipperHamCallIndexBase + j - 1;
                        break;
                    }
        }
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

    app->c2_sel = index;

    if(index == FlipperHamC2IndexEdit)
    {
        app->edit_call_index = app->book_call_index;
        snprintf(app->c_edit, sizeof(app->c_edit), "%s", app->calls[app->book_call_index]);
        app->txt = 4;
        app->txt_v = FlipperHamViewC2;
        text_input_reset(app->text_input);
        text_input_set_header_text(app->text_input, "Callsign");
        text_input_set_result_callback(app->text_input, csave, app, app->c_edit, sizeof(app->c_edit), false);
        view_dispatcher_switch_to_view(app->view_dispatcher, FlipperHamViewTextInput);
        return;
    }

    if(index == FlipperHamC2IndexDelete)
    {
        app->calls[app->book_call_index][0] = 0;
        app->calls_used[app->book_call_index] = 0;
        app->bk_sel = FlipperHamBookIndexAdd;
        if(app->book_call_index + 1 < CALL_N)
        {
            uint8_t i;
            for(i = app->book_call_index + 1; i < CALL_N; i++)
                if(app->calls_used[i]) if(app->calls[i][0]) {
                    app->bk_sel = FlipperHamBookIndexBase + i;
                    break;
                }
        }
        if(app->bk_sel == FlipperHamBookIndexAdd)
        {
            uint8_t i;
            for(i = app->book_call_index; i > 0; i--)
                if(app->calls_used[i - 1]) if(app->calls[i - 1][0]) {
                    app->bk_sel = FlipperHamBookIndexBase + i - 1;
                    break;
                }
        }
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


static void flipperham_status_view_alloc(FlipperHamApp* app)
{
    app->view_port = view_port_alloc();
    view_port_draw_callback_set(app->view_port, flipperham_draw_callback, app);
    view_port_input_callback_set(app->view_port, stin, app);
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

static void flipperham_menu_free(FlipperHamApp* app)
{
    if(app->view_dispatcher) {
        view_dispatcher_remove_view(app->view_dispatcher, FlipperHamViewMenu);
        view_dispatcher_remove_view(app->view_dispatcher, FlipperHamViewSend);
        view_dispatcher_remove_view(app->view_dispatcher, FlipperHamViewSettings);
        view_dispatcher_remove_view(app->view_dispatcher, FlipperHamViewBulletin);
        view_dispatcher_remove_view(app->view_dispatcher, FlipperHamViewStatus);
        view_dispatcher_remove_view(app->view_dispatcher, FlipperHamViewMessage);
        view_dispatcher_remove_view(app->view_dispatcher, FlipperHamViewPosition);
        view_dispatcher_remove_view(app->view_dispatcher, FlipperHamViewCall);
        view_dispatcher_remove_view(app->view_dispatcher, FlipperHamViewBook);
        view_dispatcher_remove_view(app->view_dispatcher, FlipperHamViewC2);
        view_dispatcher_remove_view(app->view_dispatcher, FlipperHamViewFreq);
        view_dispatcher_remove_view(app->view_dispatcher, FlipperHamViewFreqEdit);
        view_dispatcher_remove_view(app->view_dispatcher, FlipperHamViewPosEdit);
        view_dispatcher_remove_view(app->view_dispatcher, FlipperHamViewSsid);
        view_dispatcher_remove_view(app->view_dispatcher, FlipperHamViewHam);
        view_dispatcher_remove_view(app->view_dispatcher, FlipperHamViewHamTx);
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

    if(app->ham_menu)
    {
        variable_item_list_free(app->ham_menu);
        app->ham_menu = NULL;
    }

    if(app->ham_tx_menu)
    {
        variable_item_list_free(app->ham_tx_menu);
        app->ham_tx_menu = NULL;
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

    if(app->position_menu)
    {
        submenu_free(app->position_menu);
        app->position_menu = NULL;
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

    if(app->freq_menu)
    {
        submenu_free(app->freq_menu);
        app->freq_menu = NULL;
    }

    if(app->c2_menu)
    {
        submenu_free(app->c2_menu);
        app->c2_menu = NULL;
    }

    if(app->freq_edit_menu)
    {
        variable_item_list_free(app->freq_edit_menu);
        app->freq_edit_menu = NULL;
    }

    if(app->pos_edit_menu)
    {
        variable_item_list_free(app->pos_edit_menu);
        app->pos_edit_menu = NULL;
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


static void stin(InputEvent* event, void* context)
{
    FlipperHamApp* app = context;

    if(event->type != InputTypeShort) return;
    if(event->key != InputKeyBack) return;
      if(!app->r_w) return;

    app->r_x = true;
}


static void gblink(void)
{
    furi_hal_light_blink_stop();
    furi_hal_light_set(LightBlue, 0);
    furi_hal_light_set(LightRed, 255);
      furi_hal_light_set(LightGreen, 72);
}

static uint32_t rt4(FlipperHamApp* app)
{
    if(app->encoding_index == 0) return 4;
    return 1;
}


static FlipperHamApp* flipperham_app_alloc(void)
{
    FlipperHamApp* app = malloc(sizeof(FlipperHamApp));

    app->gui = furi_record_open(RECORD_GUI);
    app->view_dispatcher = view_dispatcher_alloc();
    app->submenu = submenu_alloc();
    app->send_menu = submenu_alloc();
    app->bulletin_menu = submenu_alloc();
    app->status_menu = submenu_alloc();
    app->message_menu = submenu_alloc();
    app->position_menu = submenu_alloc();
    app->call_menu = submenu_alloc();
    app->book_menu = submenu_alloc();
    app->c2_menu = submenu_alloc();
    app->freq_menu = submenu_alloc();
    app->settings_menu = variable_item_list_alloc();
    app->ham_menu = variable_item_list_alloc();
    app->ham_tx_menu = variable_item_list_alloc();
    app->ssid_menu = variable_item_list_alloc();
    app->freq_edit_menu = variable_item_list_alloc();
    app->pos_edit_menu = variable_item_list_alloc();
    app->text_input = text_input_alloc();

        app->view_port = NULL;
        app->tx_started = false;
        app->tx_allowed = true;
        app->tx_done = false;
        app->done_w = false;
        app->send_requested = false;
        app->ham_ok = false;
        app->encoding_index = FlipperHamModemProfileDefault;
        app->ham_n = 0;
        app->repeat_n = 1;
        app->repeat_i = 1;
        app->r_w = false;
        app->r_x = false;
        app->tx_msg_index = 0;
        app->tx_t = 0;
        memset(app->ham_ssid, 0, sizeof(app->ham_ssid));
        memset(app->ham_has_ssid, 0, sizeof(app->ham_has_ssid));
        memset(app->ham_pass, 0, sizeof(app->ham_pass));
        memset(app->ham_calls, 0, sizeof(app->ham_calls));
        app->status_index = 0;
        app->message_index = 0;
        app->pos_index = 0;
        app->dst_call_index = 0;
        app->d_s = 0;
        app->edit_call_index = 0;
        app->book_call_index = 0;
        app->ham_index = 0;
        app->ham_tx_index = 0;
        app->freq_index = 0;
        app->b_sel = FlipperHamBulletinIndexAdd;
        app->st_sel = FlipperHamStatusIndexAdd;
        app->m_sel = FlipperHamMessageIndexAdd;
        app->p_sel = FlipperHamPositionIndexAdd;
        app->c_sel = FlipperHamCallIndexAdd;
          app->bk_sel = FlipperHamBookIndexAdd;
        app->c2_sel = FlipperHamC2IndexEdit;
        app->f_sel = FlipperHamFreqIndexAdd;
        app->h_sel = 0;
        app->ht_sel = 0;
        app->c2_h[0] = 0;
        app->f_edit[0] = 0;
        app->f_bad = false;
        app->go_v = FlipperHamViewMenu;
        app->txt = 0;
        app->txt_v = FlipperHamViewMenu;
        app->pkt = malloc(sizeof(Packet));
        app->wave = malloc(sizeof(uint16_t) * WAVE_N);

        cfgload(app);

    view_dispatcher_enable_queue( app->view_dispatcher);
    view_dispatcher_attach_to_gui(app->view_dispatcher, app->gui, ViewDispatcherTypeFullscreen);

    submenu_add_item( app->submenu, "Send", FlipperHamMenuIndexSend, flipperham_menu_callback, app);
    submenu_add_item( app->submenu, "Settings", FlipperHamMenuIndexSettings, flipperham_menu_callback, app);
    submenu_add_item( app->submenu, "Callbook", FlipperHamMenuIndexCallbook, flipperham_menu_callback, app);
    if(app->ham_ok) submenu_add_item( app->submenu, "Ham Radio", FlipperHamMenuIndexHam, flipperham_menu_callback, app);


    submenu_add_item( app->send_menu, "Message", FlipperHamSendIndexMessage, flipperham_send_callback, app);
    submenu_add_item( app->send_menu, "Position", FlipperHamSendIndexPosition, flipperham_send_callback, app);
    submenu_add_item( app->send_menu, "Status", FlipperHamSendIndexStatus, flipperham_send_callback, app);
    submenu_add_item( app->send_menu, "Bulletin", FlipperHamSendIndexBulletin, flipperham_send_callback, app);


    bmenu(app);
    stmenu(app);
    mmenu(app);
    pmenu(app);
    cmenu(app);
    bkmenu(app);
    fmenu(app);
    rmenu(app);
    ssidfix(app);
    hmenu(app);
    htmenu(app);


    view_set_previous_callback(submenu_get_view(app->submenu), flipperham_exit_callback);
    view_set_previous_callback(submenu_get_view(app->send_menu), flipperham_send_exit_callback);
    view_set_previous_callback(variable_item_list_get_view(app->settings_menu), flipperham_settings_exit_callback);
    view_set_previous_callback(submenu_get_view(app->bulletin_menu), flipperham_bulletin_exit_callback);
    view_set_previous_callback(submenu_get_view(app->status_menu), flipperham_status_exit_callback);
    view_set_previous_callback(submenu_get_view(app->message_menu), flipperham_message_exit_callback);
    view_set_previous_callback(submenu_get_view(app->position_menu), flipperham_position_exit_callback);
    view_set_previous_callback(submenu_get_view(app->call_menu), flipperham_call_exit_callback);
    view_set_previous_callback(submenu_get_view(app->book_menu), bookx);
    view_set_previous_callback(submenu_get_view(app->c2_menu), c2x);
    view_set_previous_callback(submenu_get_view(app->freq_menu), flipperham_freq_exit_callback);
    view_set_previous_callback(variable_item_list_get_view(app->freq_edit_menu), flipperham_freq_edit_exit_callback);
    view_set_previous_callback(variable_item_list_get_view(app->pos_edit_menu), flipperham_pos_edit_exit_callback);
    view_set_previous_callback(variable_item_list_get_view(app->ssid_menu), flipperham_ssid_exit_callback);
    view_set_previous_callback(variable_item_list_get_view(app->ham_menu), flipperham_ham_exit_callback);
    view_set_previous_callback(variable_item_list_get_view(app->ham_tx_menu), flipperham_ham_tx_exit_callback);
    view_set_previous_callback(text_input_get_view(app->text_input), flipperham_text_exit_callback);
    variable_item_list_set_enter_callback(app->ssid_menu, se, app);
    variable_item_list_set_enter_callback(app->settings_menu, re, app);
    variable_item_list_set_enter_callback(app->ham_menu, hre, app);
    variable_item_list_set_enter_callback(app->ham_tx_menu, hte, app);
    variable_item_list_set_enter_callback(app->freq_edit_menu, fe, app);
    variable_item_list_set_enter_callback(app->pos_edit_menu, pe, app);


    view_dispatcher_add_view( app->view_dispatcher, FlipperHamViewMenu, submenu_get_view(app->submenu));
    view_dispatcher_add_view( app->view_dispatcher, FlipperHamViewSend, submenu_get_view(app->send_menu));
    view_dispatcher_add_view( app->view_dispatcher, FlipperHamViewSettings, variable_item_list_get_view(app->settings_menu));
    view_dispatcher_add_view( app->view_dispatcher, FlipperHamViewBulletin, submenu_get_view(app->bulletin_menu));
    view_dispatcher_add_view( app->view_dispatcher, FlipperHamViewStatus, submenu_get_view(app->status_menu));
    view_dispatcher_add_view( app->view_dispatcher, FlipperHamViewMessage, submenu_get_view(app->message_menu));
    view_dispatcher_add_view( app->view_dispatcher, FlipperHamViewPosition, submenu_get_view(app->position_menu));
    view_dispatcher_add_view( app->view_dispatcher, FlipperHamViewCall, submenu_get_view(app->call_menu));
    view_dispatcher_add_view( app->view_dispatcher, FlipperHamViewBook, submenu_get_view(app->book_menu));
    view_dispatcher_add_view( app->view_dispatcher, FlipperHamViewC2, submenu_get_view(app->c2_menu));
    view_dispatcher_add_view( app->view_dispatcher, FlipperHamViewFreq, submenu_get_view(app->freq_menu));
    view_dispatcher_add_view( app->view_dispatcher, FlipperHamViewFreqEdit, variable_item_list_get_view(app->freq_edit_menu));
    view_dispatcher_add_view( app->view_dispatcher, FlipperHamViewPosEdit, variable_item_list_get_view(app->pos_edit_menu));
    view_dispatcher_add_view( app->view_dispatcher, FlipperHamViewSsid, variable_item_list_get_view(app->ssid_menu));
    view_dispatcher_add_view( app->view_dispatcher, FlipperHamViewHam, variable_item_list_get_view(app->ham_menu));
    view_dispatcher_add_view( app->view_dispatcher, FlipperHamViewHamTx, variable_item_list_get_view(app->ham_tx_menu));
    view_dispatcher_add_view( app->view_dispatcher, FlipperHamViewTextInput, text_input_get_view(app->text_input));
    view_dispatcher_switch_to_view(app->view_dispatcher, FlipperHamViewMenu);
    return app;
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
    uint32_t b, c;

    flipperham_status_view_alloc(app);
    app->repeat_i = 0;
    app->repeat_t0 = furi_get_tick();
    app->repeat_to = 0;
    app->r_w = false;
    app->r_x = false;
    app->done_w = false;
    furi_hal_light_blink_stop();
    furi_hal_light_set(LightBlue, 0);
    furi_hal_light_set(LightRed, 0);
    furi_hal_light_set(LightGreen, 0);
    c = rt4(app);
    furi_hal_power_suppress_charge_enter();

    for(i = 0; i < app->repeat_n; i++)
    {
        app->repeat_i = i + 1;

        txstart(app);
        if(!app->tx_ok)
        {
            furi_hal_power_suppress_charge_exit();
            furi_hal_light_blink_stop();
            furi_hal_light_set(LightBlue, 0);
            furi_hal_light_set(LightRed, 0);
            furi_hal_light_set(LightGreen, 0);
            flipperham_status_view_free(app);
            return;
        }
        app->tx_started = false;
        app->tx_allowed = true;
        app->r_w = false;
        app->done_w = false;

        gblink();
        view_port_update(app->view_port);
        furi_delay_ms(100);

        flipperham_radio_start(app);

        while(!app->tx_done) {
            view_port_update(app->view_port);
            furi_delay_ms(50);
        }

        while(app->tx_started && !furi_hal_subghz_is_async_tx_complete()) {
            view_port_update(app->view_port);
            furi_delay_ms(20);
        }

            flipperham_radio_stop(app);
        furi_hal_light_blink_stop();
        furi_hal_light_set(LightBlue, 0);
        furi_hal_light_set(LightRed, 0);
        furi_hal_light_set(LightGreen, 0);

        if(i + 1 >= app->repeat_n) break;

        app->repeat_to = a[i + 1] * c;
        app->r_w = true;
        app->tx_done = false;

        while(1)
        {
            if(app->r_x) break;

            b = furi_get_tick() - app->repeat_t0;
            if(b >= app->repeat_to) break;

            view_port_update(app->view_port);
            furi_delay_ms(50 * c);
        }

        app->r_w = false;
        if(app->r_x) break;
    }

    app->r_w = false;
    app->r_x = false;
    app->done_w = true;
    app->tx_done = true;
    view_port_update(app->view_port);
    furi_delay_ms(750);
    furi_hal_power_suppress_charge_exit();
    furi_hal_light_blink_stop();
    furi_hal_light_set(LightBlue, 0);
    furi_hal_light_set(LightRed, 0);
    furi_hal_light_set(LightGreen, 0);

    flipperham_status_view_free(app);
}


static void bx0(void* context, uint32_t index) { bx(context, InputTypeShort, index); }
static void sx0(void* context, uint32_t index) { sx(context, InputTypeShort, index); }
static void cx0(void* context, uint32_t index) { cx(context, InputTypeShort, index); }
static void fq0(void* context, uint32_t index) { fq(context, InputTypeShort, index); }
static void mx0(void* context, uint32_t index) { mx(context, InputTypeShort, index); }
static void px0(void* context, uint32_t index) { px(context, InputTypeShort, index); }


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
