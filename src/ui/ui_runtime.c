#include "ui_i.h"

#include <furi_hal.h>
#include <gui/elements.h>
#include <gui/view.h>
#include <notification/notification.h>
#include <notification/notification_messages.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void stin(InputEvent *event, void *context);
static void splash_draw(Canvas *canvas, void *model);
static bool splash_input(InputEvent *event, void *context);
static bool splash_custom(uint32_t event, void *context);
static void splash_enter(void *context);
static void splash_exit(void *context);
static void splash_timer(void *context);
static void splash_cycle_timer(void *context);
static void splash_footer_draw(Canvas *canvas, uint8_t footer_i);
static void splash_view_alloc(FlipperHamApp *app);
static void splash_view_free(FlipperHamApp *app);

enum
{
    SplashCustomEventShowNext = 1,
    SplashCustomEventFooterTick,
};

typedef struct
{
    const uint8_t *data;
    uint8_t w;
    uint8_t h;
    int8_t y;
} SplashText;

typedef struct
{
    const SplashText *a;
    const SplashText *b;
} SplashFooter;

typedef struct
{
    bool allow_next_button;
    bool show_next_button;
    uint8_t footer_i;
} SplashModel;
/* Previous custom chunky logo, kept around in case we want to switch back.
static const uint8_t splash_yo3gnd_blocky[] = {
    0x03, 0xc3, 0x0f, 0xff, 0xc0, 0x0f, 0x03, 0xf3, 0x0f,
    0x03, 0xc3, 0x0f, 0xff, 0xc0, 0x0f, 0x03, 0xf3, 0x0f,
    0x03, 0x33, 0x30, 0x00, 0x33, 0x30, 0x0f, 0x33, 0x30,
    0x03, 0x33, 0x30, 0x00, 0x33, 0x30, 0x0f, 0x33, 0x30,
    0xcc, 0x30, 0x30, 0x00, 0x33, 0x00, 0x33, 0x33, 0x30,
    0xcc, 0x30, 0x30, 0x00, 0x33, 0x00, 0x33, 0x33, 0x30,
    0x30, 0x30, 0x30, 0xfc, 0x30, 0x3f, 0xc3, 0x33, 0x30,
    0x30, 0x30, 0x30, 0xfc, 0x30, 0x3f, 0xc3, 0x33, 0x30,
    0x30, 0x30, 0x30, 0x00, 0x33, 0x30, 0x03, 0x33, 0x30,
    0x30, 0x30, 0x30, 0x00, 0x33, 0x30, 0x03, 0x33, 0x30,
    0x30, 0x30, 0x30, 0x00, 0x33, 0x30, 0x03, 0x33, 0x30,
    0x30, 0x30, 0x30, 0x00, 0x33, 0x30, 0x03, 0x33, 0x30,
    0x30, 0xc0, 0x0f, 0xff, 0xc0, 0x0f, 0x03, 0xf3, 0x0f,
    0x30, 0xc0, 0x0f, 0xff, 0xc0, 0x0f, 0x03, 0xf3, 0x0f,
};
*/

/* Terminus 9pt (ter-u12n) doubled 2x and trimmed to the glyph bounds. */
static const uint8_t splash_yo3gnd[] = {
    0x03, 0xc3, 0x0f, 0xfc, 0xc0, 0x0f, 0x03, 0xf3, 0x03,
    0x03, 0xc3, 0x0f, 0xfc, 0xc0, 0x0f, 0x03, 0xf3, 0x03,
    0x03, 0x33, 0x30, 0x03, 0x33, 0x30, 0x03, 0x33, 0x0c,
    0x03, 0x33, 0x30, 0x03, 0x33, 0x30, 0x03, 0x33, 0x0c,
    0xcc, 0x30, 0x30, 0x00, 0x33, 0x00, 0x0f, 0x33, 0x30,
    0xcc, 0x30, 0x30, 0x00, 0x33, 0x00, 0x0f, 0x33, 0x30,
    0xcc, 0x30, 0x30, 0xf0, 0x30, 0x00, 0x33, 0x33, 0x30,
    0xcc, 0x30, 0x30, 0xf0, 0x30, 0x00, 0x33, 0x33, 0x30,
    0x30, 0x30, 0x30, 0x00, 0x33, 0x3f, 0xc3, 0x33, 0x30,
    0x30, 0x30, 0x30, 0x00, 0x33, 0x3f, 0xc3, 0x33, 0x30,
    0x30, 0x30, 0x30, 0x00, 0x33, 0x30, 0x03, 0x33, 0x30,
    0x30, 0x30, 0x30, 0x00, 0x33, 0x30, 0x03, 0x33, 0x30,
    0x30, 0x30, 0x30, 0x03, 0x33, 0x30, 0x03, 0x33, 0x0c,
    0x30, 0x30, 0x30, 0x03, 0x33, 0x30, 0x03, 0x33, 0x0c,
    0x30, 0xc0, 0x0f, 0xfc, 0xc0, 0x0f, 0x03, 0xf3, 0x03,
    0x30, 0xc0, 0x0f, 0xfc, 0xc0, 0x0f, 0x03, 0xf3, 0x03,
};

