#include "../flipperham.h"
#include "../aprs.h"
#include "ui.h"
#include "../rf_gen.h"

#include <furi_hal.h>
#include <gui/view.h>
#include <notification/notification.h>
#include <notification/notification_messages.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static uint32_t flipperham_exit_callback(void *context);
static uint32_t flipperham_send_exit_callback(void *context);
static uint32_t flipperham_settings_exit_callback(void *context);
static uint32_t flipperham_bulletin_exit_callback(void *context);
static uint32_t flipperham_status_exit_callback(void *context);
static uint32_t flipperham_message_exit_callback(void *context);
static uint32_t flipperham_position_exit_callback(void *context);
static uint32_t flipperham_ssid_exit_callback(void *context);
static uint32_t flipperham_call_exit_callback(void *context);
static uint32_t flipperham_freq_exit_callback(void *context);
static uint32_t flipperham_freq_edit_exit_callback(void *context);
static uint32_t flipperham_pos_edit_exit_callback(void *context);
static uint32_t flipperham_ham_exit_callback(void *context);
static uint32_t flipperham_ham_tx_exit_callback(void *context);
static uint32_t flipperham_readme_exit_callback(void *context);
static uint32_t book_exit(void *context);
static uint32_t book_action_exit(void *context);
static uint32_t flipperham_text_exit_callback(void *context);

static void flipperham_bulletin_callback(void *context, uint32_t index);
static void bulletin_pick(void *context, InputType input_type, uint32_t index);
static void st(void *context, uint32_t index);
static void status_pick(void *context, InputType input_type, uint32_t index);
static void message_pick(void *context, InputType input_type, uint32_t index);
static void m(void *context, uint32_t index);
static void p(void *context, uint32_t index);
static void position_pick(void *context, InputType input_type, uint32_t index);
static void call_pick(void *context, InputType input_type, uint32_t index);
static void cl(void *context, uint32_t index);
static void cb(void *context, uint32_t index);
static void c2(void *context, uint32_t index);
static void freq_pick(void *context, InputType input_type, uint32_t index);
static void bulletin_save(void *context);
static void status_save(void *context);
static void message_save(void *context);
static void position_save(void *context);
static void call_save(void *context);
static void freq_save(void *context);

static void bulletin_menu_build(FlipperHamApp *app);
static void status_menu_build(FlipperHamApp *app);
static void message_menu_build(FlipperHamApp *app);
static void position_menu_build(FlipperHamApp *app);
static void call_menu_build(FlipperHamApp *app);
static void book_menu_build(FlipperHamApp *app);
static void book_action_menu_build(FlipperHamApp *app);
static void ssid_menu_build(FlipperHamApp *app);
static void settings_menu_build(FlipperHamApp *app);
static void ham_menu_build(FlipperHamApp *app);
static void ham_tx_menu_build(FlipperHamApp *app);
static void freq_menu_build(FlipperHamApp *app);
static void freq_edit_menu_build(FlipperHamApp *app);
static void pos_edit_menu_build(FlipperHamApp *app);
static void ssidfix(FlipperHamApp *app);
static void ssid_change(VariableItem *item);
static void baud_change(VariableItem *item);
static void profile_change(VariableItem *item);
static void deviation_change(VariableItem *item);
static void repeat_change(VariableItem *item);
static void leadin_change(VariableItem *item);
static void preamble_change(VariableItem *item);
static void ham_call_change(VariableItem *item);
static void ham_ssid_change(VariableItem *item);
static void freq_change(VariableItem *item);
static void ssid_enter(void *context, uint32_t index);
static void settings_enter(void *context, uint32_t index);
static void ham_enter(void *context, uint32_t index);
static void ham_tx_enter(void *context, uint32_t index);
static void freq_edit_enter(void *context, uint32_t index);
static void pos_edit_enter(void *context, uint32_t index);
static uint32_t freq_step(uint32_t a, int8_t d);
static uint32_t freq_min_hz(void);
static uint32_t freq_max_hz(void);
static bool freq_band(uint32_t a, uint32_t *lo, uint32_t *hi);
static uint32_t freq_next_band(uint32_t a);
static void freq_show(char *o, uint16_t n, uint32_t a);
static bool freq_parse(const char *s, uint32_t *out);

static void flipperham_draw_callback(Canvas *canvas, void *context);
static void ham_blank_draw(Canvas *canvas, void *context);
static void ham_blank_input(InputEvent *event, void *context);
static void ham_morse_play(FlipperHamApp *app);
static void stin(InputEvent *event, void *context);
static void flipperham_status_view_alloc(FlipperHamApp *app);
static void flipperham_status_view_free(FlipperHamApp *app);
static void flipperham_menu_free(FlipperHamApp *app);
static FlipperHamApp *flipperham_app_alloc(void);
static void flipperham_app_free(FlipperHamApp *app);
static void flipperham_send_hardcoded_message(FlipperHamApp *app);
static void flipperham_menu_callback(void *context, uint32_t index);
static void flipperham_send_callback(void *context, uint32_t index);
static void readme_back(GuiButtonType result, InputType type, void *context);

static FlipperHamApp *gapp;
static bool call_copy(FlipperHamApp *app);
static void tx_blink_green(void);
static uint32_t repeat_scale(FlipperHamApp *app);

static void flipperham_draw_callback(Canvas *canvas, void *context)
{
    FlipperHamApp *app = context;
    char a[16];
    uint32_t n, w, m, x;

    canvas_clear(canvas);
    canvas_set_font(canvas, FontBigNumbers);
    snprintf(a, sizeof(a), "%06lu", (unsigned long)(tx_freq_get(app) / 1000UL));
    canvas_draw_str_aligned(canvas, 64, 11, AlignCenter, AlignCenter, a);
    canvas_set_font(canvas, FontPrimary);

    if (!app->tx_allowed)
    {
        canvas_draw_str_aligned(canvas, 64, 32, AlignCenter, AlignCenter, "TX blocked");
        return;
    }

    if (app->show_done)
    {
        canvas_draw_str_aligned(canvas, 64, 32, AlignCenter, AlignCenter, "Done");
        return;
    }

    if (app->repeat_n >= 4)
        canvas_draw_str_aligned(canvas, 64, 24, AlignCenter, AlignCenter, "Sending...");
    else if (app->repeat_n > 1)
        canvas_draw_str_aligned(canvas, 64, 26, AlignCenter, AlignCenter, "Sending...");
    else
        canvas_draw_str_aligned(canvas, 64, 32, AlignCenter, AlignCenter, "Sending...");

    if (app->repeat_n > 1)
    {
        snprintf(a, sizeof(a), "%u/%u", app->repeat_i, app->repeat_n);
        canvas_set_font(canvas, FontSecondary);
        if (app->repeat_n >= 4)
            canvas_draw_str_aligned(canvas, 64, 42, AlignCenter, AlignCenter, a);
        else
            canvas_draw_str_aligned(canvas, 64, 38, AlignCenter, AlignCenter, a);
        canvas_set_font(canvas, FontPrimary);
    }

    if (app->repeat_n >= 4)
    {
        x = repeat_scale(app);
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
        if (w)
            canvas_draw_box(canvas, 24, 32, w, 3);
    }
}

static void ham_blank_draw(Canvas *canvas, void *context)
{
    UNUSED(context);
    canvas_clear(canvas);
}

static void ham_blank_input(InputEvent *event, void *context)
{
    UNUSED(event);
    UNUSED(context);
}

static void ham_morse_play(FlipperHamApp *app)
{
    static const char *a = "73 DE YO3GND";
    ViewPort *v;
    NotificationApp *n;
    const char *p;
    uint32_t d;
    uint8_t i, j;

    v = view_port_alloc();
    view_port_draw_callback_set(v, ham_blank_draw, app);
    view_port_input_callback_set(v, ham_blank_input, app);
    gui_add_view_port(app->gui, v, GuiLayerFullscreen);
    view_port_update(v);

    n = furi_record_open(RECORD_NOTIFICATION);
    furi_hal_light_blink_stop();
    furi_hal_light_set(LightRed, 0);
    furi_hal_light_set(LightGreen, 0);
    furi_hal_light_set(LightBlue, 0);
    notification_message(n, &sequence_display_backlight_off);
    notification_message(n, &sequence_reset_rgb);
    furi_delay_ms(1000);

    if (!furi_hal_speaker_acquire(1000))
        goto x;

    for (i = 0; a[i]; i++)
    {
        p = NULL;
        switch (a[i])
        {
        case '7':
            p = "--...";
            break;
        case '3':
            p = "...--";
            break;
        case 'D':
            p = "-..";
            break;
        case 'E':
            p = ".";
            break;
        case 'Y':
            p = "-.--";
            break;
        case 'O':
            p = "---";
            break;
        case 'G':
            p = "--.";
            break;
        case 'N':
            p = "-.";
            break;
        case ' ':
            break;
        default:
            break;
        }

        if (!p)
            continue;

        for (j = 0; p[j]; j++)
        {
            d = (p[j] == '-') ? 180 : 60;
            notification_message(n, &sequence_display_backlight_on);
            furi_hal_speaker_start(880.0f, 0.7f);
            furi_delay_ms(d);
            furi_hal_speaker_stop();
            notification_message(n, &sequence_display_backlight_off);
            if (p[j + 1])
                furi_delay_ms(60);
        }

        if (!a[i + 1])
            continue;
        if (a[i + 1] == ' ')
            furi_delay_ms(420);
        else
            furi_delay_ms(180);
    }

    furi_hal_speaker_release();

x:
    notification_message(n, &sequence_display_backlight_enforce_auto);
    notification_message(n, &sequence_display_backlight_on);
    furi_record_close(RECORD_NOTIFICATION);
    gui_remove_view_port(app->gui, v);
    view_port_free(v);
}

static bool call_copy(FlipperHamApp *app)
{
    char a[CALL_LEN];
    char b[CALL_LEN];
    uint8_t i, j, s, k, p, x;
    bool d;
    bool f;

    if (app->book_call_index >= CALL_N)
        return false;
    if (!app->calls_used[app->book_call_index])
        return false;
    if (!app->calls[app->book_call_index][0])
        return false;
    if (!call_split(app->calls[app->book_call_index], a, &s, &d))
        return false;

    k = d ? (s + 1) : 0;

    for (i = 0; i < 16; i++)
    {
        s = (k + i) & 15;
        p = 0;
        j = 0;
        while (a[j])
            b[p++] = a[j++];
        b[p++] = '-';
        if (s >= 10)
            b[p++] = '0' + (s / 10);
        b[p++] = '0' + (s % 10);
        b[p] = 0;
        f = false;

        for (x = 0; x < CALL_N; x++)
        {
            if (x == app->book_call_index)
                continue;
            if (!app->calls_used[x])
                continue;
            if (strcmp(app->calls[x], b))
                continue;
            f = true;
            break;
        }

        if (f)
            continue;

        for (j = 0; j < CALL_N; j++)
            if (!app->calls_used[j] || !app->calls[j][0])
            {
                snprintf(app->calls[j], sizeof(app->calls[j]), "%s", b);
                app->calls_used[j] = 1;
                app->book_sel = FlipperHamBookIndexBase + j;
                app->book_call_index = j;
                calls_fix(app);
                cfgsave(app);
                callbook_save_txt(app);
                call_menu_build(app);
                book_menu_build(app);
                return true;
            }

        break;
    }

    return false;
}

