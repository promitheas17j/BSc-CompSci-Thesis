// menu.cpp

#include "menu.h"
#include "states.h"
#include "utils.h"
#include "globals.h"
#include "Waveshare_LCD1602.h"
#include <string.h>
#include <stdio.h>

extern Waveshare_LCD1602 lcd; // defined in vital_monitor.ino

const char *menu_disconnected[] = {"Scan", "Setup"};
const char *menu_connected[] = {"Read", "Disconnect"};
const char *menu_reading[] = {"Cancel reading"};
const char *menu_processing[] = {"Abort process"};
const char *menu_transmitting[] = {"Abort Tx"};

struct Menu menu_table[] = {
	{menu_disconnected, sizeof(menu_disconnected) / sizeof(menu_disconnected[0])},
	{menu_connected, sizeof(menu_connected) / sizeof(menu_connected[0])},
	{menu_reading, sizeof(menu_reading) / sizeof(menu_reading[0])},
	{menu_processing, sizeof(menu_processing) / sizeof(menu_processing[0])},
	{menu_transmitting, sizeof(menu_transmitting) / sizeof(menu_transmitting[0])}
};

void handle_menu(states current_state) {
	uint8_t state_index = (uint8_t)current_state;
	const char **options = menu_table[state_index].options;
	uint8_t num_options = menu_table[state_index].num_options;
	uint8_t result = handle_menu_options_buttons(options, num_options);
	if (result != 255) {
		char msg[64];
		snprintf(msg, sizeof(msg), "Selected option: %s", options[result]);
		log_msg("INFO", msg);
		if (current_state == DISCONNECTED) {
			if (strcmp(options[result], "Setup") == 0) {
				change_state(CONNECTED);
				state_connected();
			}
			else if (strcmp(options[result], "Scan") == 0) {
				change_state(CONNECTED);
				state_connected();
				// log_msg("INFO", "Scan selected - not implemented yet.");
			}
		}
		else if (current_state == CONNECTED) {
			if (strcmp(options[result], "Disconnect") == 0) {
				change_state(DISCONNECTED);
				state_disconnected();
			}
			else if (strcmp(options[result], "Read") == 0) {
				log_msg("INFO", "Read selected - not implemented yet.");
			}
		}

	}
}

uint8_t handle_menu_options_buttons(const char **options, uint8_t num_options) {
	if (g_current_option_index != g_last_option_index_displayed) {
		lcd_print_line(options[g_current_option_index]);
		g_last_option_index_displayed = g_current_option_index;
	}
	// Edge detection - remember last button states (only do something when button state changes)
	static uint8_t last_prev_btn_state = 0;
	static uint8_t last_next_btn_state = 0;
	static uint8_t last_select_btn_state = 0;
	// PREV button rising edge
	if (g_prev_button_state && !last_prev_btn_state) {
		log_msg("DEBUG", "PREV PRESSED");
		g_current_option_index = (g_current_option_index == 0) ? num_options - 1 : g_current_option_index - 1;
	}
	// NEXT button rising edge
	if (g_next_button_state && !last_next_btn_state) {
		log_msg("DEBUG", "NEXT PRESSED");
		g_current_option_index = (g_current_option_index + 1) % num_options;
	}
	// SELECT button rising edge
	if (g_select_button_state && !last_select_btn_state) {
		log_msg("DEBUG", "SELECT PRESSED");
		return g_current_option_index;
	}
	// Update last button states
	last_prev_btn_state = g_prev_button_state;
	last_next_btn_state = g_next_button_state;
	last_select_btn_state = g_select_button_state;
	return 255; // 255 = nothing selected yet
}

void lcd_print_line(const char *option) {
	lcd.clear();
	lcd.setCursor(0, 0);
	lcd.send_string(option);  
	lcd.setCursor(0, 1);
	lcd.send_string("<    SELECT    >");
}