/* Native Terminus 9pt (ter-u12n), trimmed to the glyph bounds. */
static const uint8_t splash_web_yo3gnd[] = {
    0x00, 0x00, 0x00, 0x00, 0xe0, 0x00, 0x00, 0x04, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x10, 0x01, 0x00, 0x04, 0x00, 0x00,
    0x51, 0x14, 0x01, 0x91, 0x03, 0x79, 0x8f, 0x07, 0x74, 0x0e,
    0x51, 0x14, 0x01, 0x51, 0xc4, 0x44, 0x51, 0x04, 0x0c, 0x11,
    0x55, 0x55, 0x01, 0x51, 0x04, 0x45, 0x51, 0x04, 0x04, 0x11,
    0x55, 0x55, 0x01, 0x51, 0x04, 0x45, 0x51, 0x04, 0x04, 0x11,
    0x55, 0x55, 0x11, 0x51, 0x14, 0x45, 0x51, 0x44, 0x04, 0x11,
    0x8e, 0xe3, 0x10, 0x9e, 0xe3, 0x78, 0x91, 0x47, 0x04, 0x0e,
    0x00, 0x00, 0x00, 0x10, 0x00, 0x40, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x0e, 0x00, 0x38, 0x00, 0x00, 0x00, 0x00,
};

static const uint8_t splash_at_yo3gnd[] = {
    0x0e, 0x00, 0x38, 0x00, 0x00, 0x01,
    0x11, 0x00, 0x44, 0x00, 0x00, 0x01,
    0x59, 0xe4, 0x40, 0xde, 0xe3, 0x01,
    0x55, 0x14, 0x31, 0x51, 0x14, 0x01,
    0x55, 0x14, 0x41, 0x51, 0x14, 0x01,
    0x59, 0x14, 0x41, 0x51, 0x14, 0x01,
    0x41, 0x14, 0x45, 0x51, 0x14, 0x01,
    0x9e, 0xe7, 0x38, 0x5e, 0xe4, 0x01,
    0x00, 0x04, 0x00, 0x10, 0x00, 0x00,
    0x80, 0x03, 0x00, 0x0e, 0x00, 0x00,
};

static const uint8_t splash_website[] = {
    0x00, 0x00, 0x00, 0x04, 0x00, 0x00,
    0x00, 0x10, 0x00, 0x04, 0x01, 0x00,
    0x00, 0x10, 0x00, 0x00, 0x01, 0x00,
    0x91, 0xf3, 0x78, 0x86, 0xe3, 0x00,
    0x51, 0x14, 0x05, 0x04, 0x11, 0x01,
    0xd5, 0x17, 0x39, 0x04, 0xf1, 0x01,
    0x55, 0x10, 0x41, 0x04, 0x11, 0x00,
    0x55, 0x10, 0x41, 0x04, 0x11, 0x00,
    0x8e, 0xf7, 0x3c, 0x0e, 0xe6, 0x01,
};

static const uint8_t splash_instagram[] = {
    0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x02, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00,
    0xe3, 0xf1, 0x1c, 0xc7, 0xeb, 0x9c, 0x07,
    0x22, 0x0a, 0x08, 0x28, 0x1a, 0xa0, 0x0a,
    0x22, 0x72, 0x08, 0x2f, 0x0a, 0xbc, 0x0a,
    0x22, 0x82, 0x88, 0x28, 0x0a, 0xa2, 0x0a,
    0x22, 0x82, 0x88, 0x28, 0x0a, 0xa2, 0x0a,
    0x27, 0x7a, 0x30, 0xcf, 0x0b, 0xbc, 0x0a,
    0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00,
    0x00, 0x00, 0x00, 0xc0, 0x01, 0x00, 0x00,
};