static uint32_t flipperham_exit_callback(void *context)
{
    UNUSED(context);
    return VIEW_NONE;
}

static uint32_t flipperham_send_exit_callback(void *context)
{
    UNUSED(context);

    return FlipperHamViewMenu;
}

static uint32_t flipperham_settings_exit_callback(void *context)
{
    UNUSED(context);

    return FlipperHamViewMenu;
}

static uint32_t flipperham_bulletin_exit_callback(void *context)
{
    UNUSED(context);

    return FlipperHamViewSend;
}

static uint32_t flipperham_status_exit_callback(void *context)
{
    UNUSED(context);

    return FlipperHamViewSend;
}

static uint32_t flipperham_message_exit_callback(void *context)
{
    UNUSED(context);

    return FlipperHamViewSend;
}

static uint32_t flipperham_position_exit_callback(void *context)
{
    UNUSED(context);

    return FlipperHamViewSend;
}

static uint32_t flipperham_ssid_exit_callback(void *context)
{
    UNUSED(context);

    return FlipperHamViewCall;
}

static uint32_t flipperham_call_exit_callback(void *context)
{
    FlipperHamApp *app = gapp;

    UNUSED(context);
    if (!app)
        return FlipperHamViewSend;
    if (app->tx_type == 2)
        return FlipperHamViewMessage;
    return FlipperHamViewSend;
}

static uint32_t flipperham_freq_exit_callback(void *context)
{
    UNUSED(context);

    return FlipperHamViewSettings;
}

static uint32_t flipperham_freq_edit_exit_callback(void *context)
{
    UNUSED(context);

    return FlipperHamViewFreq;
}

static uint32_t flipperham_pos_edit_exit_callback(void *context)
{
    UNUSED(context);

    return FlipperHamViewPosition;
}

static uint32_t flipperham_ham_exit_callback(void *context)
{
    UNUSED(context);

    return FlipperHamViewMenu;
}

static uint32_t flipperham_ham_tx_exit_callback(void *context)
{
    UNUSED(context);

    return FlipperHamViewHam;
}

static uint32_t flipperham_readme_exit_callback(void *context)
{
    UNUSED(context);

    return FlipperHamViewMenu;
}

static uint32_t book_exit(void *context)
{
    UNUSED(context);

    return FlipperHamViewMenu;
}

static uint32_t book_action_exit(void *context)
{
    UNUSED(context);

    return FlipperHamViewBook;
}

static uint32_t flipperham_text_exit_callback(void *context)
{
    FlipperHamApp *app = gapp;

    UNUSED(context);
    if (!app)
        return FlipperHamViewMenu;
    return app->text_view;
}

static void flipperham_menu_callback(void *context, uint32_t index)
{
    FlipperHamApp *app = context;

    if (index == FlipperHamMenuIndexSend)
        view_dispatcher_switch_to_view(app->view_dispatcher, FlipperHamViewSend);
    if (index == FlipperHamMenuIndexSettings)
        view_dispatcher_switch_to_view(app->view_dispatcher, FlipperHamViewSettings);
    if (index == FlipperHamMenuIndexCallbook)
    {
        book_menu_build(app);
        view_dispatcher_switch_to_view(app->view_dispatcher, FlipperHamViewBook);
    }
    if (index == FlipperHamMenuIndexHam)
        view_dispatcher_switch_to_view(app->view_dispatcher, FlipperHamViewHam);
    if (index == FlipperHamMenuIndexReadme)
        view_dispatcher_switch_to_view(app->view_dispatcher, FlipperHamViewReadme);
}

static void readme_back(GuiButtonType result, InputType type, void *context)
{
    FlipperHamApp *app = context;

    if (type != InputTypeShort)
        return;
    if (result != GuiButtonTypeLeft)
        return;
    view_dispatcher_switch_to_view(app->view_dispatcher, FlipperHamViewMenu);
}

static void flipperham_send_callback(void *context, uint32_t index)
{
    FlipperHamApp *app = context;

    if (index == FlipperHamSendIndexMessage)
    {
        message_menu_build(app);
        view_dispatcher_switch_to_view(app->view_dispatcher, FlipperHamViewMessage);
    }

    if (index == FlipperHamSendIndexPosition)
    {
        position_menu_build(app);
        view_dispatcher_switch_to_view(app->view_dispatcher, FlipperHamViewPosition);
    }

    if (index == FlipperHamSendIndexBulletin)
    {
        bulletin_menu_build(app);
        view_dispatcher_switch_to_view(app->view_dispatcher, FlipperHamViewBulletin);
    }

    if (index == FlipperHamSendIndexStatus)
    {
        status_menu_build(app);
        view_dispatcher_switch_to_view(app->view_dispatcher, FlipperHamViewStatus);
    }
}

static void bulletin_menu_build(FlipperHamApp *app)
{
    uint8_t i;

    submenu_reset(app->bulletin_menu);
    submenu_add_item(app->bulletin_menu, "Add new...", FlipperHamBulletinIndexAdd,
                     flipperham_bulletin_callback, app);

    for (i = 0; i < TXT_N; i++)
    {
        if (!app->bulletin_used[i])
            continue;
        if (!app->bulletin[i][0])
            continue;

        submenu_add_item_ex(app->bulletin_menu, app->bulletin[i], FlipperHamBulletinIndexBase + i,
                            bulletin_pick, app);
    }

    submenu_set_selected_item(app->bulletin_menu, FlipperHamBulletinIndexAdd);
    if (app->bulletin_sel >= FlipperHamBulletinIndexBase)
    {
        i = app->bulletin_sel - FlipperHamBulletinIndexBase;
        if (i < TXT_N)
            if (app->bulletin_used[i])
                if (app->bulletin[i][0])
                    submenu_set_selected_item(app->bulletin_menu, app->bulletin_sel);
    }
}

static void status_menu_build(FlipperHamApp *app)
{
    uint8_t i;

    submenu_reset(app->status_menu);
    submenu_add_item(app->status_menu, "Add new...", FlipperHamStatusIndexAdd, st, app);

    for (i = 0; i < TXT_N; i++)
    {
        if (!app->status_used[i])
            continue;
        if (!app->status[i][0])
            continue;

        submenu_add_item_ex(app->status_menu, app->status[i], FlipperHamStatusIndexBase + i,
                            status_pick, app);
    }

    submenu_set_selected_item(app->status_menu, FlipperHamStatusIndexAdd);
    if (app->status_sel >= FlipperHamStatusIndexBase)
    {
        i = app->status_sel - FlipperHamStatusIndexBase;
        if (i < TXT_N)
            if (app->status_used[i])
                if (app->status[i][0])
                    submenu_set_selected_item(app->status_menu, app->status_sel);
    }
}

static void call_menu_build(FlipperHamApp *app)
{
    uint8_t i;

    submenu_reset(app->call_menu);
    if (app->tx_type == 2)
        submenu_set_header(app->call_menu, "Destination:");
    else
        submenu_set_header(app->call_menu, NULL);
    submenu_add_item(app->call_menu, "Add new callsign...", FlipperHamCallIndexAdd, cl, app);

    for (i = 0; i < CALL_N; i++)
    {
        if (!app->calls_used[i])
            continue;
        if (!app->calls[i][0])
            continue;

        submenu_add_item_ex(app->call_menu, app->calls[i], FlipperHamCallIndexBase + i, call_pick,
                            app);
    }

    submenu_set_selected_item(app->call_menu, FlipperHamCallIndexAdd);
    if (app->call_sel >= FlipperHamCallIndexBase)
    {
        i = app->call_sel - FlipperHamCallIndexBase;
        if (i < CALL_N)
            if (app->calls_used[i])
                if (app->calls[i][0])
                    submenu_set_selected_item(app->call_menu, app->call_sel);
    }
}

static void book_menu_build(FlipperHamApp *app)
{
    uint8_t i;

    submenu_reset(app->book_menu);
    submenu_add_item(app->book_menu, "Add new callsign...", FlipperHamBookIndexAdd, cb, app);

    for (i = 0; i < CALL_N; i++)
    {
        if (!app->calls_used[i])
            continue;
        if (!app->calls[i][0])
            continue;

        submenu_add_item(app->book_menu, app->calls[i], FlipperHamBookIndexBase + i, cb, app);
    }

    submenu_set_selected_item(app->book_menu, FlipperHamBookIndexAdd);
    if (app->book_sel >= FlipperHamBookIndexBase)
    {
        i = app->book_sel - FlipperHamBookIndexBase;
        if (i < CALL_N)
            if (app->calls_used[i])
                if (app->calls[i][0])
                    submenu_set_selected_item(app->book_menu, app->book_sel);
    }
}

static void book_action_menu_build(FlipperHamApp *app)
{
    submenu_reset(app->c2_menu);
    submenu_set_header(app->c2_menu, app->c2_h);
    submenu_add_item(app->c2_menu, "Edit", FlipperHamC2IndexEdit, c2, app);
    submenu_add_item(app->c2_menu, "Delete", FlipperHamC2IndexDelete, c2, app);
    submenu_add_item(app->c2_menu, "Copy", FlipperHamC2IndexCopy, c2, app);
    submenu_set_selected_item(app->c2_menu, app->book_action_sel);
}

static uint32_t freq_step(uint32_t a, int8_t d)
{
    uint32_t b;
    uint32_t c;
    uint32_t e;

    b = a;
    if (!freq_band(b, &c, &e))
    {
        c = freq_min_hz();
        e = freq_max_hz();
        if (b < c)
            b = c;
        if (b > e)
            b = e;
        if (!freq_band(b, &c, &e))
            return b;
    }

    if (d > 0)
    {
        if (b >= e)
            return e;
        b += 2500UL;
        if (b > e)
            b = e;
    }
    else
    {
        if (b <= c)
            return c;
        b -= 2500UL;
        if (b < c)
            b = c;
    }

    return b;
}

static uint32_t freq_min_hz(void)
{
    uint32_t a;

    for (a = 0; a < 1000000000UL; a += 2500UL)
        if (furi_hal_subghz_is_frequency_valid(a))
            return a;

    return CARRIER_HZ;
}

static uint32_t freq_max_hz(void)
{
    uint32_t a;

    for (a = 1000000000UL - 1; a > 2500UL; a -= 2500UL)
        if (furi_hal_subghz_is_frequency_valid(a))
            return a;

    return CARRIER_HZ;
}

static bool freq_band(uint32_t a, uint32_t *lo, uint32_t *hi)
{
    uint32_t b;

    if (!furi_hal_subghz_is_frequency_valid(a))
        return false;

    b = a;
    while (b >= 2500UL)
    {
        if (!furi_hal_subghz_is_frequency_valid(b - 2500UL))
            break;
        b -= 2500UL;
    }
    *lo = b;

    b = a;
    while (b <= 1000000000UL - 2500UL)
    {
        if (!furi_hal_subghz_is_frequency_valid(b + 2500UL))
            break;
        b += 2500UL;
    }
    *hi = b;

    return true;
}

static uint32_t freq_next_band(uint32_t a)
{
    uint32_t c;
    uint32_t e;
    uint32_t b;
    uint32_t message_pick;

    message_pick = freq_max_hz();
    if (!freq_band(a, &c, &e))
        return freq_min_hz();

    b = e;
    while (b <= message_pick - 2500UL)
    {
        b += 2500UL;
        if (furi_hal_subghz_is_frequency_valid(b))
            return b;
    }

    return freq_min_hz();
}

