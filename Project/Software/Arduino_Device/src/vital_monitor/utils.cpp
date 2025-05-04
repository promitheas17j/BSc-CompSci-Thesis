// utils.cpp

#include "utils.h"
#include "globals.h"
#include "states.h"
#include <Arduino.h>
#include <string.h>
#include <stdio.h>

uint8_t debounceReadButton(uint8_t pin, struct ButtonDebounce* btn) {
	const unsigned long debounceDelay = 20;
	uint8_t reading = digitalRead(pin);
	// If reading changed, reset timer
	if (reading != btn->last_reading) {
		btn->last_debounce_time = millis();
		btn->last_reading = reading;
	}
	// If stable for debounceDelay, update stable state
	if ((millis() - btn->last_debounce_time) > debounceDelay) {
		btn->stable_state = btn->last_reading;
	}
	return btn->stable_state;
}

void log_msg(const char *msg_level, const char *msg) {
	if (!debug_enabled && strcmp(msg_level, "DEBUG") == 0) {
		return;
	}
	char buffer[64];
	snprintf(buffer, sizeof(buffer), "[%s]: %s", msg_level, msg);
	Serial.print(buffer);
	Serial.print("\n");
}

void log_msg(const char *msg_level, const char *msg, const bool optional_val) {
	if (!debug_enabled && strcmp(msg_level, "DEBUG") == 0) {
		return;
	}
	char buffer[128];
	snprintf(buffer, sizeof(buffer), "[%s]: %s %s", msg_level, msg, optional_val? "true" : "false");
	Serial.print(buffer);
	Serial.print("\n");
}

void cycle_leds() {
	uint8_t cycle_time = 100;
	digitalWrite(LED_BLUE, HIGH);
	delay(cycle_time);
	digitalWrite(LED_BLUE, LOW);
	delay(cycle_time);
	digitalWrite(LED_GREEN, HIGH);
	delay(cycle_time);
	digitalWrite(LED_GREEN, LOW);
	delay(cycle_time);
	digitalWrite(LED_YELLOW, HIGH);
	delay(cycle_time);
	digitalWrite(LED_YELLOW, LOW);
	delay(cycle_time);
	digitalWrite(LED_RED, HIGH);
	delay(cycle_time);
	digitalWrite(LED_RED, LOW);
}

const char* state_to_string(states s) {
	switch (s) {
		case DISCONNECTED: return "DISCONNECTED";
		case CONNECTED: return "CONNECTED";
		case READING: return "READING";
		case PROCESSING: return "PROCESSING";
		case TRANSMITTING: return "TRANSMITTING";
		default: return "UNKNOWN";
	}
}

bool validate_message(const char *msg) {
	if (strncmp(msg, "BP:", 3) == 0) {
		log_msg("DEBUG", "Received data is from a BP device.");
		const char* data = msg + 3;
		int systolic, diastolic, consumed = 0;
		if (sscanf(data, "%d/%d%n", &systolic, &diastolic, &consumed) == 2) {
			if (data[consumed] == '\0') {
				return true;
			}
		}
	}
	else if (strncmp(msg, "TEMP:", 5) == 0) {
		log_msg("DEBUG", "Received data is from a TEMP device.");
		const char* data = msg + 5;
		char* endptr;
		strtod(data, &endptr);
		if (endptr != data && *endptr == '\0') {
			return true;
		}
	}
	else if (strncmp(msg, "HR:", 3) == 0) {
		log_msg("DEBUG", "Received data is from a HR device.");
		const char* data = msg + 3;
		char* endptr;
		strtol(data, &endptr, 10);
		if (endptr != data && *endptr == '\0') {
			return true;
		}
	}
	return false;
}

void update_led_based_on_state() {
	if (g_current_state == DISCONNECTED) {
		digitalWrite(LED_BLUE, LOW);
	}
	else if (g_current_state == CONNECTED) {
		digitalWrite(LED_BLUE, HIGH);
	}
	else if (g_current_state == SETUP) {
		if (g_previous_state == CONNECTED) {
			digitalWrite(LED_BLUE, HIGH);
		}
		else {
			digitalWrite(LED_BLUE, LOW);
		}
	}
	// else if (g_current_state == READING) {
	// 	if (g_previous_state == CONNECTED || g_previous_state == READING) {
	// 		digitalWrite(LED_BLUE, HIGH);
	// 	}
	// 	else {
	// 		digitalWrite(LED_BLUE, LOW);
	// 	}
	// }
	// else if (g_current_state == PROCESSING || g_current_state == TRANSMITTING) {
	// 	// Keep LED unchanged.
	// }
}
