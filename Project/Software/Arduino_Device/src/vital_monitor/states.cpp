// states.cpp

#include "states.h"
#include "globals.h"
#include "menu.h"
#include "utils.h"
#include <stdio.h>
#include <Arduino.h>
#include <EEPROM.h>

struct StateTable stateTable[] = {
	{DISCONNECTED, state_disconnected},
	{SETUP, state_setup},
	{SETUP_BP, state_setup_bp},
	{SETUP_TEMP, state_setup_temp},
	{SETUP_HR, state_setup_hr},
	{CONNECTED, state_connected},
	{READING, state_reading},
	{PROCESSING, state_processing},
	{TRANSMITTING, state_transmitting}
};

const uint8_t NUM_STATES = sizeof(stateTable) / sizeof(stateTable[0]);

// TODO: Implement logic for each state
states state_disconnected() {
	// digitalWrite(LED_BLUE, LOW);
	return DISCONNECTED;
}

states state_setup() {
	// digitalWrite(LED_BLUE, digitalRead(BT_STATE));
	return SETUP;
}

states state_setup_bp() {
	// digitalWrite(LED_BLUE, digitalRead(BT_STATE));
	static uint8_t step = 0; // 0=systolic min, 1=systolic max, 2=diastolic min, 3=diastolic max
	// static bool changed = false;
	uint8_t* current_value_ptr = nullptr;
	const char* label = "";
	// Select which variable we're editing based on step
	switch (step) {
		case 0:
			current_value_ptr = &g_bp_systolic_threshold_min;
			label = "SYS MIN";
			break;
		case 1:
			current_value_ptr = &g_bp_systolic_threshold_max;
			label = "SYS MAX";
			break;
		case 2:
			current_value_ptr = &g_bp_diastolic_threshold_min;
			label = "DIAS MIN";
			break;
		case 3:
			current_value_ptr = &g_bp_diastolic_threshold_max;
			label = "DIAS MAX";
			break;
		default:
			// All steps completed → save to EEPROM
			EEPROM.write(G_BP_SYS_MIN_ADDR, g_bp_systolic_threshold_min);
			EEPROM.write(G_BP_SYS_MAX_ADDR, g_bp_systolic_threshold_max);
			EEPROM.write(G_BP_DIAS_MIN_ADDR, g_bp_diastolic_threshold_min);
			EEPROM.write(G_BP_DIAS_MAX_ADDR, g_bp_diastolic_threshold_max);
			log_msg("INFO", "BP thresholds saved to EEPROM");
			step = 0; // reset step for next time
			return SETUP; // go back to setup menu
	}
	// Display current label and value
	char line1[16];
	char line2[16];
	snprintf(line1, sizeof(line1), "%s: %u", label, *current_value_ptr);
	snprintf(line2, sizeof(line2), "< +  SELECT  ->");
	lcd_print_line(line1, true);
	// Button handling
	// static uint8_t last_prev_state = 0, last_next_state = 0, last_select_state = 0;
	// if (g_prev_button_state && !last_prev_state) {
	// 	if (*current_value_ptr > 0) (*current_value_ptr)--;
	// 	changed = true;
	// }
	// if (g_next_button_state && !last_next_state) {
	// 	if (*current_value_ptr < 255) (*current_value_ptr)++;
	// 	changed = true;
	// }
	// if (g_select_button_state && !last_select_state) {
	// 	log_msg("DEBUG", "SELECT pressed: moving to next step");
	// 	step++;
	// }
	// last_prev_state = g_prev_button_state;
	// last_next_state = g_next_button_state;
	// last_select_state = g_select_button_state;
	return SETUP_BP;
}

states state_setup_temp() {
	// digitalWrite(LED_BLUE, digitalRead(BT_STATE));
	return SETUP_TEMP;
}

states state_setup_hr() {
	// digitalWrite(LED_BLUE, digitalRead(BT_STATE));
	return SETUP_HR;
}

states state_connected() {
	log_msg("INFO", "Bluetooth device connected.");
	// digitalWrite(LED_BLUE, HIGH);
	return CONNECTED;
}

