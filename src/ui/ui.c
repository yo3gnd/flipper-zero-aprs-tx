#include "ui_i.h"

#include <furi_hal.h>
#include <gui/view.h>
#include <notification/notification.h>
#include <notification/notification_messages.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void ham_blank_draw(Canvas *canvas, void *context);
static void ham_blank_input(InputEvent *event, void *context);
static void ham_morse_play(FlipperHamApp *app);
static void c2(void *context, uint32_t index);
FlipperHamApp *gapp;
static bool call_copy(FlipperHamApp *app);

void flipperham_draw_callback(Canvas *canvas, void *context)
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

uint32_t flipperham_exit_callback(void *context)
{
    UNUSED(context);
    return VIEW_NONE;
}

uint32_t flipperham_send_exit_callback(void *context)
{
    UNUSED(context);

    return FlipperHamViewMenu;
}

uint32_t flipperham_settings_exit_callback(void *context)
{
    UNUSED(context);

    return FlipperHamViewMenu;
}

uint32_t flipperham_bulletin_exit_callback(void *context)
{
    UNUSED(context);

    return FlipperHamViewSend;
}

uint32_t flipperham_status_exit_callback(void *context)
{
    UNUSED(context);

    return FlipperHamViewSend;
}

uint32_t flipperham_message_exit_callback(void *context)
{
    UNUSED(context);

    return FlipperHamViewSend;
}

uint32_t flipperham_position_exit_callback(void *context)
{
    UNUSED(context);

    return FlipperHamViewSend;
}

uint32_t flipperham_ssid_exit_callback(void *context)
{
    UNUSED(context);

    return FlipperHamViewCall;
}

uint32_t flipperham_call_exit_callback(void *context)
{
    FlipperHamApp *app = gapp;

    UNUSED(context);
    if (!app)
        return FlipperHamViewSend;
    if (app->tx_type == 2)
        return FlipperHamViewMessage;
    return FlipperHamViewSend;
}

uint32_t flipperham_freq_exit_callback(void *context)
{
    UNUSED(context);

    return FlipperHamViewSettings;
}

uint32_t flipperham_freq_edit_exit_callback(void *context)
{
    UNUSED(context);

    return FlipperHamViewFreq;
}

uint32_t flipperham_pos_edit_exit_callback(void *context)
{
    UNUSED(context);

    return FlipperHamViewPosition;
}

uint32_t flipperham_ham_exit_callback(void *context)
{
    UNUSED(context);

    return FlipperHamViewMenu;
}

uint32_t flipperham_ham_tx_exit_callback(void *context)
{
    UNUSED(context);

    return FlipperHamViewHam;
}

uint32_t flipperham_readme_exit_callback(void *context)
{
    UNUSED(context);

    return FlipperHamViewMenu;
}

uint32_t book_exit(void *context)
{
    UNUSED(context);

    return FlipperHamViewMenu;
}

uint32_t book_action_exit(void *context)
{
    UNUSED(context);

    return FlipperHamViewBook;
}

uint32_t flipperham_text_exit_callback(void *context)
{
    FlipperHamApp *app = gapp;

    UNUSED(context);
    if (!app)
        return FlipperHamViewMenu;
    return app->text_view;
}

void flipperham_menu_callback(void *context, uint32_t index)
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
    {
        app->splash_next_view = FlipperHamViewReadme;
        app->splash_back_exit = false;
        view_dispatcher_switch_to_view(app->view_dispatcher, FlipperHamViewSplash);
    }
}

void readme_back(GuiButtonType result, InputType type, void *context)
{
    FlipperHamApp *app = context;

    if (type != InputTypeShort)
        return;
    if (result != GuiButtonTypeLeft)
        return;
    view_dispatcher_switch_to_view(app->view_dispatcher, FlipperHamViewMenu);
}

void flipperham_send_callback(void *context, uint32_t index)
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

void bulletin_menu_build(FlipperHamApp *app)
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

void status_menu_build(FlipperHamApp *app)
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

void call_menu_build(FlipperHamApp *app)
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

void book_menu_build(FlipperHamApp *app)
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

