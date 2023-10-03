#include "rf_gen.h"
#include "aprs.h"

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
static bool wave_add(FlipperHamApp* app, double value);
static LevelDuration edge_yield(void* context);
static uint16_t d2(double a);


FLIPPERHAM_ASYNC_PRESET(flipperham_preset_2fsk_dev5_16khz_async_regs, 0x04, 0x83, 0x67, 0x15)
  FLIPPERHAM_ASYNC_PRESET(flipperham_preset_2fsk_dev2_38khz_async_regs, 0x04, 0x83, 0x67, 0x04)
FLIPPERHAM_ASYNC_PRESET(flipperham_preset_2fsk_dev3_17khz_async_regs, 0x04, 0x83, 0x67, 0x10)


FLIPPERHAM_ASYNC_PRESET(flipperham_preset_gfsk_dev2_38khz_async_regs, 0x14, 0x83, 0x67, 0x04)
FLIPPERHAM_ASYNC_PRESET(flipperham_preset_gfsk_dev3_17khz_async_regs, 0x14, 0x83, 0x67, 0x10)
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

      { "2FSK 3.0", flipperham_preset_2fsk_dev3_17khz_async_regs },
      { "GFSK 3.0", flipperham_preset_gfsk_dev3_17khz_async_regs },

      { "2FSK 5.0", flipperham_preset_2fsk_dev5_16khz_async_regs },
      { "GFSK 5.0", flipperham_preset_gfsk_dev5_16khz_async_regs },
  };


void pf(FlipperHamApp* app)
{
    uint8_t a;

    if(app->rf_m > 1) app->rf_m = 0;
    if(app->rf_d > 2) app->rf_d = 2;

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


static bool wave_add(FlipperHamApp* app, double value)
{
    uint16_t a;

    if(!app->wave) return false;
    if(app->wave_len >= WAVE_N) return false;

    value += app->wave_carry;
    a = d2(value);
    app->wave_carry = value - a;
    if(!a) return true;

    app->wave[app->wave_len++] = a;
    return true;
}

static uint16_t d2(double a)
{
    uint16_t q;
    double r;

    q = (uint16_t)a;
    r = a - q;

      if(r > (double)0.5f) q++;
      else if(r >= (double)0.5f - (double)0.000001f) if(q & 1) q++;

    return q;
}


    static bool wave_put(FlipperHamApp* app, uint8_t bit)
    {
        const FlipperHamModemProfile* p;
        double b;
        double h;
        double a;

        p = &flipperham_modem_profiles[app->encoding_index];
        b = 1000000.0 / p->baud;

        if(bit == 0) app->wave_is_mark = !app->wave_is_mark;

        if(app->wave_is_mark) h = 1000000.0 / (2.0 * p->mark_hz);
        else h = 1000000.0 / (2.0 * p->space_hz);

        a = 0.0;

        while(a + h < b - (double)0.000001f)
        {
            if(!wave_add(app, h)) return false;
            a += h;
        }

        if(!wave_add(app, b - a)) return false;

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
    const char* src;
    uint16_t i;
    uint8_t j;
    uint8_t src_ssid;
    uint8_t ssid;
    bool has_ssid;

    app->tx_done = false;
    app->tx_ok = false;
    app->wave_i = 0;
    app->level = true;
    app->wave_len = 0;
    app->wave_carry = 0;
    app->pre_b = 0;
    app->pre_h = 0;
    app->pre_c = 0;
    app->pre_a = 0;
    app->pre_us = 0;
    app->wave_is_mark = true;

    if(!app->pkt) return;
    if(!app->wave) return;
    if(app->tx_msg_index >= TXT_N) return;

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
    else if(app->tx_t == 3)
    {
        if(!app->pos_used[app->tx_msg_index]) return;
        if(!app->pos_name[app->tx_msg_index][0]) return;
        if(!app->pos_lat[app->tx_msg_index][0]) return;
        if(!app->pos_lon[app->tx_msg_index][0]) return;
        if(!aprs_pos(message, sizeof(message), app->pos_name[app->tx_msg_index], app->pos_lat[app->tx_msg_index], app->pos_lon[app->tx_msg_index])) return;
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

    src = MY_CALL;
    src_ssid = 0;
    if(app->ham_ok) if(app->ham_n) if(app->ham_index < app->ham_n)
    {
        if(app->ham_calls[app->ham_index][0]) src = app->ham_calls[app->ham_index];
        if(app->ham_has_ssid[app->ham_index]) src_ssid = app->ham_ssid[app->ham_index];
    }

    packet_init(app->pkt);
    snprintf((char*)app->pkt->payload, sizeof(app->pkt->payload), "%s", message);
    app->pkt->payload_len = strlen((char*)app->pkt->payload);
    packet_make_ax25(app->pkt, src, src_ssid, MY_TOCALL, 0);
    packet_add_fcs(app->pkt);
    packet_stuff(app->pkt);
    packet_nrzi(app->pkt);

    if(app->preamble_ms)
    {
        app->pre_us = (app->preamble_ms * flipperham_modem_profiles[app->encoding_index].baud + 500) / 1000;
        if(!app->pre_us) app->pre_us = 1;
        app->pre_b = 1000000.0 / flipperham_modem_profiles[app->encoding_index].baud;
        app->pre_h = 1000000.0 / (2.0 * flipperham_modem_profiles[app->encoding_index].mark_hz);
    }


    for(i = 0; i < 50; i++)
    {
        if(!wave_flag(app)) return;
    }


      for(i = 8; i + 8 < app->pkt->stuffed_len; i++)
      {
          if(!wave_put(app, app->pkt->stuffed[i])) return;
      }


    for(i = 0; i < 3; i++)
    {
        if(!wave_flag(app)) return;
    }

    if(!app->wave_len && !app->pre_us) app->tx_done = true;
    else app->tx_ok = true;
}


static LevelDuration edge_yield(void* context)
{
    FlipperHamApp* app = context;
    LevelDuration ld;
    uint16_t half_us;
    double a;

    if(app->tx_done) return level_duration_reset();

    if(app->pre_us)
    {
        if(app->pre_a + app->pre_h < app->pre_b - (double)0.000001f)
        {
            a = app->pre_h + app->pre_c;
            half_us = d2(a);
            app->pre_c = a - half_us;
            app->pre_a += app->pre_h;
        }
        else
        {
            a = (app->pre_b - app->pre_a) + app->pre_c;
            half_us = d2(a);
            app->pre_c = a - half_us;
            app->pre_a = 0;
            app->pre_us--;
        }

        if(!half_us) half_us = 1;
        ld = level_duration_make(app->level, half_us);
        app->level = !app->level;
        if(!app->pre_us) if(!app->wave_len) app->tx_done = true;
        return ld;
    }

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