states state_reading() {
	char input_buffer[64];
	uint8_t index = 0;
	static unsigned long last_high_time = 0;
	const unsigned long disconnect_threshold = 2000; // 2 seconds of LOW before triggering
	uint8_t bt_pin_state = digitalRead(BT_STATE);
	if (bt_pin_state == HIGH) {
		last_high_time = millis();
	}
	else {
		if ((millis() - last_high_time) > disconnect_threshold) {
			log_msg("WARN", "Bluetooth disconnected during READING");
			return DISCONNECTED;
		}
	}
	while (HM10_UART.available()) {
		log_msg("DEBUG", "Reading data");
		char incoming_byte = HM10_UART.read();
		char debug_char[2] = {incoming_byte, '\0'};
		log_msg("DEBUG", debug_char);
		if (incoming_byte == '\n') {
			input_buffer[index] = '\0';
			size_t len = strlen(input_buffer);
			if (len > 0 && input_buffer[len - 1] == '\r') {
				input_buffer[len - 1] = '\0';
				log_msg("DEBUG", "Stripped trailing \\r from input_buffer");
			}
			log_msg("DEBUG", "Received string: ");
			log_msg("DEBUG", input_buffer);
			if (validate_message(input_buffer)) {
				HM10_UART.println("ACK");
				log_msg("INFO", "Valid data received. ACK sent.");
				strncpy(g_received_data_buffer, input_buffer, sizeof(g_received_data_buffer));
				g_received_data_buffer[sizeof(g_received_data_buffer) - 1] = '\0'; // ensure that the last character is the null terminator no matter what
				return PROCESSING;
			}
			else {
				HM10_UART.println("RETRY");
				log_msg("INFO", "Invalid data received. Retry request sent.");
			}
			index = 0;
		}
		else if (index < sizeof(input_buffer) - 1) {
			input_buffer[index++] = incoming_byte;
		}
		else { // Buffer overflow
			index = 0;
			log_msg("WARN", "Input buffer overflow. Resetting.");
		}
	}
	return READING;
}

states state_processing() {
	char msg[64];
	snprintf(msg, sizeof(msg), "Data buffer contains:\n\t %s", g_received_data_buffer);
	log_msg("DEBUG", msg);
	return TRANSMITTING;
}

states state_transmitting() {
	return CONNECTED;
}

void change_state(states new_state) {
	if (new_state != g_current_state) {
		g_previous_state = g_current_state;
	}
	g_current_state = new_state;
	g_current_option_index = 0;
	g_last_option_index_displayed = 255;
	g_selection_pending = false; // prevent "bouncing" of multiple menu options after selection
	if (g_current_state != SETUP_BP && g_current_state != SETUP_TEMP && g_current_state != SETUP_HR) {
		lcd_print_line(menu_table[(uint8_t)g_current_state].options[0], menu_table[(uint8_t)g_current_state].num_options > 1);
	}
	char msg[64];
	snprintf(msg, sizeof(msg), "State: %s", state_to_string(g_current_state));
	log_msg("INFO", msg);
	if (new_state == READING) {
		while (HM10_UART.available()) {
			HM10_UART.read();
		}
		log_msg("DEBUG", "Flushed UART buffer before entering READING state");
	}
	update_led_based_on_state();
}

states check_bt_connection(states current_state) {
	// const unsigned long stable_threshold = 3000, check_interval = 500; // 3 second HIGH, only check 2 times per second
	const unsigned long stable_threshold = 1000, stabilisation_period = 5000; // 3 second HIGH, wait 5 seconds after boot before checking
	static unsigned long last_high_time = 0;
	// static unsigned long last_check_time = 0;
	static bool is_connected = false;
	uint8_t bt_pin_state = digitalRead(BT_STATE);
	if ((millis() - g_startup_time) < stabilisation_period) {
		return current_state;
	}

	// States that ignore connection changes completely:
	if (current_state == PROCESSING || current_state == TRANSMITTING || current_state == SETUP) {
		return current_state;
	}
	if (bt_pin_state == HIGH) {
		if (!is_connected && (millis() - last_high_time > stable_threshold)) {
			is_connected = true;
			log_msg("DEBUG", "BT connection detected.");
			return CONNECTED;
		}
	}
	else {
		last_high_time = millis();
		if (is_connected) {
			is_connected = false;
			log_msg("DEBUG", "BT disconnection detected.");
			return DISCONNECTED;
		}
	}
	return current_state;
}