static void freq_show(char *o, uint16_t n, uint32_t a)
{
    if (!o)
        return;
    if (!n)
        return;

    snprintf(o, n, "%06lu.%1lu", (unsigned long)(a / 1000UL),
             (unsigned long)((a % 1000UL) / 100UL));
}

static void fsh2(char *o, uint16_t n, uint32_t a)
{
    if (!o)
        return;
    if (!n)
        return;

    snprintf(o, n, "%06lu", (unsigned long)(a / 1000UL));
}

static bool freq_parse(const char *s, uint32_t *out)
{
    char *e;
    uint32_t a;
    uint32_t b;

    if (!s)
        return false;
    if (!out)
        return false;
    if (!s[0])
        return false;

    a = strtoul(s, &e, 10);
    if (e == s)
        return false;
    if (*e)
        return false;

    b = a;
    if (furi_hal_subghz_is_frequency_valid(b))
    {
        *out = b;
        return true;
    }

    if (a <= 1000000UL)
    {
        b = a * 1000UL;
        if (furi_hal_subghz_is_frequency_valid(b))
        {
            *out = b;
            return true;
        }
    }

    if (a <= 1000UL)
    {
        b = a * 1000000UL;
        if (furi_hal_subghz_is_frequency_valid(b))
        {
            *out = b;
            return true;
        }
    }

    return false;
}

static void freq_menu_build(FlipperHamApp *app)
{
    uint8_t i;

    submenu_reset(app->freq_menu);

    if (app->freq_n < FREQ_N)
        submenu_add_item_ex(app->freq_menu, "Add new...", FlipperHamFreqIndexAdd, freq_pick, app);

    for (i = 0; i < FREQ_N; i++)
    {
        if (!app->freq_used[i])
            continue;
        if (app->tx_freq_index == i)
            snprintf(app->freq_s[i], sizeof(app->freq_s[i]), "%06lu.%1lu *",
                     (unsigned long)(app->freq[i] / 1000UL),
                     (unsigned long)((app->freq[i] % 1000UL) / 100UL));
        else
            freq_show(app->freq_s[i], sizeof(app->freq_s[i]), app->freq[i]);
        submenu_add_item_ex(app->freq_menu, app->freq_s[i], FlipperHamFreqIndexBase + i, freq_pick,
                            app);
    }

    if (app->freq_n < FREQ_N)
        submenu_set_selected_item(app->freq_menu, FlipperHamFreqIndexAdd);
    else
        submenu_set_selected_item(app->freq_menu, FlipperHamFreqIndexBase + app->tx_freq_index);

    if (app->freq_sel >= FlipperHamFreqIndexBase)
    {
        i = app->freq_sel - FlipperHamFreqIndexBase;
        if (i < FREQ_N)
            if (app->freq_used[i])
                submenu_set_selected_item(app->freq_menu, app->freq_sel);
    }
}

static void freq_edit_menu_build(FlipperHamApp *app)
{
    VariableItem *it;

    variable_item_list_reset(app->freq_edit_menu);

    it = variable_item_list_add(app->freq_edit_menu, "Frequency", 201, freq_change, app);
    variable_item_set_current_value_index(it, 100);
    fsh2(app->f_edit, sizeof(app->f_edit), app->freq_edit_hz);
    variable_item_set_current_value_text(it, app->f_edit);

    it = variable_item_list_add(app->freq_edit_menu, "Enter frequency", 1, NULL, NULL);
    variable_item_set_current_value_index(it, 0);
    variable_item_set_current_value_text(it, app->f_bad ? "bad" : "");

    variable_item_list_add(app->freq_edit_menu, "Save", 1, NULL, NULL);
    variable_item_list_add(app->freq_edit_menu, "Select for TX", 1, NULL, NULL);

    if (app->freq_n > 1)
        if (app->freq_index < FREQ_N)
            if (app->freq_used[app->freq_index])
                variable_item_list_add(app->freq_edit_menu, "Delete", 1, NULL, NULL);
    variable_item_list_set_selected_item(app->freq_edit_menu, 0);
}

static void ssid_menu_build(FlipperHamApp *app)
{
    VariableItem *it;
    char a[4];

    variable_item_list_reset(app->ssid_menu);

    it = variable_item_list_add(app->ssid_menu, "SSID", 16, ssid_change, app);
    variable_item_set_current_value_index(it, app->dst_ssid);
    snprintf(a, sizeof(a), "%u", app->dst_ssid);
    variable_item_set_current_value_text(it, a);

    variable_item_list_add(app->ssid_menu, "Send", 0, NULL, NULL);
    variable_item_list_set_selected_item(app->ssid_menu, 0);
}

static void ham_menu_build(FlipperHamApp *app)
{
    VariableItem *it;
    char a[16];

    variable_item_list_reset(app->ham_menu);
    if (!app->ham_n)
        return;
    if (app->ham_index >= app->ham_n)
        app->ham_index = 0;

    it = variable_item_list_add(app->ham_menu, "Callsign", app->ham_n, ham_call_change, app);
    variable_item_set_current_value_index(it, app->ham_index);
    if (app->ham_has_ssid[app->ham_index])
        snprintf(a, sizeof(a), "%s-%u", app->ham_calls[app->ham_index],
                 app->ham_ssid[app->ham_index]);
    else
        snprintf(a, sizeof(a), "%s", app->ham_calls[app->ham_index]);
    variable_item_set_current_value_text(it, a);

    variable_item_list_add(app->ham_menu, "73 de YO3GND", 1, NULL, NULL);
    variable_item_list_set_selected_item(app->ham_menu, app->ham_sel);
}

static void ham_tx_menu_build(FlipperHamApp *app)
{
    VariableItem *it;
    char a[4];

    variable_item_list_reset(app->ham_tx_menu);
    if (!app->ham_n)
        return;
    if (app->ham_tx_index >= HAM_N)
        app->ham_tx_index = 0;

    it = variable_item_list_add(app->ham_tx_menu, "SSID", 16, ham_ssid_change, app);
    variable_item_set_current_value_index(it, app->ham_ssid[app->ham_tx_index]);
    snprintf(a, sizeof(a), "%u", app->ham_ssid[app->ham_tx_index]);
    variable_item_set_current_value_text(it, a);

    variable_item_list_add(app->ham_tx_menu, "Select for TX", 0, NULL, NULL);
    variable_item_list_set_selected_item(app->ham_tx_menu, app->ham_tx_sel);
}

static void settings_menu_build(FlipperHamApp *app)
{
    VariableItem *it;
    char a[16];

    variable_item_list_reset(app->settings_menu);

    it = variable_item_list_add(app->settings_menu, "Frequency", 1, NULL, NULL);
    freq_show(a, sizeof(a), tx_freq_get(app));
    variable_item_set_current_value_index(it, 0);
    variable_item_set_current_value_text(it, a);

    it = variable_item_list_add(
        app->settings_menu, "Baud",
        sizeof(flipperham_modem_profiles) / sizeof(flipperham_modem_profiles[0]), baud_change, app);
    variable_item_set_current_value_index(it, app->encoding_index);
    variable_item_set_current_value_text(it, flipperham_modem_profiles[app->encoding_index].name);

    it = variable_item_list_add(app->settings_menu, "CC1101 profile", 2, profile_change, app);
    variable_item_set_current_value_index(it, app->rf_mod);
    variable_item_set_current_value_text(it, app->rf_mod ? "GFSK" : "2FSK");

    it = variable_item_list_add(app->settings_menu, "Deviation", 9, deviation_change, app);
    variable_item_set_current_value_index(it, app->rf_dev);
    {
        static const char *dl[] = {"1.6", "1.8", "2.0", "2.2", "2.4", "2.5", "2.8", "3.0", "5.0"};
        variable_item_set_current_value_text(it, dl[app->rf_dev < 9 ? app->rf_dev : 8]);
    }

    it = variable_item_list_add(app->settings_menu, "Repeat TX", 5, repeat_change, app);
    variable_item_set_current_value_index(it, app->repeat_n - 1);
    snprintf(a, sizeof(a), "%u", app->repeat_n);
    variable_item_set_current_value_text(it, a);

    it = variable_item_list_add(app->settings_menu, "Lead-in (ms)", 21, leadin_change, app);
    variable_item_set_current_value_index(it, app->leadin_ms / 50);
    snprintf(a, sizeof(a), "%u", app->leadin_ms);
    variable_item_set_current_value_text(it, a);

    it = variable_item_list_add(app->settings_menu, "Preamble (ms)", 21, preamble_change, app);
    variable_item_set_current_value_index(it, app->preamble_ms / 50);
    snprintf(a, sizeof(a), "%u", app->preamble_ms);
    variable_item_set_current_value_text(it, a);
}

static void message_menu_build(FlipperHamApp *app)
{
    uint8_t i;

    submenu_reset(app->message_menu);
    submenu_add_item(app->message_menu, "Add new...", FlipperHamMessageIndexAdd, m, app);

    for (i = 0; i < TXT_N; i++)
    {
        if (!app->message_used[i])
            continue;
        if (!app->message[i][0])
            continue;

        submenu_add_item_ex(app->message_menu, app->message[i], FlipperHamMessageIndexBase + i,
                            message_pick, app);
    }

    submenu_set_selected_item(app->message_menu, FlipperHamMessageIndexAdd);
    if (app->message_sel >= FlipperHamMessageIndexBase)
    {
        i = app->message_sel - FlipperHamMessageIndexBase;
        if (i < TXT_N)
            if (app->message_used[i])
                if (app->message[i][0])
                    submenu_set_selected_item(app->message_menu, app->message_sel);
    }
}

static void position_menu_build(FlipperHamApp *app)
{
    uint8_t i;

    submenu_reset(app->position_menu);
    submenu_add_item(app->position_menu, "Add new...", FlipperHamPositionIndexAdd, p, app);

    for (i = 0; i < TXT_N; i++)
    {
        if (!app->pos_used[i])
            continue;
        if (!app->pos_name[i][0])
            continue;

        submenu_add_item_ex(app->position_menu, app->pos_name[i], FlipperHamPositionIndexBase + i,
                            position_pick, app);
    }

    submenu_set_selected_item(app->position_menu, FlipperHamPositionIndexAdd);
    if (app->position_sel >= FlipperHamPositionIndexBase)
    {
        i = app->position_sel - FlipperHamPositionIndexBase;
        if (i < TXT_N)
            if (app->pos_used[i])
                if (app->pos_name[i][0])
                    submenu_set_selected_item(app->position_menu, app->position_sel);
    }
}

static void pos_edit_menu_build(FlipperHamApp *app)
{
    VariableItem *it;
    const char *name_label;
    const char *lat_label;
    const char *lon_label;

    variable_item_list_reset(app->pos_edit_menu);

    name_label = "Name";
    lat_label = "Latitude";
    lon_label = "Longitude";
    if (app->position_sel >= FlipperHamPositionIndexBase)
    {
        name_label = "Edit name";
        lat_label = "Edit latitude";
        lon_label = "Edit longitude";
    }

    it = variable_item_list_add(app->pos_edit_menu, name_label, 1, NULL, NULL);
    variable_item_set_current_value_index(it, 0);
    variable_item_set_current_value_text(it, app->p_name_edit);

    it = variable_item_list_add(app->pos_edit_menu, lat_label, 1, NULL, NULL);
    variable_item_set_current_value_index(it, 0);
    variable_item_set_current_value_text(it, app->p_lat_edit);

    it = variable_item_list_add(app->pos_edit_menu, lon_label, 1, NULL, NULL);
    variable_item_set_current_value_index(it, 0);
    variable_item_set_current_value_text(it, app->p_lon_edit);

    if (app->pos_n > 1)
        if (app->pos_index < TXT_N)
            if (app->pos_used[app->pos_index])
                variable_item_list_add(app->pos_edit_menu, "Delete", 1, NULL, NULL);

    variable_item_list_set_selected_item(app->pos_edit_menu, 0);
}