static const uint8_t splash_tiktok[] = {
    0x80, 0x00, 0x00, 0x00, 0x00,
    0x82, 0x10, 0x08, 0x40, 0x00,
    0x02, 0x10, 0x08, 0x40, 0x00,
    0xc7, 0x90, 0x1c, 0x47, 0x02,
    0x82, 0x50, 0x88, 0x48, 0x01,
    0x82, 0x30, 0x88, 0xc8, 0x00,
    0x82, 0x30, 0x88, 0xc8, 0x00,
    0x82, 0x50, 0x88, 0x48, 0x01,
    0xcc, 0x91, 0x30, 0x47, 0x02,
};

static const uint8_t splash_github_com[] = {
    0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x41, 0x04, 0x40, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x40, 0x04, 0x40, 0x00, 0x00, 0x00, 0x00,
    0x9e, 0xe1, 0x3c, 0xd1, 0x03, 0x38, 0xce, 0x03,
    0x11, 0x41, 0x44, 0x51, 0x04, 0x44, 0x51, 0x05,
    0x11, 0x41, 0x44, 0x51, 0x04, 0x04, 0x51, 0x05,
    0x11, 0x41, 0x44, 0x51, 0x04, 0x04, 0x51, 0x05,
    0x11, 0x41, 0x44, 0x51, 0x44, 0x44, 0x51, 0x05,
    0x9e, 0x83, 0x45, 0xde, 0x43, 0x38, 0x4e, 0x05,
    0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x0e, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

static const uint8_t splash_yo3gnd_pad[] = {
    0x00, 0x00, 0x38, 0x00, 0x00, 0x01,
    0x00, 0x00, 0x44, 0x00, 0x00, 0x01,
    0x40, 0xe4, 0x40, 0xde, 0xe3, 0x01,
    0x40, 0x14, 0x31, 0x51, 0x14, 0x01,
    0x40, 0x14, 0x41, 0x51, 0x14, 0x01,
    0x40, 0x14, 0x41, 0x51, 0x14, 0x01,
    0x40, 0x14, 0x45, 0x51, 0x14, 0x01,
    0x80, 0xe7, 0x38, 0x5e, 0xe4, 0x01,
    0x00, 0x04, 0x00, 0x10, 0x00, 0x00,
    0x80, 0x03, 0x00, 0x0e, 0x00, 0x00,
};

static const uint8_t splash_youtube[] = {
    0x00, 0x00, 0x10, 0x40, 0x00, 0x00,
    0x00, 0x00, 0x10, 0x40, 0x00, 0x00,
    0x91, 0x13, 0x39, 0xd1, 0xe3, 0x00,
    0x51, 0x14, 0x11, 0x51, 0x14, 0x01,
    0x51, 0x14, 0x11, 0x51, 0xf4, 0x01,
    0x51, 0x14, 0x11, 0x51, 0x14, 0x00,
    0x51, 0x14, 0x11, 0x51, 0x14, 0x00,
    0x9e, 0xe3, 0x61, 0xde, 0xe3, 0x01,
    0x10, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x0e, 0x00, 0x00, 0x00, 0x00, 0x00,
};

static const uint8_t splash_left_icon[] = {
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x80, 0x01, 0x00,
    0x00, 0x80, 0x01, 0x00,
    0x00, 0xf8, 0x1f, 0x00,
    0x00, 0xfe, 0x7f, 0x00,
    0x00, 0x06, 0x60, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x80, 0x01, 0x00,
    0x00, 0xf8, 0x1f, 0x00,
    0x00, 0x38, 0x1c, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0xc0, 0x03, 0x00,
    0x00, 0x60, 0x06, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x80, 0x01, 0x00,
    0x00, 0xc0, 0x03, 0x00,
    0x46, 0xc4, 0x23, 0x62,
    0x46, 0x8c, 0x31, 0x62,
    0xc6, 0x0c, 0x30, 0x63,
    0xc6, 0x18, 0x18, 0x63,
    0xc6, 0x31, 0x8c, 0x63,
    0x8c, 0x01, 0x80, 0x31,
    0x0c, 0x03, 0xc0, 0x30,
    0x1e, 0x06, 0x60, 0x78,
    0x3f, 0x04, 0x20, 0xfc,
    0x31, 0x00, 0x00, 0x8c,
    0xe0, 0x00, 0x00, 0x07,
    0xc0, 0x01, 0x80, 0x03,
    0x80, 0x01, 0x80, 0x01,
    0x00, 0x00, 0x00, 0x00,
};

static const SplashText splash_text_www_yo3gnd = { splash_web_yo3gnd, 77, 10, 0 };
static const SplashText splash_text_at_yo3gnd = { splash_at_yo3gnd, 41, 10, 0 };
static const SplashText splash_text_website = { splash_website, 41, 9, 0 };
static const SplashText splash_text_instagram = { splash_instagram, 52, 11, 0 };
static const SplashText splash_text_tiktok = { splash_tiktok, 34, 9, 0 };
static const SplashText splash_text_github_com = { splash_github_com, 59, 11, 0 };
static const SplashText splash_text_yo3gnd_pad = { splash_yo3gnd_pad, 42, 10, 0 };
static const SplashText splash_text_youtube = { splash_youtube, 41, 10, 0 };

static const SplashFooter splash_footer_seq[] = {
    { &splash_text_www_yo3gnd, &splash_text_website },
    { &splash_text_at_yo3gnd, &splash_text_instagram },
    { &splash_text_at_yo3gnd, &splash_text_tiktok },
    { &splash_text_www_yo3gnd, &splash_text_website },
    { &splash_text_at_yo3gnd, &splash_text_instagram },
    { &splash_text_at_yo3gnd, &splash_text_tiktok },
    { &splash_text_yo3gnd_pad, &splash_text_github_com },
    { &splash_text_at_yo3gnd, &splash_text_youtube },
};

void flipperham_status_view_alloc(FlipperHamApp *app)
{
    app->view_port = view_port_alloc();
    view_port_draw_callback_set(app->view_port, flipperham_draw_callback, app);
    view_port_input_callback_set(app->view_port, stin, app);
    gui_add_view_port(app->gui, app->view_port, GuiLayerFullscreen);
}

void flipperham_status_view_free(FlipperHamApp *app)
{
    if (!app->view_port)
    {
        return;
    }

    gui_remove_view_port(app->gui, app->view_port);
    view_port_free(app->view_port);
    app->view_port = NULL;
}

static void splash_footer_draw(Canvas *canvas, uint8_t footer_i)
{
    const SplashFooter *f;
    int y;
    int x;

    f = &splash_footer_seq[footer_i % (sizeof(splash_footer_seq) / sizeof(splash_footer_seq[0]))];
    y = 36;

    x = 32 + (96 - f->a->w) / 2;
    canvas_draw_xbm(canvas, x, y + f->a->y, f->a->w, f->a->h, f->a->data);

    x = 32 + (96 - f->b->w) / 2;
    canvas_draw_xbm(canvas, x, y + f->a->h + 2 + f->b->y, f->b->w, f->b->h, f->b->data);


}

static void splash_draw(Canvas *canvas, void *model)
{
    SplashModel *m = model;

    canvas_clear(canvas);
    canvas_draw_xbm(canvas, 4, 16, 32, 32, splash_left_icon);

    canvas_draw_xbm(canvas, 45, 8, 70, 16, splash_yo3gnd);
    splash_footer_draw(canvas, m->footer_i);

    if (m->allow_next_button && m->show_next_button)
        elements_button_right(canvas, "Next");
}

static void splash_timer(void *context)
{
    FlipperHamApp *app = context;

    if (!app)
        return;
    view_dispatcher_send_custom_event(app->view_dispatcher, SplashCustomEventShowNext);
}

static void splash_cycle_timer(void *context)
{
    FlipperHamApp *app = context;

    if (!app)
        return;
    view_dispatcher_send_custom_event(app->view_dispatcher, SplashCustomEventFooterTick);
}

static bool splash_custom(uint32_t event, void *context)
{
    FlipperHamApp *app = context;

    if (event == SplashCustomEventShowNext)
    {
        with_view_model(
            app->splash_view,
            SplashModel *m,
            {
                if (m->allow_next_button)
                    m->show_next_button = true;
            },
            true);

        return true;
    }

    if (event == SplashCustomEventFooterTick)
    {
        with_view_model(
            app->splash_view,
            SplashModel *m,
            {
                m->footer_i++;
                if (m->footer_i >= sizeof(splash_footer_seq) / sizeof(splash_footer_seq[0]))
                    m->footer_i = 0;
            },
            true);

        return true;
    }

    return false;
}

static void splash_enter(void *context)
{
    FlipperHamApp *app = context;
    bool show_next;

    show_next = app->splash_next_view == FlipperHamViewReadme && !app->splash_back_exit;

    with_view_model(
        app->splash_view,
        SplashModel *m,
        {
            m->allow_next_button = show_next;
            m->show_next_button = false;
            m->footer_i = 0;
        },
        true);

    if (show_next && app->splash_timer)
        furi_timer_start(app->splash_timer, furi_ms_to_ticks(1000));
    if (app->splash_cycle_timer)
        furi_timer_start(app->splash_cycle_timer, furi_ms_to_ticks(1000));
}

static void splash_exit(void *context)
{
    FlipperHamApp *app = context;

    if (app->splash_timer)
        furi_timer_stop(app->splash_timer);
    if (app->splash_cycle_timer)
        furi_timer_stop(app->splash_cycle_timer);

    with_view_model(
        app->splash_view,
        SplashModel *m,
        {
            m->allow_next_button = false;
            m->show_next_button = false;
            m->footer_i = 0;
        },
        false);
}

static bool splash_input(InputEvent *event, void *context)
{
    FlipperHamApp *app = context;
    bool wait_next;

    if (event->type != InputTypeShort)
        return false;

    wait_next = false;
    with_view_model(
        app->splash_view,
        SplashModel *m,
        {
            if (m->allow_next_button && !m->show_next_button)
                wait_next = true;
        },
        false);

    if (wait_next)
        return true;

    if (event->key == InputKeyBack)
    {
        if (app->splash_back_exit)
            view_dispatcher_stop(app->view_dispatcher);
        else
            view_dispatcher_switch_to_view(app->view_dispatcher, app->splash_next_view);
        return true;
    }

    view_dispatcher_switch_to_view(app->view_dispatcher, app->splash_next_view);
    return true;
}

static void splash_view_alloc(FlipperHamApp *app)
{
    app->splash_view = view_alloc();
    app->splash_timer = furi_timer_alloc(splash_timer, FuriTimerTypeOnce, app);
    app->splash_cycle_timer = furi_timer_alloc(splash_cycle_timer, FuriTimerTypePeriodic, app);
    view_set_context(app->splash_view, app);
    view_allocate_model(app->splash_view, ViewModelTypeLocking, sizeof(SplashModel));
    view_set_draw_callback(app->splash_view, splash_draw);
    view_set_input_callback(app->splash_view, splash_input);
    view_set_custom_callback(app->splash_view, splash_custom);
    view_set_enter_callback(app->splash_view, splash_enter);
    view_set_exit_callback(app->splash_view, splash_exit);

    with_view_model(
        app->splash_view,
        SplashModel *m,
        {
            m->allow_next_button = false;
            m->show_next_button = false;
            m->footer_i = 0;
        },
        true);
}

static void splash_view_free(FlipperHamApp *app)
{
    if (app->splash_timer)
    {
        furi_timer_free(app->splash_timer);
        app->splash_timer = NULL;
    }
    if (app->splash_cycle_timer)
    {
        furi_timer_free(app->splash_cycle_timer);
        app->splash_cycle_timer = NULL;
    }

    if (!app->splash_view)
        return;

    view_free(app->splash_view);
    app->splash_view = NULL;
}

void flipperham_menu_free(FlipperHamApp *app)
{
    if (app->view_dispatcher)
    {
        view_dispatcher_remove_view(app->view_dispatcher, FlipperHamViewSplash);
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

    splash_view_free(app);
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

uint32_t repeat_scale(FlipperHamApp *app)
{
    if (app->encoding_index == 0)
        return 4;
    return 1;
}

FlipperHamApp *flipperham_app_alloc(void)
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
    app->splash_view = NULL;
    app->splash_timer = NULL;
    app->splash_cycle_timer = NULL;

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
    app->return_view = FlipperHamViewSplash;
    app->splash_next_view = FlipperHamViewMenu;
    app->splash_back_exit = true;
    app->text_mode = 0;
    app->text_view = FlipperHamViewMenu;
    app->pkt = NULL;
    app->wave = NULL;

    cfgload(app);

    view_dispatcher_attach_to_gui(app->view_dispatcher, app->gui, ViewDispatcherTypeFullscreen);
    splash_view_alloc(app);

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
        "APRS experimental transmiter for Flipper. Don't transmit where you shouldn't. Uses FSK "
        "as a weak substitute for FM. Works, sometimes.\n\nI'm quite interested on what kind of "
        "hardware and with what parameters you got decodes.\n\nReports are really appreciated. Contact "
        "me at:\n\nwww.yo3gnd.ro\nyo3gnd@gmail.com\ngithub.com/yo3gnd\ninstagram: @yo3gnd\ntiktok: @yo3ngd\nyoutube.com/@yo3gnd\n\n");
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

    view_dispatcher_add_view(app->view_dispatcher, FlipperHamViewSplash, app->splash_view);
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

void flipperham_app_free(FlipperHamApp *app)
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

void flipperham_send_hardcoded_message(FlipperHamApp *app)
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
