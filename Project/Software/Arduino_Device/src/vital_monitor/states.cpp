// states.cpp

#include "states.h"
#include "globals.h"
#include "utils.h"
#include <stdio.h>
#include <Arduino.h>

struct StateTable stateTable[] = {
	{DISCONNECTED, state_disconnected},
	{CONNECTED, state_connected},
	{READING, state_reading},
	{PROCESSING, state_processing},
	{TRANSMITTING, state_transmitting}
};

const uint8_t NUM_STATES = sizeof(stateTable) / sizeof(stateTable[0]);

// TODO: Implement logic for each state
states state_disconnected() {
	digitalWrite(LED_BLUE, LOW);
	// if (digitalRead(BT_STATE) == HIGH) {
	// 	change_state(CONNECTED);
	// 	return CONNECTED;
	// }
	return DISCONNECTED;
}

states state_setup() {
	digitalWrite(LED_BLUE, digitalRead(BT_STATE));
	return SETUP;
}

states state_connected() {
	log_msg("INFO", "Bluetooth device connected.");
	digitalWrite(LED_BLUE, HIGH);
	if (HM10_UART.available()) {
		Serial.write(HM10_UART.read());
	}
	if (digitalRead(BT_STATE) == LOW){
		return DISCONNECTED;
	}
	return CONNECTED;
}

states state_reading() {
	static char input_buffer[64];
	static uint8_t index = 0;
	if (index > 0 && g_previous_state != READING) {
		log_msg("DEBUG", "Resetting input buffer and index");
		log_msg("DEBUG", "Entered READING");
		char msg[64];
		snprintf(msg, sizeof(msg), "g_previous_state: %s", state_to_string(g_previous_state));
		log_msg("DEBUG", msg);
		memset(input_buffer, 0 , sizeof(input_buffer));
	}
	while (HM10_UART.available()) {
		log_msg("DEBUG", "Reading data");
		char incoming_byte = HM10_UART.read();
		char debug_char[2] = {incoming_byte, '\0'};
		log_msg("DEBUG", debug_char);
		if (incoming_byte == '\n' || (index >= 1 && input_buffer[index - 1] == '\\' && incoming_byte == 'n')) {
			if (incoming_byte == 'n' && input_buffer[index - 1] == '\\') {
				index--;
			}
			input_buffer[index] = '\0';
			size_t len = strlen(input_buffer);
			if (len > 0 && input_buffer[len - 1] == '\r') {
				input_buffer[len - 1] = '\0';
				log_msg("DEBUG", "Stripped trailing \\r from input_buffer");
			}
			for (size_t i = 0; i < strlen(input_buffer); i++) {
				char debug_char[10];
				snprintf(debug_char, sizeof(debug_char), "[%02X]", input_buffer[i]);
				log_msg("DEBUG", debug_char);
			}
			log_msg("DEBUG", "Received string: ");
			log_msg("DEBUG", input_buffer);
			if (validate_message(input_buffer)) {
				HM10_UART.println("ACK");
				log_msg("INFO", "Valid data received. ACK sent.");
				return PROCESSING;
			}
			else {
				HM10_UART.println("RETRY");
				log_msg("INFO", "Invalid data received. Retry request sent.");
			}
			index = 0;
		}
		else {
			if (index < sizeof(input_buffer) - 1) {
				input_buffer[index++] = incoming_byte;
			}
			else { // Buffer overflow
				index = 0;
				log_msg("WARN", "Input buffer overflow. Resetting.");
			}
		}
	}
	return READING;
}

states state_processing() {
	return TRANSMITTING;
	// return PROCESSING;
}

states state_transmitting() {
	return CONNECTED;
	// return TRANSMITTING;
}

void change_state(states new_state) {
	g_current_state = new_state;
	g_current_option_index = 0;
	g_last_option_index_displayed = 255;
	g_selection_pending = false; // prevent "bouncing" of multiple menu options after selection
	char msg[64];
	// snprintf(msg, sizeof(msg), "Current state: %s", state_to_string(g_current_state));
	// log_msg("DEBUG", msg);
	snprintf(msg, sizeof(msg), "State: %s", state_to_string(g_current_state));
	log_msg("INFO", msg);
	for (uint8_t i = 0; i < NUM_STATES; i++) {
		if (stateTable[i].state == new_state) {
			stateTable[i].func();
			break;
		}
	}
}

states check_bt_connection(states current_state) {
	const unsigned long stable_threshold = 3000; // 3 seconds stable HIGH
	const unsigned long check_interval = 500;
	static unsigned long last_high_time = 0;
	static unsigned long last_check_time = 0;
	static bool is_connected = false;
	uint8_t bt_pin_state = digitalRead(BT_STATE);
	// char msg[1024];
	// snprintf(msg, sizeof(msg), "bt_pin_state: %u\tis_connected: %d\tlast_high_time: %lu", bt_pin_state, is_connected, last_high_time);
	// log_msg("DEBUG",msg);
	if (current_state == DISCONNECTED) {
		is_connected = false;
	}
	if ((millis() - last_check_time) < check_interval) {
		return current_state;
	}
	// States that ignore connection changes completely:
	if (current_state == PROCESSING || current_state == TRANSMITTING || current_state == SETUP) {
		return current_state;
	}
	if (bt_pin_state == HIGH) {
		if (!is_connected) {
			if ((millis() - last_high_time) > stable_threshold) {
				is_connected = true;
				log_msg("DEBUG", "BT connection detected.");
				log_msg("DEBUG", "Transitioning to CONNECTED state");
				return CONNECTED;
			}
		}
	}
	else {
		last_high_time = millis();
		if (is_connected) {
			is_connected = false;
			log_msg("DEBUG", "BT disconnection detected.");
			log_msg("DEBUG", "Transitioning to DISCONNECTED state");
			return DISCONNECTED;
		}
	}
	return current_state;
}