static void ssidfix(FlipperHamApp *app)
{
    if (app->dst_ssid > 15)
        app->dst_ssid = 0;
    ssid_menu_build(app);
}

static void ssid_change(VariableItem *item)
{
    FlipperHamApp *app = variable_item_get_context(item);
    char a[4];

    app->dst_ssid = variable_item_get_current_value_index(item);
    snprintf(a, sizeof(a), "%u", app->dst_ssid);
    variable_item_set_current_value_text(item, a);
}

static void ham_call_change(VariableItem *item)
{
    FlipperHamApp *app = variable_item_get_context(item);
    char a[16];

    app->ham_index = variable_item_get_current_value_index(item);
    if (app->ham_n)
        if (app->ham_index >= app->ham_n)
            app->ham_index = 0;
    if (app->ham_has_ssid[app->ham_index])
        snprintf(a, sizeof(a), "%s-%u", app->ham_calls[app->ham_index],
                 app->ham_ssid[app->ham_index]);
    else
        snprintf(a, sizeof(a), "%s", app->ham_calls[app->ham_index]);
    variable_item_set_current_value_text(item, a);
    if (app->ham_ok)
        if (app->ham_n > 1)
            cfgsave(app);
}

static void ham_ssid_change(VariableItem *item)
{
    FlipperHamApp *app = variable_item_get_context(item);
    char a[4];

    app->ham_ssid[app->ham_tx_index] = variable_item_get_current_value_index(item);
    app->ham_has_ssid[app->ham_tx_index] = true;
    snprintf(a, sizeof(a), "%u", app->ham_ssid[app->ham_tx_index]);
    variable_item_set_current_value_text(item, a);
    ham_save_txt(app);
}

static void baud_change(VariableItem *item)
{
    FlipperHamApp *app = variable_item_get_context(item);

    app->encoding_index = variable_item_get_current_value_index(item);
    if (app->encoding_index >=
        sizeof(flipperham_modem_profiles) / sizeof(flipperham_modem_profiles[0]))
        app->encoding_index = FlipperHamModemProfileDefault;

    variable_item_set_current_value_text(item, flipperham_modem_profiles[app->encoding_index].name);
    cfgsave(app);
}

static void profile_change(VariableItem *item)
{
    FlipperHamApp *app = variable_item_get_context(item);

    app->rf_mod = variable_item_get_current_value_index(item);
    if (app->rf_mod > 1)
        app->rf_mod = 0;

    variable_item_set_current_value_text(item, app->rf_mod ? "GFSK" : "2FSK");
    preset_fix(app);
    cfgsave(app);
}

static void deviation_change(VariableItem *item)
{
    static const char *dl[] = {"1.6", "1.8", "2.0", "2.2", "2.4", "2.5", "2.8", "3.0", "5.0"};
    FlipperHamApp *app = variable_item_get_context(item);

    app->rf_dev = variable_item_get_current_value_index(item);
    if (app->rf_dev > 8)
        app->rf_dev = 8;

    variable_item_set_current_value_text(item, dl[app->rf_dev]);
    preset_fix(app);
    cfgsave(app);
}

static void repeat_change(VariableItem *item)
{
    FlipperHamApp *app = variable_item_get_context(item);
    char a[4];

    app->repeat_n = variable_item_get_current_value_index(item) + 1;
    snprintf(a, sizeof(a), "%u", app->repeat_n);
    variable_item_set_current_value_text(item, a);
    cfgsave(app);
}

static void leadin_change(VariableItem *item)
{
    FlipperHamApp *app = variable_item_get_context(item);
    char a[8];

    app->leadin_ms = variable_item_get_current_value_index(item) * 50;
    if (app->leadin_ms > 1000)
        app->leadin_ms = 1000;
    snprintf(a, sizeof(a), "%u", app->leadin_ms);
    variable_item_set_current_value_text(item, a);
    cfgsave(app);
}

static void preamble_change(VariableItem *item)
{
    FlipperHamApp *app = variable_item_get_context(item);
    char a[8];

    app->preamble_ms = variable_item_get_current_value_index(item) * 50;
    if (app->preamble_ms > 1000)
        app->preamble_ms = 1000;
    snprintf(a, sizeof(a), "%u", app->preamble_ms);
    variable_item_set_current_value_text(item, a);
    cfgsave(app);
}

static void freq_change(VariableItem *item)
{
    FlipperHamApp *app = variable_item_get_context(item);
    uint8_t a;

    app->f_bad = false;
    a = variable_item_get_current_value_index(item);

    while (a > 100)
    {
        app->freq_edit_hz = freq_step(app->freq_edit_hz, 1);
        a--;
    }

    while (a < 100)
    {
        app->freq_edit_hz = freq_step(app->freq_edit_hz, -1);
        a++;
    }

    fsh2(app->f_edit, sizeof(app->f_edit), app->freq_edit_hz);
    variable_item_set_current_value_text(item, app->f_edit);
    variable_item_set_current_value_index(item, 100);
}

static void ssid_enter(void *context, uint32_t index)
{
    FlipperHamApp *app = context;

    UNUSED(index);
    app->call_sel = FlipperHamCallIndexBase + app->dst_call_index;
    app->message_sel = FlipperHamMessageIndexBase + app->tx_msg_index;

    app->return_view = FlipperHamViewMessage;
    app->send_requested = true;
    view_dispatcher_stop(app->view_dispatcher);
}

static void settings_enter(void *context, uint32_t index)
{
    FlipperHamApp *app = context;

    if (index != FlipperHamSettingsIndexFreq)
        return;

    freq_menu_build(app);
    view_dispatcher_switch_to_view(app->view_dispatcher, FlipperHamViewFreq);
}

static void ham_enter(void *context, uint32_t index)
{
    FlipperHamApp *app = context;

    app->ham_sel = index;
    if (index == 0)
    {
        app->ham_tx_index = app->ham_index;
        ham_tx_menu_build(app);
        view_dispatcher_switch_to_view(app->view_dispatcher, FlipperHamViewHamTx);
        return;
    }

    if (index == 1)
    {
        ham_morse_play(app);
        ham_menu_build(app);
        view_dispatcher_switch_to_view(app->view_dispatcher, FlipperHamViewHam);
        return;
    }
}

static void ham_tx_enter(void *context, uint32_t index)
{
    FlipperHamApp *app = context;

    app->ham_tx_sel = index;
    if (index == 1)
    {
        app->ham_index = app->ham_tx_index;
        ham_save_txt(app);
        cfgsave(app);
        ham_menu_build(app);
        view_dispatcher_switch_to_view(app->view_dispatcher, FlipperHamViewHam);
        return;
    }
}

static void freq_edit_enter(void *context, uint32_t index)
{
    FlipperHamApp *app = context;
    bool a;

    a = false;
    if (app->freq_n > 1)
        if (app->freq_index < FREQ_N)
            if (app->freq_used[app->freq_index])
                a = true;

    if (index == 0)
    {
        app->freq_sel = FlipperHamFreqIndexBase + app->freq_index;
        app->f_bad = false;
        app->freq_edit_hz = freq_next_band(app->freq_edit_hz);
        freq_edit_menu_build(app);
        view_dispatcher_switch_to_view(app->view_dispatcher, FlipperHamViewFreqEdit);
        return;
    }

    if (index == 1)
    {
        app->freq_sel = FlipperHamFreqIndexBase + app->freq_index;
        app->f_edit[0] = 0;
        app->text_mode = 5;
        app->text_view = FlipperHamViewFreqEdit;
        text_input_reset(app->text_input);
        text_input_set_header_text(app->text_input, "Frequency");
        text_input_set_result_callback(app->text_input, freq_save, app, app->f_edit,
                                       sizeof(app->f_edit), false);
        view_dispatcher_switch_to_view(app->view_dispatcher, FlipperHamViewTextInput);
        return;
    }

    if (index == 2)
    {
        if (app->f_bad)
            return;
        if (app->freq_index >= FREQ_N)
            return;
        if (!furi_hal_subghz_is_frequency_valid(app->freq_edit_hz))
            return;

        app->freq_sel = FlipperHamFreqIndexBase + app->freq_index;
        app->freq[app->freq_index] = app->freq_edit_hz;
        app->freq_used[app->freq_index] = 1;
        freq_fix(app);
        cfgsave(app);
        freq_menu_build(app);
        submenu_set_selected_item(app->freq_menu, FlipperHamFreqIndexBase + app->freq_index);
        settings_menu_build(app);
        view_dispatcher_switch_to_view(app->view_dispatcher, FlipperHamViewFreq);
        return;
    }

    if (index == 3)
    {
        if (app->f_bad)
            return;
        if (app->freq_index >= FREQ_N)
            return;
        if (!furi_hal_subghz_is_frequency_valid(app->freq_edit_hz))
            return;

        app->freq_sel = FlipperHamFreqIndexBase + app->freq_index;
        app->freq[app->freq_index] = app->freq_edit_hz;
        app->freq_used[app->freq_index] = 1;
        app->tx_freq_index = app->freq_index;
        freq_fix(app);
        cfgsave(app);
        freq_menu_build(app);
        submenu_set_selected_item(app->freq_menu, FlipperHamFreqIndexBase + app->freq_index);
        settings_menu_build(app);
        view_dispatcher_switch_to_view(app->view_dispatcher, FlipperHamViewFreq);
        return;
    }

    if (a && index == 4)
    {
        app->freq_sel = FlipperHamFreqIndexAdd;
        app->freq[app->freq_index] = 0;
        app->freq_used[app->freq_index] = 0;
        freq_fix(app);
        cfgsave(app);
        freq_menu_build(app);
        settings_menu_build(app);
        view_dispatcher_switch_to_view(app->view_dispatcher, FlipperHamViewFreq);
        return;
    }
}

