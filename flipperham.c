#include "flipperham.h"

#include <furi_hal.h>
#include <gui/gui.h>
#include <gui/modules/submenu.h>
#include <gui/view.h>
#include <gui/view_dispatcher.h>
#include <gui/view_port.h>
#include <cc1101_regs.h>
#include <furi_hal_subghz_configs.h>
#include <lib/toolbox/level_duration.h>

#include <stdio.h>

#define CARRIER_HZ 431000000UL

#include "tools/3_aprs_packet.txt"

#define SEG packet3_aprs_packet_durations_count

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

enum {
    FlipperHamPresetDefault = 0,
    FlipperHamModemProfileDefault = 0,
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

typedef struct {
    Gui* gui;
    ViewDispatcher* view_dispatcher;
    Submenu* submenu;
    ViewPort* view_port;
    volatile uint16_t current_tone_hz;
    volatile uint16_t current_half_period_us;
    volatile uint16_t segment_index;
    volatile bool level;
    volatile bool tx_started;
    volatile bool tx_done;
    volatile bool tx_allowed;
    bool send_requested;
} FlipperHamApp;

enum {
    FlipperHamViewSubmenu = 0,
};

enum {
    FlipperHamSubmenuIndexSend = 0,
};

static const FlipperHamPreset flipperham_presets[] = {
    { "2FSK 5.16", flipperham_preset_2fsk_dev5_16khz_async_regs },
    { "GFSK 4.8", flipperham_preset_gfsk_dev5_16khz_async_regs },
    { "GFSK 9.99", flipperham_preset_gfsk_dev5_16khz_fast_async_regs },
};

static const FlipperHamModemProfile flipperham_modem_profiles[] = {
    { "300bd", 300, 1600, 1800 },
    { "1200bd", 1200, 1200, 2200 },
};

static const FlipperHamPreset* flipperham_preset = &flipperham_presets[FlipperHamPresetDefault];
static const FlipperHamModemProfile* flipperham_modem_profile =
    &flipperham_modem_profiles[FlipperHamModemProfileDefault];

static uint32_t flipperham_exit_callback(void* context) 
{
    UNUSED(context);
    return VIEW_NONE;
}

static void flipperham_submenu_callback(void* context, uint32_t index) 
{
    FlipperHamApp* app = context;

    if(index != FlipperHamSubmenuIndexSend) {
        return;
    }

    app->send_requested = true;
    view_dispatcher_stop(app->view_dispatcher);
}

static void flipperham_draw_callback(Canvas* canvas, void* context) 
{
    FlipperHamApp* app = context;
    char text[32];

    canvas_clear(canvas);
    canvas_set_font(canvas, FontPrimary);

    if(!app->tx_allowed) 
    {
        canvas_draw_str_aligned(canvas, 64, 32, AlignCenter, AlignCenter, "TX blocked");
        return;
    }



    if(app->tx_done) {
        canvas_draw_str_aligned(canvas, 64, 32, AlignCenter, AlignCenter, "Done");
        return;
    }

    if(!app->tx_started) {
        canvas_draw_str_aligned(
            canvas,
            64,
            24,
            AlignCenter,
            AlignCenter,
            flipperham_preset->name);
        canvas_set_font(canvas, FontSecondary);
        canvas_draw_str_aligned(canvas, 64, 42, AlignCenter, AlignCenter, "Sending");
        return;
    }

    canvas_set_font(canvas, FontSecondary);
    canvas_draw_str_aligned(
        canvas, 64, 14, AlignCenter, AlignCenter, flipperham_preset->name);
    canvas_set_font(canvas, FontPrimary);
    snprintf(text, sizeof(text), "%u", app->current_tone_hz); canvas_draw_str_aligned(canvas, 64, 32, AlignCenter, AlignCenter, text);
}

static void flipperham_load_first_segment(FlipperHamApp* app) 
{
    app->segment_index = 0;
    app->level = !packet3_aprs_packet_start_level;
    app->tx_done = false;
    app->current_tone_hz = flipperham_modem_profile->mark_hz;
    app->current_half_period_us = packet3_aprs_packet_durations_us[0];
}

static LevelDuration flipperham_yield(void* context) 
{
    FlipperHamApp* app = context;
    LevelDuration level_duration;

    if(app->tx_done) return level_duration_reset();
    

    level_duration = level_duration_make(app->level, app->current_half_period_us);
    app->level = !app->level;
    app->segment_index++;
    if(app->segment_index >= SEG) {
        app->tx_done = true;
    } else {
        app->current_half_period_us = packet3_aprs_packet_durations_us[app->segment_index];
    }

    return level_duration;
}

static void flipperham_radio_start(FlipperHamApp* app) 
{
    furi_hal_subghz_reset();
    furi_hal_subghz_idle();
    furi_hal_subghz_load_custom_preset((uint8_t*)flipperham_preset->regs);
    furi_hal_subghz_set_frequency_and_path(433250000UL);
    furi_hal_subghz_flush_tx();

    if(!furi_hal_subghz_tx()) {
        app->tx_allowed = false;
        app->tx_done = true;
        return;
    }

    app->tx_allowed = furi_hal_subghz_start_async_tx(flipperham_yield, app);
    app->tx_started = app->tx_allowed;
    if(!app->tx_allowed) app->tx_done = true;
}

static void flipperham_radio_stop(FlipperHamApp* app) 
{
    if(app->tx_started) furi_hal_subghz_stop_async_tx();
    furi_hal_subghz_sleep();
}

static FlipperHamApp* flipperham_app_alloc(void) {
    FlipperHamApp* app = malloc(sizeof(FlipperHamApp));

    app->gui = furi_record_open(RECORD_GUI);
    app->view_dispatcher = view_dispatcher_alloc();
    app->submenu = submenu_alloc();

        app->view_port = NULL;
        app->tx_started = false;
        app->tx_allowed = true;
        app->tx_done = false;
        app->send_requested = false;

    view_dispatcher_enable_queue( app->view_dispatcher);
    view_dispatcher_attach_to_gui(app->view_dispatcher, app->gui, ViewDispatcherTypeFullscreen);

    submenu_add_item( app->submenu, "Send", FlipperHamSubmenuIndexSend, flipperham_submenu_callback, app);
    view_set_previous_callback(submenu_get_view(app->submenu), flipperham_exit_callback);
    view_dispatcher_add_view( app->view_dispatcher, FlipperHamViewSubmenu, submenu_get_view(app->submenu));
    view_dispatcher_switch_to_view(app->view_dispatcher, FlipperHamViewSubmenu);

    flipperham_load_first_segment(app);

    return app;
}

static void flipperham_menu_free(FlipperHamApp* app) 
{
    if(app->view_dispatcher) {
        view_dispatcher_remove_view(app->view_dispatcher, FlipperHamViewSubmenu);
        view_dispatcher_free(app->view_dispatcher);
        app->view_dispatcher = NULL;
    }

    if(app->submenu) 
    {
        submenu_free(app->submenu);
        app->submenu = NULL;
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

static void flipperham_app_free(FlipperHamApp* app) 
{
    if(!app) return;

    flipperham_status_view_free(app);
    flipperham_menu_free(app);
    if(app->gui) furi_record_close(RECORD_GUI);
    free(app);
}

static void flipperham_send_hardcoded_message(FlipperHamApp* app) 
{
    flipperham_load_first_segment(app);
    app->tx_started = false;
    app->tx_allowed = true;

    flipperham_status_view_alloc(app);
    view_port_update(app->view_port);

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

    view_port_update(app->view_port);
    furi_delay_ms(750);

    flipperham_radio_stop(app);
    furi_hal_power_suppress_charge_exit();
}

int32_t flipperham_app(void* p)
{
    UNUSED(p);

    FlipperHamApp* app = flipperham_app_alloc();

    view_dispatcher_run(app->view_dispatcher);
    flipperham_menu_free(app);

    if(app->send_requested) {
        flipperham_send_hardcoded_message(app);
    }

    flipperham_app_free(app);
    return 0;
}
