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
	Serial.println(buffer);
}

void log_msg(const char *msg_level, const char *msg, const bool optional_val) {
	if (!debug_enabled && strcmp(msg_level, "DEBUG") == 0) {
		return;
	}
	char buffer[128];
	snprintf(buffer, sizeof(buffer), "[%s]: %s %s", msg_level, msg, optional_val? "true" : "false");
	Serial.println(buffer);
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