static void pos_edit_enter(void *context, uint32_t index)
{
    FlipperHamApp *app = context;
    bool a;

    a = false;
    if (app->pos_n > 1)
        if (app->pos_index < TXT_N)
            if (app->pos_used[app->pos_index])
                a = true;

    if (index == 0)
    {
        const char *title;

        title = "Name";
        if (app->position_sel >= FlipperHamPositionIndexBase)
            title = "Edit name";
        app->text_mode = 6;
        app->text_view = FlipperHamViewPosEdit;
        text_input_reset(app->text_input);
        text_input_set_header_text(app->text_input, title);
        text_input_set_result_callback(app->text_input, position_save, app, app->p_name_edit,
                                       sizeof(app->p_name_edit), false);
        view_dispatcher_switch_to_view(app->view_dispatcher, FlipperHamViewTextInput);
        return;
    }

    if (index == 1)
    {
        const char *title;

        title = "Latitude";
        if (app->position_sel >= FlipperHamPositionIndexBase)
            title = "Edit latitude";
        app->text_mode = 7;
        app->text_view = FlipperHamViewPosEdit;
        text_input_reset(app->text_input);
        text_input_set_header_text(app->text_input, title);
        text_input_set_result_callback(app->text_input, position_save, app, app->p_lat_edit,
                                       sizeof(app->p_lat_edit), false);
        view_dispatcher_switch_to_view(app->view_dispatcher, FlipperHamViewTextInput);
        return;
    }

    if (index == 2)
    {
        const char *title;

        title = "Longitude";
        if (app->position_sel >= FlipperHamPositionIndexBase)
            title = "Edit longitude";
        app->text_mode = 8;
        app->text_view = FlipperHamViewPosEdit;
        text_input_reset(app->text_input);
        text_input_set_header_text(app->text_input, title);
        text_input_set_result_callback(app->text_input, position_save, app, app->p_lon_edit,
                                       sizeof(app->p_lon_edit), false);
        view_dispatcher_switch_to_view(app->view_dispatcher, FlipperHamViewTextInput);
        return;
    }

    if (a && index == 3)
    {
        uint8_t i;

        app->pos_name[app->pos_index][0] = 0;
        app->pos_lat[app->pos_index][0] = 0;
        app->pos_lon[app->pos_index][0] = 0;
        app->pos_used[app->pos_index] = 0;
        app->position_sel = FlipperHamPositionIndexAdd;

        for (i = app->pos_index; i < TXT_N; i++)
            if (app->pos_used[i])
                if (app->pos_name[i][0])
                {
                    app->position_sel = FlipperHamPositionIndexBase + i;
                    break;
                }
        if (app->position_sel == FlipperHamPositionIndexAdd)
            for (i = app->pos_index; i > 0; i--)
                if (app->pos_used[i - 1])
                    if (app->pos_name[i - 1][0])
                    {
                        app->position_sel = FlipperHamPositionIndexBase + i - 1;
                        break;
                    }

        position_fix(app);
        cfgsave(app);
        position_menu_build(app);
        view_dispatcher_switch_to_view(app->view_dispatcher, FlipperHamViewPosition);
    }
}

static void freq_pick(void *context, InputType input_type, uint32_t index)
{
    FlipperHamApp *app = context;
    uint8_t a;

    if (index == FlipperHamFreqIndexAdd)
    {
        app->freq_sel = FlipperHamFreqIndexAdd;
        app->freq_index = 0xff;
        app->f_bad = false;

        for (a = 0; a < FREQ_N; a++)
            if (!app->freq_used[a])
            {
                app->freq_index = a;
                break;
            }

        if (app->freq_index == 0xff)
            return;

        app->freq_edit_hz = tx_freq_get(app);
        freq_edit_menu_build(app);
        view_dispatcher_switch_to_view(app->view_dispatcher, FlipperHamViewFreqEdit);
        return;
    }

    a = index - FlipperHamFreqIndexBase;
    if (a >= FREQ_N)
        return;
    if (!app->freq_used[a])
        return;

    app->freq_sel = index;

    if (input_type == InputTypeLong)
    {
        app->tx_freq_index = a;
        app->f_bad = false;
        cfgsave(app);
        freq_menu_build(app);
        settings_menu_build(app);
        submenu_set_selected_item(app->freq_menu, FlipperHamFreqIndexBase + a);
        return;
    }

    if (input_type != InputTypeShort)
        return;

    app->freq_index = a;
    app->freq_edit_hz = app->freq[a];
    app->f_bad = false;
    freq_edit_menu_build(app);
    view_dispatcher_switch_to_view(app->view_dispatcher, FlipperHamViewFreqEdit);
}

static void freq_save(void *context)
{
    FlipperHamApp *app = context;
    uint32_t a;

    if (freq_parse(app->f_edit, &a))
    {
        app->freq_edit_hz = a;
        app->f_bad = false;
    }
    else
        app->f_bad = true;

    freq_edit_menu_build(app);
    view_dispatcher_switch_to_view(app->view_dispatcher, FlipperHamViewFreqEdit);
}

static void m(void *context, uint32_t index)
{
    FlipperHamApp *app = context;
    uint8_t i;

    if (index == FlipperHamMessageIndexAdd)
    {
        app->message_sel = FlipperHamMessageIndexAdd;
        app->message_index = 0xff;

        for (i = 0; i < TXT_N; i++)
            if (!app->message_used[i] || !app->message[i][0])
            {
                app->message_index = i;
                break;
            }

        if (app->message_index == 0xff)
            return;

        app->m_edit[0] = 0;
    }
    else
    {
        i = index - FlipperHamMessageIndexBase;
        if (i >= TXT_N)
            return;

        app->message_sel = index;
        app->message_index = i;
        snprintf(app->m_edit, sizeof(app->m_edit), "%s", app->message[i]);
    }

    app->text_mode = 3;
    app->text_view = FlipperHamViewMessage;

    text_input_reset(app->text_input);
    text_input_set_header_text(app->text_input, "Message");
    text_input_set_result_callback(app->text_input, message_save, app, app->m_edit,
                                   sizeof(app->m_edit), false);

    view_dispatcher_switch_to_view(app->view_dispatcher, FlipperHamViewTextInput);
}

static void message_pick(void *context, InputType input_type, uint32_t index)
{
    FlipperHamApp *app = context;
    uint8_t i;

    i = index - FlipperHamMessageIndexBase;
    if (i >= TXT_N)
        return;

    if (input_type == InputTypeShort)
    {
        app->tx_type = 2;
        app->tx_msg_index = i;
        app->message_sel = index;
        if (app->dst_call_index < CALL_N)
            if (app->calls_used[app->dst_call_index])
                if (app->calls[app->dst_call_index][0])
                    app->call_sel = FlipperHamCallIndexBase + app->dst_call_index;
        call_menu_build(app);

        view_dispatcher_switch_to_view(app->view_dispatcher, FlipperHamViewCall);
        return;
    }

    if (input_type != InputTypeLong)
        return;

    app->message_sel = index;
    app->message_index = i;
    snprintf(app->m_edit, sizeof(app->m_edit), "%s", app->message[i]);
    app->text_mode = 3;
    app->text_view = FlipperHamViewMessage;
    text_input_reset(app->text_input);
    text_input_set_header_text(app->text_input, "Edit message");
    text_input_set_result_callback(app->text_input, message_save, app, app->m_edit,
                                   sizeof(app->m_edit), false);
    view_dispatcher_switch_to_view(app->view_dispatcher, FlipperHamViewTextInput);
}

static void message_save(void *context)
{
    FlipperHamApp *app = context;
    uint8_t i, j;

    i = app->message_index;
    if (i >= TXT_N)
        return;

    if (!app->m_edit[0])
    {
        app->message[i][0] = 0;
        app->message_used[i] = 0;
        app->message_sel = FlipperHamMessageIndexAdd;
        for (j = i; j < TXT_N; j++)
            if (app->message_used[j])
                if (app->message[j][0])
                {
                    app->message_sel = FlipperHamMessageIndexBase + j;
                    break;
                }
        if (app->message_sel == FlipperHamMessageIndexAdd)
            for (j = i; j > 0; j--)
                if (app->message_used[j - 1])
                    if (app->message[j - 1][0])
                    {
                        app->message_sel = FlipperHamMessageIndexBase + j - 1;
                        break;
                    }
    }
    else
    {
        snprintf(app->message[i], sizeof(app->message[i]), "%s", app->m_edit);
        app->message_used[i] = 1;
        app->message_sel = FlipperHamMessageIndexBase + i;
    }

    message_fix(app);
    cfgsave(app);
    message_menu_build(app);

    view_dispatcher_switch_to_view(app->view_dispatcher, FlipperHamViewMessage);
}

static void p(void *context, uint32_t index)
{
    FlipperHamApp *app = context;
    uint8_t i;

    if (index != FlipperHamPositionIndexAdd)
        return;

    app->position_sel = FlipperHamPositionIndexAdd;
    app->pos_index = 0xff;

    for (i = 0; i < TXT_N; i++)
        if (!app->pos_used[i] || !app->pos_name[i][0])
        {
            app->pos_index = i;
            break;
        }

    if (app->pos_index == 0xff)
        return;

    app->pos_name[app->pos_index][0] = 0;
    app->pos_lat[app->pos_index][0] = 0;
    app->pos_lon[app->pos_index][0] = 0;
    app->pos_used[app->pos_index] = 0;

    snprintf(app->p_name_edit, sizeof(app->p_name_edit), "Position");
    snprintf(app->p_lat_edit, sizeof(app->p_lat_edit), "0.00");
    snprintf(app->p_lon_edit, sizeof(app->p_lon_edit), "0.00");
    pos_edit_menu_build(app);
    view_dispatcher_switch_to_view(app->view_dispatcher, FlipperHamViewPosEdit);
}

static void position_pick(void *context, InputType input_type, uint32_t index)
{
    FlipperHamApp *app = context;
    uint8_t i;

    i = index - FlipperHamPositionIndexBase;
    if (i >= TXT_N)
        return;

    if (input_type == InputTypeShort)
    {
        app->position_sel = index;
        app->tx_type = 3;
        app->tx_msg_index = i;
        app->return_view = FlipperHamViewPosition;
        app->send_requested = true;
        view_dispatcher_stop(app->view_dispatcher);
        return;
    }

    if (input_type != InputTypeLong)
        return;

    app->position_sel = index;
    app->pos_index = i;
    snprintf(app->p_name_edit, sizeof(app->p_name_edit), "%s", app->pos_name[i]);
    snprintf(app->p_lat_edit, sizeof(app->p_lat_edit), "%s", app->pos_lat[i]);
    snprintf(app->p_lon_edit, sizeof(app->p_lon_edit), "%s", app->pos_lon[i]);
    pos_edit_menu_build(app);
    view_dispatcher_switch_to_view(app->view_dispatcher, FlipperHamViewPosEdit);
}

static void position_save(void *context)
{
    FlipperHamApp *app = context;
    uint8_t i, j;
    char a[POS_LEN];

    i = app->pos_index;
    if (i >= TXT_N)
        return;

    if (app->text_mode == 6)
        snprintf(app->pos_name[i], sizeof(app->pos_name[i]), "%s", app->p_name_edit);
    if (app->text_mode == 7)
    {
        if (aprs_ll_clamp(a, sizeof(a), app->p_lat_edit, 0))
            snprintf(app->p_lat_edit, sizeof(app->p_lat_edit), "%s", a);
        else if (app->pos_lat[i][0])
            snprintf(app->p_lat_edit, sizeof(app->p_lat_edit), "%s", app->pos_lat[i]);
        else
            snprintf(app->p_lat_edit, sizeof(app->p_lat_edit), "0.00000");
        snprintf(app->pos_lat[i], sizeof(app->pos_lat[i]), "%s", app->p_lat_edit);
    }
    if (app->text_mode == 8)
    {
        if (aprs_ll_clamp(a, sizeof(a), app->p_lon_edit, 1))
            snprintf(app->p_lon_edit, sizeof(app->p_lon_edit), "%s", a);
        else if (app->pos_lon[i][0])
            snprintf(app->p_lon_edit, sizeof(app->p_lon_edit), "%s", app->pos_lon[i]);
        else
            snprintf(app->p_lon_edit, sizeof(app->p_lon_edit), "0.00000");
        snprintf(app->pos_lon[i], sizeof(app->pos_lon[i]), "%s", app->p_lon_edit);
    }

    if (!app->pos_name[i][0])
    {
        app->pos_used[i] = 0;
        app->position_sel = FlipperHamPositionIndexAdd;
        for (j = i; j < TXT_N; j++)
            if (app->pos_used[j])
                if (app->pos_name[j][0])
                {
                    app->position_sel = FlipperHamPositionIndexBase + j;
                    break;
                }
        if (app->position_sel == FlipperHamPositionIndexAdd)
            for (j = i; j > 0; j--)
                if (app->pos_used[j - 1])
                    if (app->pos_name[j - 1][0])
                    {
                        app->position_sel = FlipperHamPositionIndexBase + j - 1;
                        break;
                    }
    }
    else
    {
        app->pos_used[i] = 1;
        app->position_sel = FlipperHamPositionIndexBase + i;
    }

    position_fix(app);
    cfgsave(app);
    position_menu_build(app);
    pos_edit_menu_build(app);
    view_dispatcher_switch_to_view(app->view_dispatcher, FlipperHamViewPosEdit);
}