void book_action_menu_build(FlipperHamApp *app)
{
    submenu_reset(app->c2_menu);
    submenu_set_header(app->c2_menu, app->c2_h);
    submenu_add_item(app->c2_menu, "Edit", FlipperHamC2IndexEdit, c2, app);
    submenu_add_item(app->c2_menu, "Delete", FlipperHamC2IndexDelete, c2, app);
    submenu_add_item(app->c2_menu, "Copy", FlipperHamC2IndexCopy, c2, app);
    submenu_set_selected_item(app->c2_menu, app->book_action_sel);
}

void ssid_menu_build(FlipperHamApp *app)
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

void ham_menu_build(FlipperHamApp *app)
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

void ham_tx_menu_build(FlipperHamApp *app)
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

void settings_menu_build(FlipperHamApp *app)
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

void message_menu_build(FlipperHamApp *app)
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

void position_menu_build(FlipperHamApp *app)
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

void pos_edit_menu_build(FlipperHamApp *app)
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

void ssidfix(FlipperHamApp *app)
{
    if (app->dst_ssid > 15)
        app->dst_ssid = 0;
    ssid_menu_build(app);
}

void ssid_change(VariableItem *item)
{
    FlipperHamApp *app = variable_item_get_context(item);
    char a[4];

    app->dst_ssid = variable_item_get_current_value_index(item);
    snprintf(a, sizeof(a), "%u", app->dst_ssid);
    variable_item_set_current_value_text(item, a);
}

void ham_call_change(VariableItem *item)
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

void ham_ssid_change(VariableItem *item)
{
    FlipperHamApp *app = variable_item_get_context(item);
    char a[4];

    app->ham_ssid[app->ham_tx_index] = variable_item_get_current_value_index(item);
    app->ham_has_ssid[app->ham_tx_index] = true;
    snprintf(a, sizeof(a), "%u", app->ham_ssid[app->ham_tx_index]);
    variable_item_set_current_value_text(item, a);
    ham_save_txt(app);
}

void baud_change(VariableItem *item)
{
    FlipperHamApp *app = variable_item_get_context(item);

    app->encoding_index = variable_item_get_current_value_index(item);
    if (app->encoding_index >=
        sizeof(flipperham_modem_profiles) / sizeof(flipperham_modem_profiles[0]))
        app->encoding_index = FlipperHamModemProfileDefault;

    variable_item_set_current_value_text(item, flipperham_modem_profiles[app->encoding_index].name);
    cfgsave(app);
}

void profile_change(VariableItem *item)
{
    FlipperHamApp *app = variable_item_get_context(item);

    app->rf_mod = variable_item_get_current_value_index(item);
    if (app->rf_mod > 1)
        app->rf_mod = 0;

    variable_item_set_current_value_text(item, app->rf_mod ? "GFSK" : "2FSK");
    preset_fix(app);
    cfgsave(app);
}

void deviation_change(VariableItem *item)
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

void repeat_change(VariableItem *item)
{
    FlipperHamApp *app = variable_item_get_context(item);
    char a[4];

    app->repeat_n = variable_item_get_current_value_index(item) + 1;
    snprintf(a, sizeof(a), "%u", app->repeat_n);
    variable_item_set_current_value_text(item, a);
    cfgsave(app);
}

void leadin_change(VariableItem *item)
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

void preamble_change(VariableItem *item)
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

void freq_change(VariableItem *item)
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

void ssid_enter(void *context, uint32_t index)
{
    FlipperHamApp *app = context;

    UNUSED(index);
    app->call_sel = FlipperHamCallIndexBase + app->dst_call_index;
    app->message_sel = FlipperHamMessageIndexBase + app->tx_msg_index;

    app->return_view = FlipperHamViewMessage;
    app->send_requested = true;
    view_dispatcher_stop(app->view_dispatcher);
}

void settings_enter(void *context, uint32_t index)
{
    FlipperHamApp *app = context;

    if (index != FlipperHamSettingsIndexFreq)
        return;

    freq_menu_build(app);
    view_dispatcher_switch_to_view(app->view_dispatcher, FlipperHamViewFreq);
}

void ham_enter(void *context, uint32_t index)
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

void ham_tx_enter(void *context, uint32_t index)
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

void pos_edit_enter(void *context, uint32_t index)
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
