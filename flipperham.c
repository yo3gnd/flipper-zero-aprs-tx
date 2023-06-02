#include "flipperham.h"

#include <furi_hal.h>
#include <gui/gui.h>
#include <gui/view_port.h>
#include <furi_hal_subghz_configs.h>
#include <lib/toolbox/level_duration.h>

#include <stdio.h>

#define CARRIER_HZ 433500000UL
#define SEG 6U

typedef struct {
    Gui* gui;
    ViewPort* view_port;
    volatile uint16_t current_tone_hz;
    volatile uint16_t current_half_period_us;
    volatile uint16_t edgess;

    volatile uint8_t segment_index;



    volatile bool level;
    volatile bool tx_started;
    volatile bool tx_done;
    volatile bool tx_allowed;
} FlipperHamApp;

static const uint16_t flipperham_tones_hz[SEG] = { 2200, 1200,  2200, 1200,   2200, 1200, };

static const uint16_t flipperham_half_periods_us[SEG] = { 227, 417, 227, 417, 227, 417, };

static const uint16_t flipperham_edges_per_segment[SEG] = { 4400, 2400, 4400, 2400, 4400, 2400, };

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

    snprintf(text, sizeof(text), "%u", app->current_tone_hz); canvas_draw_str_aligned(canvas, 64, 32, AlignCenter, AlignCenter, text);
}

static void flipperham_load_first_segment(FlipperHamApp* app) 
{
    app->segment_index = 0;
    app->level = false;
    app->tx_done = false;
    app->current_tone_hz = flipperham_tones_hz[0];
    app->current_half_period_us = flipperham_half_periods_us[0];
    app->edgess = flipperham_edges_per_segment[0];
}

static LevelDuration flipperham_yield(void* context) 
{
    FlipperHamApp* app = context;
    LevelDuration level_duration;

    if(app->tx_done) {
        return level_duration_reset();
    }

    level_duration = level_duration_make(app->level, app->current_half_period_us);
    app->level = !app->level;

    if(app->edgess > 0) 
        app->edgess--;

    if(app->edgess == 0) 
    {
        app->segment_index++;
        if(app->segment_index >= SEG) {
            app->tx_done = true;
        } else {
            app->current_tone_hz = flipperham_tones_hz[app->segment_index];
            app->current_half_period_us = flipperham_half_periods_us[app->segment_index];
            app->edgess = flipperham_edges_per_segment[app->segment_index];
        }
    }

    return level_duration;
}

static void flipperham_radio_start(FlipperHamApp* app) 
    {
    furi_hal_subghz_reset();
    furi_hal_subghz_idle();
    furi_hal_subghz_load_custom_preset((uint8_t*)furi_hal_subghz_preset_2fsk_dev2_38khz_async_regs);
    furi_hal_subghz_set_frequency_and_path(433250000UL);
    furi_hal_subghz_flush_tx();

    if(!furi_hal_subghz_tx()) {
        app->tx_allowed = false;
        app->tx_done = true;
        return;
    }

    app->tx_allowed = furi_hal_subghz_start_async_tx(flipperham_yield, app);
    app->tx_started = app->tx_allowed;
    if(!app->tx_allowed) {
        app->tx_done = true;
    }
}

static void flipperham_radio_stop(FlipperHamApp* app) {
    if(app->tx_started) {
        furi_hal_subghz_stop_async_tx();
    }
    furi_hal_subghz_sleep();
}

int32_t flipperham_app(void* p) {
    UNUSED(p);

    FlipperHamApp* app = malloc(sizeof(FlipperHamApp));
    app->gui = furi_record_open(RECORD_GUI);
    app->view_port = view_port_alloc();
    app->tx_started = false;
    app->tx_allowed = true;

    flipperham_load_first_segment(app);

    view_port_draw_callback_set(app->view_port, flipperham_draw_callback, app);
    gui_add_view_port(app->gui, app->view_port, GuiLayerFullscreen);

    furi_hal_power_suppress_charge_enter();
    flipperham_radio_start(app);

    while(!app->tx_done) {
        view_port_update(app->view_port);
        furi_delay_ms(50);
    }

    while(app->tx_started && !furi_hal_subghz_is_async_tx_complete()) 
    {
        view_port_update(app->view_port);

        /* furi_delay_ms(100); */
        furi_delay_ms(20);
    }

    view_port_update(app->view_port);
    furi_delay_ms(250);

    flipperham_radio_stop(app);
    furi_hal_power_suppress_charge_exit();

    gui_remove_view_port(app->gui, app->view_port);
    view_port_free(app->view_port);
        furi_record_close(RECORD_GUI);


        free(app);

      return 0;
}
