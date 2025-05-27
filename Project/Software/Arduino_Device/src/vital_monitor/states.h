// states.h

#pragma once

#include <stdint.h>

typedef enum {
	DISCONNECTED = 0,
	SETUP,
	CONNECTED,
	READING,
	PROCESSING,
	TRANSMITTING,
	SETUP_BP,
	SETUP_TEMP,
	SETUP_HR
} states;

typedef states (*StateFunc)();

struct StateTable {
	states state;
	StateFunc func;
};

extern struct StateTable stateTable[];
extern const uint8_t NUM_STATES;

extern states state_disconnected();
extern states state_setup();
extern states state_setup_bp();
extern states state_setup_temp();
extern states state_setup_hr();
extern states state_connected();
extern states state_reading();
extern states state_processing();
extern states state_transmitting();

void change_state(states new_state);
states check_bt_connection(states current_state);
