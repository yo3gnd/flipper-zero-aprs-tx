#include "flipperham.h"

#include <furi_hal.h>
#include <gui/gui.h>
#include <gui/modules/variable_item_list.h>
#include <gui/view.h>
#include <gui/view_dispatcher.h>
#include <gui/view_port.h>
#include <cc1101_regs.h>
#include <furi_hal_subghz_configs.h>
#include <lib/toolbox/level_duration.h>

#include <stdio.h>

#define CARRIER_HZ    433250000UL
#define MY_TOCALL     "APZFLP"

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

void packet_do_all(Packet* p, const char* from, uint8_t from_ssid, const char* to, uint8_t to_ssid, const char* s);

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
    VariableItemList* variable_item_list;
    ViewPort* view_port;
    volatile uint16_t current_tone_hz;
    volatile uint16_t current_half_period_us;
    volatile uint16_t segment_index;
    volatile bool level;
    volatile bool tx_started;
    volatile bool tx_done;
    volatile bool tx_allowed;
    bool send_requested;
    uint8_t encoding_index;
    uint8_t msg_n;
    uint8_t src_ssid;
    Packet* pkt;
    uint16_t* wave;
    uint16_t wave_n;
    int16_t wave_c;
    bool wave_mark;
    FlipperHamRuntimeTx tx;
} FlipperHamApp;

enum 
{
    FlipperHamViewSubmenu = 0,
};

enum 
{
    FlipperHamMenuIndexSend = 0,
    FlipperHamMenuIndexEncoding,
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
static const FlipperHamModemProfile* flipperham_modem_profile =
    &flipperham_modem_profiles[FlipperHamModemProfileDefault];
static const char* flipperham_encoding_text[] = 
{
    "300 bd",
    "1200 bd",
};

static uint32_t flipperham_exit_callback(void* context) 
{
    UNUSED(context);
    return VIEW_NONE;
}

static void flipperham_encoding_change(VariableItem* item)
{
    FlipperHamApp* app = variable_item_get_context(item);
    uint8_t index = variable_item_get_current_value_index(item);

    app->encoding_index = index;
    variable_item_set_current_value_text(item, flipperham_encoding_text[index]);
}

static void flipperham_menu_callback(void* context, uint32_t index) 
{
    FlipperHamApp* app = context;

    if(index == FlipperHamMenuIndexSend) 
    {
        app->send_requested = true;
        view_dispatcher_stop(app->view_dispatcher);
    }

}

static void flipperham_draw_callback(Canvas* canvas, void* context) 
{
    FlipperHamApp* app = context;

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

        canvas_draw_str_aligned(canvas, 64, 32, AlignCenter, AlignCenter, "Sending...");
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
    char s[64];
    uint16_t i;

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

    snprintf(s, sizeof(s), "Hello world, I am Flipper Zero :D msg %02u", app->msg_n);
    packet_do_all(app->pkt, "YO0FLP", app->src_ssid, MY_TOCALL, 0, s);

    app->msg_n++;
    if(app->msg_n > 99) app->msg_n = 0;

    app->src_ssid++;
    if(app->src_ssid > 3) app->src_ssid = 0;

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

#if 0
static void txnext(FlipperHamApp* app)
{
    uint8_t b;

        if(app->tx.i >= app->tx.bits_n) {
            app->tx_done = true;
            return;
        }

    b = app->tx.bits[app->tx.i];

    /* NRZI edge = flip at bit start */
    if(app->tx.i == 0) {
        if(b != 1) app->tx.mark = !app->tx.mark;
    } else {
        if(b != app->tx.bits[app->tx.i - 1]) app->tx.mark = !app->tx.mark;
    }

    app->tx.n = 0;

    /* 1200 mark must be 417+416, not 416+416+1 */
    if(app->tx.mark) 
    {
        app->current_tone_hz = 1200;
        app->tx.k = 2;
        app->tx.part[0] = 13750;
        app->tx.part[1] = 13750;
        app->tx.part[2] = 0;
        app->tx.part[3] = 0;
    }
    else
    {
        app->current_tone_hz = 2200;
        app->tx.k = 4;
        app->tx.part[0] = 7500;
        app->tx.part[1] = 7500;
        app->tx.part[2] = 7500;
        app->tx.part[3] = 5000;
    }

    app->tx.half_us = 0;
    app->tx.half_left_us = 0;
    app->tx.bit_left_us = 833;
}
#endif

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
    if(app->tx_started) furi_hal_subghz_stop_async_tx();
    furi_hal_subghz_sleep();
}

static FlipperHamApp* flipperham_app_alloc(void) {
    FlipperHamApp* app = malloc(sizeof(FlipperHamApp));
    VariableItem* item;

    app->gui = furi_record_open(RECORD_GUI);
    app->view_dispatcher = view_dispatcher_alloc();
    app->variable_item_list = variable_item_list_alloc();

        app->view_port = NULL;
        app->tx_started = false;
        app->tx_allowed = true;
        app->tx_done = false;
        app->send_requested = false;
        app->encoding_index = FlipperHamModemProfileDefault;
        app->msg_n = 0;
        app->src_ssid = 0;
        app->pkt = malloc(sizeof(Packet));
        app->wave = malloc(sizeof(uint16_t) * 4096);

    view_dispatcher_enable_queue( app->view_dispatcher);
    view_dispatcher_attach_to_gui(app->view_dispatcher, app->gui, ViewDispatcherTypeFullscreen);

    variable_item_list_set_enter_callback( app->variable_item_list, flipperham_menu_callback, app);

    variable_item_list_add( app->variable_item_list, "Send", 0, NULL, app);

    item = variable_item_list_add( app->variable_item_list, "Encoding", 2, flipperham_encoding_change, app);
    variable_item_set_current_value_index(item, app->encoding_index);
    variable_item_set_current_value_text(item, flipperham_encoding_text[app->encoding_index]);

    view_set_previous_callback(variable_item_list_get_view(app->variable_item_list), flipperham_exit_callback);
    view_dispatcher_add_view( app->view_dispatcher, FlipperHamViewSubmenu, variable_item_list_get_view(app->variable_item_list));
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

    if(app->variable_item_list) 
    {
        variable_item_list_free(app->variable_item_list);
        app->variable_item_list = NULL;
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
    if(app->pkt) free(app->pkt);
    if(app->wave) free(app->wave);
    if(app->gui) furi_record_close(RECORD_GUI);
    free(app);
}

static void flipperham_send_hardcoded_message(FlipperHamApp* app) 
{
    txstart(app);
    /* flipperham_load_first_segment(app); // old table */
    app->tx_started = false;
    app->tx_allowed = true;

    flipperham_status_view_alloc(app);
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
    view_port_update(app->view_port);
    furi_delay_ms(750);
    furi_hal_power_suppress_charge_exit();


flipperham_status_view_free(app);
}

int32_t flipperham_app(void* p)
{
    UNUSED(p);

    FlipperHamApp* app = flipperham_app_alloc();

    while(1) 
    {
        app->send_requested = false;
        view_dispatcher_switch_to_view(app->view_dispatcher, FlipperHamViewSubmenu);
        view_dispatcher_run(app->view_dispatcher);



        if(!app->send_requested) break;

        flipperham_send_hardcoded_message(app);
    }

    flipperham_app_free(app);
    return 0;
}
