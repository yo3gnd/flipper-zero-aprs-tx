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
    Submenu* position_menu;
    Submenu* call_menu;
    Submenu* book_menu;
    Submenu* c2_menu;
    Submenu* freq_menu;
    VariableItemList* settings_menu;
    VariableItemList* ham_menu;
    VariableItemList* ham_tx_menu;
    VariableItemList* ssid_menu;
    VariableItemList* freq_edit_menu;
    VariableItemList* pos_edit_menu;
    TextInput* text_input;
    ViewPort* view_port;
    volatile uint16_t wave_i;
    volatile bool level;
    volatile bool tx_started;
    volatile bool tx_done;
    volatile bool tx_allowed;
    bool tx_ok;
    bool done_w;
    bool send_requested;
    bool ham_ok;
    uint8_t encoding_index;
    uint8_t rf_m;
    uint8_t rf_d;
    uint8_t repeat_n;
    uint8_t repeat_i;
    bool r_w;
    bool r_x;
    Packet* pkt;
    uint16_t* wave;
    uint16_t wave_len;
    double wave_carry;
    bool wave_is_mark;
    uint32_t repeat_t0;
    uint32_t repeat_to;
    uint8_t go_v;
    char bulletin[TXT_N][TXT_LEN];
    char status[TXT_N][TXT_LEN];
    char message[TXT_N][TXT_LEN];
    char calls[CALL_N][CALL_LEN];
    char ham_calls[HAM_N][CALL_LEN];
    char pos_name[TXT_N][TXT_LEN];
    char pos_lat[TXT_N][POS_LEN];
    char pos_lon[TXT_N][POS_LEN];
    uint32_t freq[FREQ_N];
    uint8_t bulletin_used[TXT_N];
    uint8_t status_used[TXT_N];
    uint8_t message_used[TXT_N];
    uint8_t calls_used[CALL_N];
    uint8_t pos_used[TXT_N];
    uint8_t freq_used[FREQ_N];
    uint8_t ham_ssid[HAM_N];
    bool ham_has_ssid[HAM_N];
    uint16_t ham_pass[HAM_N];
    uint8_t bulletin_n;
    uint8_t status_n;
    uint8_t message_n;
    uint8_t calls_n;
    uint8_t ham_n;
    uint8_t pos_n;
    uint8_t freq_n;
    uint8_t tx_freq_index;
    bool f_bad;
    uint8_t tx_msg_index;
    uint8_t tx_t;
    uint8_t bulletin_index;
    uint8_t status_index;
    uint8_t message_index;
    uint8_t pos_index;
      uint8_t dst_call_index;
    uint8_t d_s;
    uint8_t edit_call_index;
    uint8_t book_call_index;
    uint8_t ham_index;
    uint8_t ham_tx_index;
    uint8_t freq_index;
    uint16_t b_sel;
    uint16_t st_sel;
    uint16_t m_sel;
    uint16_t p_sel;
    uint16_t c_sel;
      uint16_t bk_sel;
    uint16_t c2_sel;
    uint16_t f_sel;
    uint16_t h_sel;
    uint16_t ht_sel;
    uint8_t txt;
    uint8_t txt_v;
    char b_edit[TXT_LEN];
    char st_edit[TXT_LEN];
    char m_edit[TXT_LEN];
    char p_name_edit[TXT_LEN];
    char p_lat_edit[POS_LEN];
    char p_lon_edit[POS_LEN];
    char c_edit[CALL_LEN];
    char f_edit[16];
    char c2_h[24];
    char freq_s[FREQ_N][16];
    uint32_t freq_edit_hz;
} FlipperHamApp;

#endif
