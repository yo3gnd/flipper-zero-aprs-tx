#include "rf_gen.h"

#include <furi_hal.h>
#include <cc1101_regs.h>
#include <furi_hal_subghz_configs.h>
#include <lib/toolbox/level_duration.h>

#include <stdio.h>
#include <string.h>

bool csplit(const char* s, char* out, uint8_t* ssid, bool* has_ssid);


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


static const FlipperHamPreset* flipperham_preset = &flipperham_presets[FlipperHamPresetDefault];

static bool wave_flag(FlipperHamApp* app);
static bool wave_put(FlipperHamApp* app, uint8_t bit);
static bool wave_add(FlipperHamApp* app, uint16_t value);
static LevelDuration edge_yield(void* context);


FLIPPERHAM_ASYNC_PRESET(flipperham_preset_2fsk_dev5_16khz_async_regs, 0x04, 0x83, 0x67, 0x15)
  FLIPPERHAM_ASYNC_PRESET(flipperham_preset_2fsk_dev2_38khz_async_regs, 0x04, 0x83, 0x67, 0x04)


FLIPPERHAM_ASYNC_PRESET(flipperham_preset_gfsk_dev2_38khz_async_regs, 0x14, 0x83, 0x67, 0x04)
FLIPPERHAM_ASYNC_PRESET(flipperham_preset_gfsk_dev5_16khz_async_regs, 0x14, 0x83, 0x67, 0x15)


const FlipperHamModemProfile flipperham_modem_profiles[] =
{
    { "300bd", 300, 1600, 1800 },
      { "1200bd", 1200, 1200, 2200 },
};


  const FlipperHamPreset flipperham_presets[] =
  {
      { "2FSK 2.5", flipperham_preset_2fsk_dev2_38khz_async_regs },
      { "GFSK 2.5", flipperham_preset_gfsk_dev2_38khz_async_regs },

      { "2FSK 5.0", flipperham_preset_2fsk_dev5_16khz_async_regs },
      { "GFSK 5.0", flipperham_preset_gfsk_dev5_16khz_async_regs },
  };


void pf(FlipperHamApp* app)
{
    uint8_t a;

    if(app->rf_m > 1) app->rf_m = 0;
    if(app->rf_d > 1) app->rf_d = 1;

    a = (app->rf_d * 2) + app->rf_m;
    if(a >= sizeof(flipperham_presets) / sizeof(flipperham_presets[0])) a = FlipperHamPresetDefault;

    flipperham_preset = &flipperham_presets[a];
}

uint32_t txf(FlipperHamApp* app)
{
    if(app->tx_freq_index < FREQ_N)
        if(app->freq_used[app->tx_freq_index])
            if(app->freq[app->tx_freq_index])
                return app->freq[app->tx_freq_index];

    return CARRIER_HZ;
}


static bool wave_add(FlipperHamApp* app, uint16_t value)
{
    int32_t acc;

    if(!app->wave) return false;
    if(app->wave_len >= 4096) return false;

    acc = value + app->wave_carry;
    value = (acc + 16) / 33;
    app->wave_carry = acc - ((int32_t)value * 33);
    if(!value) return true;

    app->wave[app->wave_len++] = value;
    return true;
}


    static bool wave_put(FlipperHamApp* app, uint8_t bit)
    {
        const FlipperHamModemProfile* p;
        uint32_t b;
        uint32_t h;
        uint32_t r;

        p = &flipperham_modem_profiles[app->encoding_index];
        b = (33000000UL + (p->baud / 2)) / p->baud;

        if(bit == 0) app->wave_is_mark = !app->wave_is_mark;

        if(app->wave_is_mark) h = (16500000UL + (p->mark_hz / 2)) / p->mark_hz;
        else h = (16500000UL + (p->space_hz / 2)) / p->space_hz;

        r = b;

        while(r > h)
        {
            if(!wave_add(app, h)) return false;
            r -= h;
        }

        if(r) if(!wave_add(app, r)) return false;

        return true;
    }


static bool wave_flag(FlipperHamApp* app)
{
    static const uint8_t flag[] = {0, 1, 1, 1, 1, 1, 1, 0};
    uint8_t i;

    for(i = 0; i < sizeof(flag); i++)
    {
        if(!wave_put(app, flag[i])) return false;
    }

    return true;
}


