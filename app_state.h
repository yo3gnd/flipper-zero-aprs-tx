#ifndef GUARD_QWAASASESWQ3321_APPSTATE
#define GUARD_QWAASASESWQ3321_APPSTATE

#include "flipperham.h"
#include "packet.h"

#include <gui/modules/submenu.h>
#include <gui/modules/text_input.h>
#include <gui/modules/variable_item_list.h>


#include <gui/view_dispatcher.h>
#include <gui/view_port.h>
#include <gui/gui.h>


typedef struct FlipperHamApp
{
    Gui* gui;
    ViewDispatcher* view_dispatcher;
    Submenu* submenu;
    Submenu* send_menu;
    Submenu* bulletin_menu;
    Submenu* status_menu;
    Submenu* message_menu;
    Submenu* call_menu;
    Submenu* book_menu;
    Submenu* c2_menu;
    VariableItemList* settings_menu;
    VariableItemList* ssid_menu;
    TextInput* text_input;
    ViewPort* view_port;
    volatile uint16_t wave_i;
    volatile bool level;
    volatile bool tx_started;
    volatile bool tx_done;
    volatile bool tx_allowed;
    bool done_w;
    bool send_requested;
    uint8_t encoding_index;
    uint8_t rf_m;
    uint8_t rf_d;
    uint8_t repeat_n;
    uint8_t repeat_i;
    Packet* pkt;
    uint16_t* wave;
    uint16_t wave_len;
    int16_t wave_carry;
    bool wave_is_mark;
    uint32_t repeat_t0;
    uint32_t repeat_to;
    uint8_t go_v;
    char bulletin[TXT_N][TXT_LEN];
    char status[TXT_N][TXT_LEN];
    char message[TXT_N][TXT_LEN];
    char calls[CALL_N][CALL_LEN];
    uint8_t bulletin_used[TXT_N];
    uint8_t status_used[TXT_N];
    uint8_t message_used[TXT_N];
    uint8_t calls_used[CALL_N];
    uint8_t bulletin_n;
    uint8_t status_n;
    uint8_t message_n;
    uint8_t calls_n;
    uint8_t tx_msg_index;
    uint8_t tx_t;
    uint8_t bulletin_index;
    uint8_t status_index;
    uint8_t message_index;
      uint8_t dst_call_index;
    uint8_t d_s;
    uint8_t edit_call_index;
    uint8_t book_call_index;
    uint8_t txt;
    char b_edit[TXT_LEN];
    char st_edit[TXT_LEN];
    char m_edit[TXT_LEN];
    char c_edit[CALL_LEN];
    char c2_h[24];
} FlipperHamApp;

#endif