static void flipperham_bulletin_callback(void *context, uint32_t index)
{
    FlipperHamApp *app = context;
    uint8_t i;

    if (index != FlipperHamBulletinIndexAdd)
        return;

    app->bulletin_sel = FlipperHamBulletinIndexAdd;
    app->bulletin_index = 0xff;

    for (i = 0; i < TXT_N; i++)
        if (!app->bulletin_used[i])
        {
            app->bulletin_index = i;
            break;
        }

    if (app->bulletin_index == 0xff)
        return;

    app->b_edit[0] = 0;
    app->text_mode = 0;
    app->text_view = FlipperHamViewBulletin;

    text_input_reset(app->text_input);
    text_input_set_header_text(app->text_input, "Bulletin");
    text_input_set_result_callback(app->text_input, bulletin_save, app, app->b_edit,
                                   sizeof(app->b_edit), false);

    view_dispatcher_switch_to_view(app->view_dispatcher, FlipperHamViewTextInput);
}

static void bulletin_pick(void *context, InputType input_type, uint32_t index)
{
    FlipperHamApp *app = context;
    uint8_t i;

    i = index - FlipperHamBulletinIndexBase;
    if (i >= TXT_N)
        return;

    if (input_type == InputTypeShort)
    {
        app->bulletin_sel = index;
        app->tx_type = 0;
        app->tx_msg_index = i;
        app->return_view = FlipperHamViewBulletin;
        app->send_requested = true;
        view_dispatcher_stop(app->view_dispatcher);
        return;
    }

    if (input_type != InputTypeLong)
        return;

    app->bulletin_sel = index;
    app->bulletin_index = i;
    snprintf(app->b_edit, sizeof(app->b_edit), "%s", app->bulletin[i]);
    app->text_mode = 0;
    app->text_view = FlipperHamViewBulletin;

    text_input_reset(app->text_input);
    text_input_set_header_text(app->text_input, "Edit bulletin");
    text_input_set_result_callback(app->text_input, bulletin_save, app, app->b_edit,
                                   sizeof(app->b_edit), false);

    view_dispatcher_switch_to_view(app->view_dispatcher, FlipperHamViewTextInput);
}

static void bulletin_save(void *context)
{
    FlipperHamApp *app = context;
    uint8_t i, j;

    i = app->bulletin_index;
    if (i >= TXT_N)
        return;

    if (!app->b_edit[0])
    {
        app->bulletin[i][0] = 0;
        app->bulletin_used[i] = 0;
        app->bulletin_sel = FlipperHamBulletinIndexAdd;
        for (j = i; j < TXT_N; j++)
            if (app->bulletin_used[j])
                if (app->bulletin[j][0])
                {
                    app->bulletin_sel = FlipperHamBulletinIndexBase + j;
                    break;
                }
        if (app->bulletin_sel == FlipperHamBulletinIndexAdd)
            for (j = i; j > 0; j--)
                if (app->bulletin_used[j - 1])
                    if (app->bulletin[j - 1][0])
                    {
                        app->bulletin_sel = FlipperHamBulletinIndexBase + j - 1;
                        break;
                    }
    }
    else
    {
        snprintf(app->bulletin[i], sizeof(app->bulletin[i]), "%s", app->b_edit);
        app->bulletin_used[i] = 1;
        app->bulletin_sel = FlipperHamBulletinIndexBase + i;
    }

    bulletin_fix(app);
    cfgsave(app);
    bulletin_menu_build(app);

    view_dispatcher_switch_to_view(app->view_dispatcher, FlipperHamViewBulletin);
}

static void st(void *context, uint32_t index)
{
    FlipperHamApp *app = context;
    uint8_t i;

    if (index != FlipperHamStatusIndexAdd)
        return;

    app->status_sel = FlipperHamStatusIndexAdd;
    app->status_index = 0xff;

    for (i = 0; i < TXT_N; i++)
        if (!app->status_used[i])
        {
            app->status_index = i;
            break;
        }

    if (app->status_index == 0xff)
        return;

    app->st_edit[0] = 0;
    app->text_mode = 1;
    app->text_view = FlipperHamViewStatus;

    text_input_reset(app->text_input);
    text_input_set_header_text(app->text_input, "Status");
    text_input_set_result_callback(app->text_input, status_save, app, app->st_edit,
                                   sizeof(app->st_edit), false);

    view_dispatcher_switch_to_view(app->view_dispatcher, FlipperHamViewTextInput);
}

static void status_pick(void *context, InputType input_type, uint32_t index)
{
    FlipperHamApp *app = context;
    uint8_t i;

    i = index - FlipperHamStatusIndexBase;
    if (i >= TXT_N)
        return;

    if (input_type == InputTypeShort)
    {
        app->status_sel = index;
        app->tx_type = 1;
        app->tx_msg_index = i;
        app->return_view = FlipperHamViewStatus;
        app->send_requested = true;
        view_dispatcher_stop(app->view_dispatcher);
        return;
    }

    if (input_type != InputTypeLong)
        return;

    app->status_sel = index;
    app->status_index = i;
    snprintf(app->st_edit, sizeof(app->st_edit), "%s", app->status[i]);
    app->text_mode = 1;
    app->text_view = FlipperHamViewStatus;

    text_input_reset(app->text_input);
    text_input_set_header_text(app->text_input, "Edit status");
    text_input_set_result_callback(app->text_input, status_save, app, app->st_edit,
                                   sizeof(app->st_edit), false);

    view_dispatcher_switch_to_view(app->view_dispatcher, FlipperHamViewTextInput);
}

static void status_save(void *context)
{
    FlipperHamApp *app = context;
    uint8_t i, j;

    i = app->status_index;
    if (i >= TXT_N)
        return;

    if (!app->st_edit[0])
    {
        app->status[i][0] = 0;
        app->status_used[i] = 0;
        app->status_sel = FlipperHamStatusIndexAdd;
        for (j = i; j < TXT_N; j++)
            if (app->status_used[j])
                if (app->status[j][0])
                {
                    app->status_sel = FlipperHamStatusIndexBase + j;
                    break;
                }
        if (app->status_sel == FlipperHamStatusIndexAdd)
            for (j = i; j > 0; j--)
                if (app->status_used[j - 1])
                    if (app->status[j - 1][0])
                    {
                        app->status_sel = FlipperHamStatusIndexBase + j - 1;
                        break;
                    }
    }
    else
    {
        snprintf(app->status[i], sizeof(app->status[i]), "%s", app->st_edit);
        app->status_used[i] = 1;
        app->status_sel = FlipperHamStatusIndexBase + i;
    }

    status_fix(app);
    cfgsave(app);
    status_menu_build(app);

    view_dispatcher_switch_to_view(app->view_dispatcher, FlipperHamViewStatus);
}

static void cl(void *context, uint32_t index)
{
    FlipperHamApp *app = context;
    uint8_t i;

    if (index == FlipperHamCallIndexAdd)
    {
        app->call_sel = FlipperHamCallIndexAdd;
        app->edit_call_index = 0xff;

        for (i = 0; i < CALL_N; i++)
            if (!app->calls_used[i] || !app->calls[i][0])
            {
                app->edit_call_index = i;
                break;
            }

        if (app->edit_call_index == 0xff)
            return;

        app->c_edit[0] = 0;
    }
    else
    {
        i = index - FlipperHamCallIndexBase;
        if (i >= CALL_N)
            return;

        app->call_sel = index;
        app->edit_call_index = i;
        snprintf(app->c_edit, sizeof(app->c_edit), "%s", app->calls[i]);
    }

    app->text_mode = 2;
    app->text_view = FlipperHamViewCall;

    text_input_reset(app->text_input);
    if (index == FlipperHamCallIndexAdd)
        text_input_set_header_text(app->text_input, "Callsign");
    else
        text_input_set_header_text(app->text_input, "Edit callsign");
    text_input_set_result_callback(app->text_input, call_save, app, app->c_edit,
                                   sizeof(app->c_edit), false);

    view_dispatcher_switch_to_view(app->view_dispatcher, FlipperHamViewTextInput);
}

static void cb(void *context, uint32_t index)
{
    FlipperHamApp *app = context;
    uint8_t i;

    if (index == FlipperHamBookIndexAdd)
    {
        app->book_sel = FlipperHamBookIndexAdd;
        app->edit_call_index = 0xff;

        for (i = 0; i < CALL_N; i++)
            if (!app->calls_used[i] || !app->calls[i][0])
            {
                app->edit_call_index = i;
                break;
            }

        if (app->edit_call_index == 0xff)
            return;

        app->c_edit[0] = 0;
        app->text_mode = 4;
        app->text_view = FlipperHamViewBook;
        text_input_reset(app->text_input);
        text_input_set_header_text(app->text_input, "Callsign");
        text_input_set_result_callback(app->text_input, call_save, app, app->c_edit,
                                       sizeof(app->c_edit), false);
        view_dispatcher_switch_to_view(app->view_dispatcher, FlipperHamViewTextInput);
        return;
    }

    index -= FlipperHamBookIndexBase;
    if (index >= CALL_N)
        return;

    app->book_sel = FlipperHamBookIndexBase + index;
    app->book_call_index = index;
    app->book_action_sel = FlipperHamC2IndexEdit;
    snprintf(app->c2_h, sizeof(app->c2_h), "%s", app->calls[index]);
    book_action_menu_build(app);
    view_dispatcher_switch_to_view(app->view_dispatcher, FlipperHamViewC2);
}

