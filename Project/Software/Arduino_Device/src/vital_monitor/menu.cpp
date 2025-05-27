// menu.cpp

#include "globals.h"
#include "menu.h"
#include "states.h"
#include "utils.h"
#include "Waveshare_LCD1602.h"
#include <string.h>
#include <stdio.h>
#include <Arduino.h>

extern Waveshare_LCD1602 lcd; // defined in vital_monitor.ino

const char *menu_disconnected[] = {"No connection", "Setup"};
const char *menu_setup[] = {"Setup BP", "Setup Temp", "Setup HR", "Back"};
const char *menu_connected[] = {"Read", "Setup", "Disconnect"};
const char *menu_reading[] = {"Reading..."};
const char *menu_processing[] = {"Processing..."};
const char *menu_transmitting[] = {"Sending..."};

struct Menu menu_table[] = {
	{menu_disconnected, sizeof(menu_disconnected) / sizeof(menu_disconnected[0])},
	{menu_setup, sizeof(menu_setup) / sizeof(menu_setup[0])},
	{menu_connected, sizeof(menu_connected) / sizeof(menu_connected[0])},
	{menu_reading, sizeof(menu_reading) / sizeof(menu_reading[0])},
	{menu_processing, sizeof(menu_processing) / sizeof(menu_processing[0])},
	{menu_transmitting, sizeof(menu_transmitting) / sizeof(menu_transmitting[0])}
};

states handle_menu(states current_state) {
	uint8_t state_index = (uint8_t)current_state;
	const char **options = menu_table[state_index].options;
	// const char *const *options = menu_table[state_index].options;
	uint8_t num_options = menu_table[state_index].num_options;
	uint8_t result = handle_menu_options_buttons(options, num_options);
	if (result == 255 && !g_selection_pending && !g_select_button_state) {
		g_selection_pending = true;
	}
	if ((result != 255) && (g_selection_pending)) {
		if (current_state == DISCONNECTED && (strcmp(options[result], "Setup") == 0)) {
				g_selection_pending = false;
				return SETUP;
		}
		else if (current_state == SETUP) {
			if (strcmp(options[result], "Setup BP") == 0) {
				g_selection_pending = false;
				return SETUP_BP;
			}
			else if (strcmp(options[result], "Setup Temp") == 0) {
				g_selection_pending = false;
				return SETUP_TEMP;

			}
			else if (strcmp(options[result], "Setup HR") == 0) {
				g_selection_pending = false;
				return SETUP_HR;

			}
			else if (strcmp(options[result], "Back") == 0) {
				g_selection_pending = false;
				return g_setup_caller_state;
			}
		}
		else if (current_state == CONNECTED) {
			if (strcmp(options[result], "Read") == 0) {
				g_selection_pending = false;
				return READING;
			}
			else if (strcmp(options[result], "Setup") == 0) {
				g_selection_pending = false;
				return SETUP;
			}
			else if (strcmp(options[result], "Disconnect") == 0) {
				g_selection_pending = false;
				digitalWrite(BRK_PIN, HIGH);
				delay(2000);
				// digitalWrite(BRK_PIN, HIGH);
				return DISCONNECTED;
			}
		}
	}
	return current_state;
}

uint8_t handle_menu_options_buttons(const char **options, uint8_t num_options) {
	bool interactive = (num_options > 1);
	if (g_current_option_index != g_last_option_index_displayed) {
		lcd_print_line(options[g_current_option_index], interactive);
		g_last_option_index_displayed = g_current_option_index;
	}
	if (!interactive) {
		return 255; // if not an interactive menu but just a static one, simply return 255 and skip button handling
	}
	// Edge detection - remember last button states (only do something when button state changes)
	static uint8_t last_prev_btn_state = 0, last_next_btn_state = 0, last_select_btn_state = 0;
	// PREV button rising edge
	if (g_prev_button_state && !last_prev_btn_state) {
		g_current_option_index = (g_current_option_index == 0) ? num_options - 1 : g_current_option_index - 1;
	}
	// NEXT button rising edge
	if (g_next_button_state && !last_next_btn_state) {
		g_current_option_index = (g_current_option_index + 1) % num_options;
	}
	// SELECT button rising edge
	if (g_select_button_state && !last_select_btn_state) {
		return g_current_option_index;
	}
	// Update last button states
	last_prev_btn_state = g_prev_button_state;
	last_next_btn_state = g_next_button_state;
	last_select_btn_state = g_select_button_state;
	return 255; // 255 = nothing selected yet
}

void lcd_print_line(const char *option, bool is_interactive) {
	lcd.clear();
	lcd.setCursor(0, 0);
	lcd.send_string(option);
	// char buffer[17];
	// strncpy_P(buffer, (PGM_P)option, sizeof(buffer) - 1);
	// buffer[sizeof(buffer) - 1] = '\0';
	// lcd.send_string(buffer);
	lcd.setCursor(0, 1);
	if (is_interactive) {
		lcd.send_string(STR_MENU_NAV);
	}
	else {
		lcd.send_string("");
	}
}