void txstart(FlipperHamApp* app)
{
    char message[96];
    char bulletin_id;
    char dst[CALL_LEN];
    char dst_full[CALL_LEN];
    const FlipperHamModemProfile* p;
    uint16_t i;
    uint16_t n;
    uint8_t j;
    uint8_t ssid;
    bool has_ssid;

    app->tx_done = false;
    app->wave_i = 0;
    app->level = true;
    app->wave_len = 0;
    app->wave_carry = 0;
    app->wave_is_mark = true;

    if(!app->pkt) return;
    if(!app->wave) return;
    if(app->tx_msg_index >= TXT_N) return;

    p = &flipperham_modem_profiles[app->encoding_index];

    if(app->tx_t == 0)
    {
        if(!app->bulletin_used[app->tx_msg_index]) return;
        if(!app->bulletin[app->tx_msg_index][0]) return;

        bulletin_id = '0';
        if(app->tx_msg_index < 10) bulletin_id = '0' + app->tx_msg_index;
        else if(app->tx_msg_index < 16) bulletin_id = 'A' + (app->tx_msg_index - 10);

        snprintf(message, sizeof(message), ":BLN%c     :%s", bulletin_id, app->bulletin[app->tx_msg_index]);
    }
    else if(app->tx_t == 1)
    {
        if(!app->status_used[app->tx_msg_index]) return;
        if(!app->status[app->tx_msg_index][0]) return;

        snprintf(message, sizeof(message), ">%s", app->status[app->tx_msg_index]);
    }
    else
    {
        if(app->dst_call_index >= CALL_N) return;
        if(!app->message_used[app->tx_msg_index]) return;
        if(!app->message[app->tx_msg_index][0]) return;
        if(!app->calls_used[app->dst_call_index]) return;
        if(!app->calls[app->dst_call_index][0]) return;

        if(!csplit(app->calls[app->dst_call_index], dst, &ssid, &has_ssid)) return;
        if(!has_ssid) ssid = app->d_s;

        j = 0;
        while(dst[j])
        {
            dst_full[j] = dst[j];
            j++;
        }
        dst_full[j++] = '-';
        if(ssid >= 10) dst_full[j++] = '0' + (ssid / 10);
        dst_full[j++] = '0' + (ssid % 10);
        dst_full[j] = 0;

        snprintf(message, sizeof(message), ":%-9s:%s", dst_full, app->message[app->tx_msg_index]);
    }

    packet_init(app->pkt);
    snprintf((char*)app->pkt->payload, sizeof(app->pkt->payload), "%s", message);
    app->pkt->payload_len = strlen((char*)app->pkt->payload);
    packet_make_ax25(app->pkt, MY_CALL, 0, MY_TOCALL, 0);
    packet_add_fcs(app->pkt);
    packet_stuff(app->pkt);
    packet_nrzi(app->pkt);

    n = (50 * p->baud + 500) / 1000;
    if(!n) n = 1;
    for(i = 0; i < n && wave_put(app, 1); i++);


    for(i = 0; i < 50; i++)
    {
        if(!wave_flag(app)) break;
    }


      for(i = 8; i + 8 < app->pkt->stuffed_len; i++)
      {
          if(!wave_put(app, app->pkt->stuffed[i])) break;
      }


    for(i = 0; i < 3; i++)
    {
        if(!wave_flag(app)) break;
    }

    if(!app->wave_len) app->tx_done = true;
}


static LevelDuration edge_yield(void* context)
{
    FlipperHamApp* app = context;
    LevelDuration ld;
    uint16_t half_us;

    if(app->tx_done) return level_duration_reset();

    if(app->wave_i >= app->wave_len)
    {
        app->tx_done = true;
        return level_duration_reset();
    }

    half_us = app->wave[app->wave_i];

    ld = level_duration_make(app->level, half_us);

    app->level = !app->level;
    app->wave_i++;

    if(app->wave_i >= app->wave_len) app->tx_done = true;

    return ld;
}


void flipperham_radio_start(FlipperHamApp* app)
{
    furi_hal_subghz_reset();
    furi_hal_subghz_idle();
    furi_hal_subghz_load_custom_preset((uint8_t*)flipperham_preset->regs);
    furi_hal_subghz_set_frequency_and_path(txf(app));
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


  void flipperham_radio_stop(FlipperHamApp* app)
  {
      if(app->tx_started)
          furi_hal_subghz_stop_async_tx();

      furi_hal_subghz_sleep();
  }