static void call_pick(void *context, InputType input_type, uint32_t index)
{
    FlipperHamApp *app = context;
    uint8_t i;
    uint8_t s;
    bool d;
    char a[CALL_LEN];

    i = index - FlipperHamCallIndexBase;
    if (i >= CALL_N)
        return;

    if (input_type == InputTypeShort)
    {
        app->call_sel = index;
        if (app->tx_type == 2)
        {
            app->dst_call_index = i;
            app->return_view = FlipperHamViewMessage;

            if (call_split(app->calls[i], a, &s, &d))
            {
                if (d)
                {
                    app->dst_ssid = s;
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

    if (input_type != InputTypeLong)
        return;

    app->call_sel = index;
    app->edit_call_index = i;
    snprintf(app->c_edit, sizeof(app->c_edit), "%s", app->calls[i]);
    app->text_mode = 2;
    app->text_view = FlipperHamViewCall;

    text_input_reset(app->text_input);
    text_input_set_header_text(app->text_input, "Edit callsign");
    text_input_set_result_callback(app->text_input, call_save, app, app->c_edit,
                                   sizeof(app->c_edit), false);

    view_dispatcher_switch_to_view(app->view_dispatcher, FlipperHamViewTextInput);
}

static void call_save(void *context)
{
    FlipperHamApp *app = context;
    uint8_t i, j;

    i = app->edit_call_index;
    if (i >= CALL_N)
        return;

    if (!app->c_edit[0])
    {
        app->calls[i][0] = 0;
        app->calls_used[i] = 0;
    }
    else
    {
        if (!call_validate(app->c_edit))
        {
            if (app->text_mode == 4)
                app->book_sel = FlipperHamBookIndexBase + i;
            else
                app->call_sel = FlipperHamCallIndexBase + i;
            if (app->text_mode == 4)
                view_dispatcher_switch_to_view(app->view_dispatcher, FlipperHamViewBook);
            else
                view_dispatcher_switch_to_view(app->view_dispatcher, FlipperHamViewCall);
            return;
        }

        snprintf(app->calls[i], sizeof(app->calls[i]), "%s", app->c_edit);
        app->calls_used[i] = 1;
        if (app->text_mode == 4)
            app->book_sel = FlipperHamBookIndexBase + i;
        else
            app->call_sel = FlipperHamCallIndexBase + i;
    }

    if (!app->c_edit[0])
    {
        if (app->text_mode == 4)
        {
            app->book_sel = FlipperHamBookIndexAdd;
            for (j = i; j < CALL_N; j++)
                if (app->calls_used[j])
                    if (app->calls[j][0])
                    {
                        app->book_sel = FlipperHamBookIndexBase + j;
                        break;
                    }
            if (app->book_sel == FlipperHamBookIndexAdd)
                for (j = i; j > 0; j--)
                    if (app->calls_used[j - 1])
                        if (app->calls[j - 1][0])
                        {
                            app->book_sel = FlipperHamBookIndexBase + j - 1;
                            break;
                        }
        }
        else
        {
            app->call_sel = FlipperHamCallIndexAdd;
            for (j = i; j < CALL_N; j++)
                if (app->calls_used[j])
                    if (app->calls[j][0])
                    {
                        app->call_sel = FlipperHamCallIndexBase + j;
                        break;
                    }
            if (app->call_sel == FlipperHamCallIndexAdd)
                for (j = i; j > 0; j--)
                    if (app->calls_used[j - 1])
                        if (app->calls[j - 1][0])
                        {
                            app->call_sel = FlipperHamCallIndexBase + j - 1;
                            break;
                        }
        }
    }

    calls_fix(app);
    cfgsave(app);
    callbook_save_txt(app);
    call_menu_build(app);
    book_menu_build(app);

    if (app->text_mode == 4)
        view_dispatcher_switch_to_view(app->view_dispatcher, FlipperHamViewBook);
    else
        view_dispatcher_switch_to_view(app->view_dispatcher, FlipperHamViewCall);
}

static void c2(void *context, uint32_t index)
{
    FlipperHamApp *app = context;

    if (app->book_call_index >= CALL_N)
        return;

    app->book_action_sel = index;

    if (index == FlipperHamC2IndexEdit)
    {
        app->edit_call_index = app->book_call_index;
        snprintf(app->c_edit, sizeof(app->c_edit), "%s", app->calls[app->book_call_index]);
        app->text_mode = 4;
        app->text_view = FlipperHamViewC2;
        text_input_reset(app->text_input);
        text_input_set_header_text(app->text_input, "Edit callsign");
        text_input_set_result_callback(app->text_input, call_save, app, app->c_edit,
                                       sizeof(app->c_edit), false);
        view_dispatcher_switch_to_view(app->view_dispatcher, FlipperHamViewTextInput);
        return;
    }

    if (index == FlipperHamC2IndexDelete)
    {
        app->calls[app->book_call_index][0] = 0;
        app->calls_used[app->book_call_index] = 0;
        app->book_sel = FlipperHamBookIndexAdd;
        if (app->book_call_index + 1 < CALL_N)
        {
            uint8_t i;
            for (i = app->book_call_index + 1; i < CALL_N; i++)
                if (app->calls_used[i])
                    if (app->calls[i][0])
                    {
                        app->book_sel = FlipperHamBookIndexBase + i;
                        break;
                    }
        }
        if (app->book_sel == FlipperHamBookIndexAdd)
        {
            uint8_t i;
            for (i = app->book_call_index; i > 0; i--)
                if (app->calls_used[i - 1])
                    if (app->calls[i - 1][0])
                    {
                        app->book_sel = FlipperHamBookIndexBase + i - 1;
                        break;
                    }
        }
        calls_fix(app);
        cfgsave(app);
        callbook_save_txt(app);
        call_menu_build(app);
        book_menu_build(app);
        view_dispatcher_switch_to_view(app->view_dispatcher, FlipperHamViewBook);
        return;
    }

    if (index != FlipperHamC2IndexCopy)
        return;

    if (call_copy(app))
    {
        view_dispatcher_switch_to_view(app->view_dispatcher, FlipperHamViewBook);
        return;
    }

    snprintf(app->c2_h, sizeof(app->c2_h), "No SSID free");
    book_action_menu_build(app);
    view_dispatcher_switch_to_view(app->view_dispatcher, FlipperHamViewC2);
}

static void flipperham_status_view_alloc(FlipperHamApp *app)
{
    app->view_port = view_port_alloc();
    view_port_draw_callback_set(app->view_port, flipperham_draw_callback, app);
    view_port_input_callback_set(app->view_port, stin, app);
    gui_add_view_port(app->gui, app->view_port, GuiLayerFullscreen);
}

static void flipperham_status_view_free(FlipperHamApp *app)
{
    if (!app->view_port)
    {
        return;
    }

    gui_remove_view_port(app->gui, app->view_port);
    view_port_free(app->view_port);
    app->view_port = NULL;
}

static void flipperham_menu_free(FlipperHamApp *app)
{
    if (app->view_dispatcher)
    {
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
        view_dispatcher_remove_view(app->view_dispatcher, FlipperHamViewReadme);
        view_dispatcher_free(app->view_dispatcher);
        app->view_dispatcher = NULL;
    }

    if (app->submenu)
    {
        submenu_free(app->submenu);
        app->submenu = NULL;
    }

    if (app->send_menu)
    {
        submenu_free(app->send_menu);
        app->send_menu = NULL;
    }

    if (app->settings_menu)
    {
        variable_item_list_free(app->settings_menu);
        app->settings_menu = NULL;
    }

    if (app->ham_menu)
    {
        variable_item_list_free(app->ham_menu);
        app->ham_menu = NULL;
    }

    if (app->ham_tx_menu)
    {
        variable_item_list_free(app->ham_tx_menu);
        app->ham_tx_menu = NULL;
    }

    if (app->bulletin_menu)
    {
        submenu_free(app->bulletin_menu);
        app->bulletin_menu = NULL;
    }

    if (app->status_menu)
    {
        submenu_free(app->status_menu);
        app->status_menu = NULL;
    }

    if (app->message_menu)
    {
        submenu_free(app->message_menu);
        app->message_menu = NULL;
    }

    if (app->position_menu)
    {
        submenu_free(app->position_menu);
        app->position_menu = NULL;
    }

    if (app->call_menu)
    {
        submenu_free(app->call_menu);
        app->call_menu = NULL;
    }

    if (app->book_menu)
    {
        submenu_free(app->book_menu);
        app->book_menu = NULL;
    }

    if (app->readme_widget)
    {
        widget_free(app->readme_widget);
        app->readme_widget = NULL;
    }

    if (app->freq_menu)
    {
        submenu_free(app->freq_menu);
        app->freq_menu = NULL;
    }

    if (app->c2_menu)
    {
        submenu_free(app->c2_menu);
        app->c2_menu = NULL;
    }

    if (app->freq_edit_menu)
    {
        variable_item_list_free(app->freq_edit_menu);
        app->freq_edit_menu = NULL;
    }

    if (app->pos_edit_menu)
    {
        variable_item_list_free(app->pos_edit_menu);
        app->pos_edit_menu = NULL;
    }

    if (app->ssid_menu)
    {
        variable_item_list_free(app->ssid_menu);
        app->ssid_menu = NULL;
    }

    if (app->text_input)
    {
        text_input_free(app->text_input);
        app->text_input = NULL;
    }
}

static void stin(InputEvent *event, void *context)
{
    FlipperHamApp *app = context;

    if (event->type != InputTypeShort)
        return;
    if (event->key != InputKeyBack)
        return;
    if (!app->repeat_wait)
        return;

    app->repeat_cancel = true;
}

static void tx_blink_green(void)
{
    furi_hal_light_blink_stop();
    furi_hal_light_set(LightBlue, 0);
    furi_hal_light_set(LightRed, 255);
    furi_hal_light_set(LightGreen, 72);
}

static uint32_t repeat_scale(FlipperHamApp *app)
{
    if (app->encoding_index == 0)
        return 4;
    return 1;
}

static FlipperHamApp *flipperham_app_alloc(void)
{
    FlipperHamApp *app = malloc(sizeof(FlipperHamApp));

    gapp = app;
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
    app->readme_widget = widget_alloc();

    app->view_port = NULL;
    app->tx_started = false;
    app->tx_allowed = true;
    app->tx_done = false;
    app->show_done = false;
    app->send_requested = false;
    app->ham_ok = false;
    app->encoding_index = FlipperHamModemProfileDefault;
    app->ham_n = 0;
    app->repeat_n = 1;
    app->leadin_ms = 50;
    app->preamble_ms = 350;
    app->repeat_i = 1;
    app->repeat_wait = false;
    app->repeat_cancel = false;
    app->tx_msg_index = 0;
    app->tx_type = 0;
    memset(app->ham_ssid, 0, sizeof(app->ham_ssid));
    memset(app->ham_has_ssid, 0, sizeof(app->ham_has_ssid));
    memset(app->ham_pass, 0, sizeof(app->ham_pass));
    memset(app->ham_calls, 0, sizeof(app->ham_calls));
    app->status_index = 0;
    app->message_index = 0;
    app->pos_index = 0;
    app->dst_call_index = 0;
    app->dst_ssid = 0;
    app->edit_call_index = 0;
    app->book_call_index = 0;
    app->ham_index = 0;
    app->ham_tx_index = 0;
    app->freq_index = 0;
    app->bulletin_sel = FlipperHamBulletinIndexAdd;
    app->status_sel = FlipperHamStatusIndexAdd;
    app->message_sel = FlipperHamMessageIndexAdd;
    app->position_sel = FlipperHamPositionIndexAdd;
    app->call_sel = FlipperHamCallIndexAdd;
    app->book_sel = FlipperHamBookIndexAdd;
    app->book_action_sel = FlipperHamC2IndexEdit;
    app->freq_sel = FlipperHamFreqIndexAdd;
    app->ham_sel = 0;
    app->ham_tx_sel = 0;
    app->c2_h[0] = 0;
    app->f_edit[0] = 0;
    app->f_bad = false;
    app->return_view = FlipperHamViewMenu;
    app->text_mode = 0;
    app->text_view = FlipperHamViewMenu;
    app->pkt = NULL;
    app->wave = NULL;

    cfgload(app);

    view_dispatcher_enable_queue(app->view_dispatcher);
    view_dispatcher_attach_to_gui(app->view_dispatcher, app->gui, ViewDispatcherTypeFullscreen);

    submenu_add_item(app->submenu, "Send", FlipperHamMenuIndexSend, flipperham_menu_callback, app);
    submenu_add_item(app->submenu, "Settings", FlipperHamMenuIndexSettings,
                     flipperham_menu_callback, app);
    submenu_add_item(app->submenu, "Callbook", FlipperHamMenuIndexCallbook,
                     flipperham_menu_callback, app);
    if (app->ham_ok)
        submenu_add_item(app->submenu, "Ham Radio", FlipperHamMenuIndexHam,
                         flipperham_menu_callback, app);
    submenu_add_item(app->submenu, "About Flipper ham", FlipperHamMenuIndexReadme,
                     flipperham_menu_callback, app);

    submenu_add_item(app->send_menu, "Message", FlipperHamSendIndexMessage,
                     flipperham_send_callback, app);
    submenu_add_item(app->send_menu, "Position", FlipperHamSendIndexPosition,
                     flipperham_send_callback, app);
    submenu_add_item(app->send_menu, "Status", FlipperHamSendIndexStatus, flipperham_send_callback,
                     app);
    submenu_add_item(app->send_menu, "Bulletin", FlipperHamSendIndexBulletin,
                     flipperham_send_callback, app);

    bulletin_menu_build(app);
    status_menu_build(app);
    message_menu_build(app);
    position_menu_build(app);
    call_menu_build(app);
    book_menu_build(app);
    freq_menu_build(app);
    settings_menu_build(app);
    ssidfix(app);
    ham_menu_build(app);
    ham_tx_menu_build(app);
    widget_add_text_scroll_element(
        app->readme_widget, 0, 0, 128, 52,
        "www.yo3gnd.ro\n\nAPRS experimental transmiter for Flipper. Don't transmit where you "
        "shouldn't. Uses FSK as a weak substitute for FM. Works, sometimes.\n\nI'm quite "
        "interested on what kind of hardware and with what parameters you got decodes - reports "
        "are appreciated");
    widget_add_button_element(app->readme_widget, GuiButtonTypeLeft, "Back", readme_back, app);

    view_set_previous_callback(submenu_get_view(app->submenu), flipperham_exit_callback);
    view_set_previous_callback(submenu_get_view(app->send_menu), flipperham_send_exit_callback);
    view_set_previous_callback(variable_item_list_get_view(app->settings_menu),
                               flipperham_settings_exit_callback);
    view_set_previous_callback(submenu_get_view(app->bulletin_menu),
                               flipperham_bulletin_exit_callback);
    view_set_previous_callback(submenu_get_view(app->status_menu), flipperham_status_exit_callback);
    view_set_previous_callback(submenu_get_view(app->message_menu),
                               flipperham_message_exit_callback);
    view_set_previous_callback(submenu_get_view(app->position_menu),
                               flipperham_position_exit_callback);
    view_set_previous_callback(submenu_get_view(app->call_menu), flipperham_call_exit_callback);
    view_set_previous_callback(submenu_get_view(app->book_menu), book_exit);
    view_set_previous_callback(submenu_get_view(app->c2_menu), book_action_exit);
    view_set_previous_callback(submenu_get_view(app->freq_menu), flipperham_freq_exit_callback);
    view_set_previous_callback(variable_item_list_get_view(app->freq_edit_menu),
                               flipperham_freq_edit_exit_callback);
    view_set_previous_callback(variable_item_list_get_view(app->pos_edit_menu),
                               flipperham_pos_edit_exit_callback);
    view_set_previous_callback(variable_item_list_get_view(app->ssid_menu),
                               flipperham_ssid_exit_callback);
    view_set_previous_callback(variable_item_list_get_view(app->ham_menu),
                               flipperham_ham_exit_callback);
    view_set_previous_callback(variable_item_list_get_view(app->ham_tx_menu),
                               flipperham_ham_tx_exit_callback);
    view_set_previous_callback(text_input_get_view(app->text_input), flipperham_text_exit_callback);
    view_set_previous_callback(widget_get_view(app->readme_widget),
                               flipperham_readme_exit_callback);
    variable_item_list_set_enter_callback(app->ssid_menu, ssid_enter, app);
    variable_item_list_set_enter_callback(app->settings_menu, settings_enter, app);
    variable_item_list_set_enter_callback(app->ham_menu, ham_enter, app);
    variable_item_list_set_enter_callback(app->ham_tx_menu, ham_tx_enter, app);
    variable_item_list_set_enter_callback(app->freq_edit_menu, freq_edit_enter, app);
    variable_item_list_set_enter_callback(app->pos_edit_menu, pos_edit_enter, app);

    view_dispatcher_add_view(app->view_dispatcher, FlipperHamViewMenu,
                             submenu_get_view(app->submenu));
    view_dispatcher_add_view(app->view_dispatcher, FlipperHamViewSend,
                             submenu_get_view(app->send_menu));
    view_dispatcher_add_view(app->view_dispatcher, FlipperHamViewSettings,
                             variable_item_list_get_view(app->settings_menu));
    view_dispatcher_add_view(app->view_dispatcher, FlipperHamViewBulletin,
                             submenu_get_view(app->bulletin_menu));
    view_dispatcher_add_view(app->view_dispatcher, FlipperHamViewStatus,
                             submenu_get_view(app->status_menu));
    view_dispatcher_add_view(app->view_dispatcher, FlipperHamViewMessage,
                             submenu_get_view(app->message_menu));
    view_dispatcher_add_view(app->view_dispatcher, FlipperHamViewPosition,
                             submenu_get_view(app->position_menu));
    view_dispatcher_add_view(app->view_dispatcher, FlipperHamViewCall,
                             submenu_get_view(app->call_menu));
    view_dispatcher_add_view(app->view_dispatcher, FlipperHamViewBook,
                             submenu_get_view(app->book_menu));
    view_dispatcher_add_view(app->view_dispatcher, FlipperHamViewC2,
                             submenu_get_view(app->c2_menu));
    view_dispatcher_add_view(app->view_dispatcher, FlipperHamViewFreq,
                             submenu_get_view(app->freq_menu));
    view_dispatcher_add_view(app->view_dispatcher, FlipperHamViewFreqEdit,
                             variable_item_list_get_view(app->freq_edit_menu));
    view_dispatcher_add_view(app->view_dispatcher, FlipperHamViewPosEdit,
                             variable_item_list_get_view(app->pos_edit_menu));
    view_dispatcher_add_view(app->view_dispatcher, FlipperHamViewSsid,
                             variable_item_list_get_view(app->ssid_menu));
    view_dispatcher_add_view(app->view_dispatcher, FlipperHamViewHam,
                             variable_item_list_get_view(app->ham_menu));
    view_dispatcher_add_view(app->view_dispatcher, FlipperHamViewHamTx,
                             variable_item_list_get_view(app->ham_tx_menu));
    view_dispatcher_add_view(app->view_dispatcher, FlipperHamViewTextInput,
                             text_input_get_view(app->text_input));
    view_dispatcher_add_view(app->view_dispatcher, FlipperHamViewReadme,
                             widget_get_view(app->readme_widget));
    view_dispatcher_switch_to_view(app->view_dispatcher, FlipperHamViewMenu);
    return app;
}

static void flipperham_app_free(FlipperHamApp *app)
{
    if (!app)
        return;

    if (gapp == app)
        gapp = NULL;
    flipperham_status_view_free(app);
    flipperham_menu_free(app);
    if (app->pkt)
        free(app->pkt);
    if (app->wave)
        free(app->wave);
    if (app->gui)
        furi_record_close(RECORD_GUI);
    free(app);
}

static void flipperham_send_hardcoded_message(FlipperHamApp *app)
{
    static const uint32_t a[] = {0, 2000, 4000, 8000, 15000};
    uint8_t i;
    uint32_t b, c;

    if (!app->pkt)
        app->pkt = malloc(sizeof(Packet));
    if (!app->wave)
        app->wave = malloc(sizeof(uint16_t) * WAVE_N);
    if (!app->pkt || !app->wave)
    {
        if (app->pkt)
        {
            free(app->pkt);
            app->pkt = NULL;
        }
        if (app->wave)
        {
            free(app->wave);
            app->wave = NULL;
        }
        return;
    }

    flipperham_status_view_alloc(app);
    app->repeat_i = 0;
    app->repeat_t0 = furi_get_tick();
    app->repeat_to = 0;
    app->repeat_wait = false;
    app->repeat_cancel = false;
    app->show_done = false;
    furi_hal_light_blink_stop();
    furi_hal_light_set(LightBlue, 0);
    furi_hal_light_set(LightRed, 0);
    furi_hal_light_set(LightGreen, 0);
    c = repeat_scale(app);
    furi_hal_power_suppress_charge_enter();

    for (i = 0; i < app->repeat_n; i++)
    {
        app->repeat_i = i + 1;

        txstart(app);
        if (!app->tx_ok)
        {
            furi_hal_power_suppress_charge_exit();
            furi_hal_light_blink_stop();
            furi_hal_light_set(LightBlue, 0);
            furi_hal_light_set(LightRed, 0);
            furi_hal_light_set(LightGreen, 0);
            flipperham_status_view_free(app);
            free(app->pkt);
            free(app->wave);
            app->pkt = NULL;
            app->wave = NULL;
            return;
        }
        app->tx_started = false;
        app->tx_allowed = true;
        app->repeat_wait = false;
        app->show_done = false;

        tx_blink_green();
        view_port_update(app->view_port);
        furi_delay_ms(100);

        flipperham_radio_start(app);

        while (!app->tx_done)
        {
            view_port_update(app->view_port);
            furi_delay_ms(50);
        }

        while (app->tx_started && !furi_hal_subghz_is_async_tx_complete())
        {
            view_port_update(app->view_port);
            furi_delay_ms(20);
        }

        flipperham_radio_stop(app);
        furi_hal_light_blink_stop();
        furi_hal_light_set(LightBlue, 0);
        furi_hal_light_set(LightRed, 0);
        furi_hal_light_set(LightGreen, 0);

        if (i + 1 >= app->repeat_n)
            break;

        app->repeat_to = a[i + 1] * c;
        app->repeat_wait = true;
        app->tx_done = false;

        while (1)
        {
            if (app->repeat_cancel)
                break;

            b = furi_get_tick() - app->repeat_t0;
            if (b >= app->repeat_to)
                break;

            view_port_update(app->view_port);
            furi_delay_ms(50 * c);
        }

        app->repeat_wait = false;
        if (app->repeat_cancel)
            break;
    }

    app->repeat_wait = false;
    app->repeat_cancel = false;
    app->show_done = true;
    app->tx_done = true;
    view_port_update(app->view_port);
    furi_delay_ms(750);
    furi_hal_power_suppress_charge_exit();
    furi_hal_light_blink_stop();
    furi_hal_light_set(LightBlue, 0);
    furi_hal_light_set(LightRed, 0);
    furi_hal_light_set(LightGreen, 0);

    flipperham_status_view_free(app);
    free(app->pkt);
    free(app->wave);
    app->pkt = NULL;
    app->wave = NULL;
}

int32_t flipperham_app(void *p)
{
    UNUSED(p);

    FlipperHamApp *app = flipperham_app_alloc();

    while (1)
    {
        app->send_requested = false;
        view_dispatcher_switch_to_view(app->view_dispatcher, app->return_view);
        view_dispatcher_run(app->view_dispatcher);

        if (!app->send_requested)
            break;

        flipperham_send_hardcoded_message(app);
    }

    flipperham_app_free(app);
    return 0;
}
