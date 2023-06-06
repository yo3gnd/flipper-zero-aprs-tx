#include "flipperham.h"

#include <furi_hal.h>
#include <gui/gui.h>
#include <gui/view_port.h>
#include <input/input.h>
#include <cc1101_regs.h>
#include <furi_hal_subghz_configs.h>
#include <lib/toolbox/level_duration.h>

#include <stdio.h>

#define CARRIER_HZ 433500000UL

#include "tools/3_aprs_packet.txt"

#define SEG packet3_aprs_packet_durations_count

typedef struct {
    const char* name;
    const uint8_t* regs;
} FlipperHamPreset;

enum {
    FlipperHamPresetDefault = 0,
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
    ViewPort* view_port;
    FuriMessageQueue* event_queue;
    volatile uint16_t current_tone_hz;
    volatile uint16_t current_half_period_us;
    volatile uint16_t edgess;

    volatile uint16_t segment_index;

    volatile bool level;
    volatile bool tx_started;
    volatile bool tx_done;
    volatile bool tx_allowed;
} FlipperHamApp;

static const FlipperHamPreset flipperham_presets[] = {
    { "2FSK 5.16", flipperham_preset_2fsk_dev5_16khz_async_regs },
    { "GFSK 4.8", flipperham_preset_gfsk_dev5_16khz_async_regs },
    { "GFSK 9.99", flipperham_preset_gfsk_dev5_16khz_fast_async_regs },
};

static const FlipperHamPreset* flipperham_preset = &flipperham_presets[FlipperHamPresetDefault];

static void flipperham_input_callback(InputEvent* input_event, void* context) {
    FuriMessageQueue* event_queue = context;
    furi_message_queue_put(event_queue, input_event, FuriWaitForever);
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
        canvas_draw_str_aligned(canvas, 64, 42, AlignCenter, AlignCenter, "OK");
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


    /* app->current_tone_hz = 1200 */
    app->current_tone_hz = 0;
    app->current_half_period_us = packet3_aprs_packet_durations_us[0];
    app->edgess = 0;
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
    app->event_queue = furi_message_queue_alloc(8, sizeof(InputEvent));
    app->tx_started = false;
    app->tx_allowed = true;

    flipperham_load_first_segment(app);

    view_port_draw_callback_set(app->view_port, flipperham_draw_callback, app);
    view_port_input_callback_set(app->view_port, flipperham_input_callback, app->event_queue);
    gui_add_view_port(app->gui, app->view_port, GuiLayerFullscreen);

    view_port_update(app->view_port);

    InputEvent event;
    while(true) {
        while(furi_message_queue_get(app->event_queue, &event, FuriWaitForever) == FuriStatusOk) {
            if(event.type != InputTypeShort) {
                continue;
            }

            if(event.key == InputKeyBack) {
                gui_remove_view_port(app->gui, app->view_port);
                view_port_free(app->view_port);
                furi_message_queue_free(app->event_queue);
                furi_record_close(RECORD_GUI);
                free(app);
                return 0;
            } else if(event.key == InputKeyOk) {
                break;
            }

            view_port_update(app->view_port);
        }

        flipperham_load_first_segment(app);
        app->tx_started = false;
        app->tx_allowed = true;

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
        app->tx_started = false;
        app->tx_done = false;
        app->tx_allowed = true;

        if(event.type != InputTypeShort) {
            break;
        }
        view_port_update(app->view_port);
    }

    return 0;
}
