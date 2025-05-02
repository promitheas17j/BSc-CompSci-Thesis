// globals.h

#ifndef GLOBALS_H
#define GLOBALS_H

#include <stdint.h>
#include <stdbool.h>
#include <SoftwareSerial.h>
#include "utils.h"
#include "states.h"

#define BTN_PREV 4
#define BTN_SELECT 7
#define BTN_NEXT 8
#define LED_BLUE A5
#define LED_GREEN A4
#define LED_YELLOW A3
#define LED_RED A2
#define BT_STATE 11

extern uint8_t g_prev_button_state;
extern uint8_t g_select_button_state;
extern uint8_t g_next_button_state;
extern states g_current_state;
extern uint8_t g_current_option_index;
extern uint8_t g_last_option_index_displayed;

extern struct ButtonDebounce g_prev_button;
extern struct ButtonDebounce g_select_button;
extern struct ButtonDebounce g_next_button;

extern bool debug_enabled;
extern bool g_selection_pending;

extern SoftwareSerial HM10_UART;

#endif
