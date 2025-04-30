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
	log_msg("INFO", "State: DISCONNECTED");
	digitalWrite(LED_BLUE, LOW);
	return DISCONNECTED;
}

states state_connected() {
	log_msg("INFO", "State: CONNECTED");
	digitalWrite(LED_BLUE, HIGH);
	return CONNECTED;
}

states state_reading() {
	log_msg("INFO", "State: READING");
	return READING;
}

states state_processing() {
	log_msg("INFO", "State: PROCESSING");
	return PROCESSING;
}

states state_transmitting() {
	log_msg("INFO", "State: TRANSMITTING");
	return TRANSMITTING;
}

void change_state(states new_state) {
	g_current_state = new_state;
	g_current_option_index = 0;
	g_last_option_index_displayed = 255;
	char msg[64];
	snprintf(msg, sizeof(msg), "Current state: %s", state_to_string(g_current_state));
	log_msg("DEBUG", msg);
}
