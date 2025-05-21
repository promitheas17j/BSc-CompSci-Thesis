// utils.h

#ifndef UTILS_H
#define UTILS_H

#include "states.h"
#include <stdint.h>
#include <WString.h>
#include <TheThingsNetwork.h>

struct ButtonDebounce {
	uint8_t stable_state;
	uint8_t last_reading;
	unsigned long last_debounce_time;
};

uint8_t debounceReadButton(uint8_t pin, struct ButtonDebounce* btn);
void log_msg(const char *msg_level, const __FlashStringHelper *msg);
void log_msg(const char *msg_level, const __FlashStringHelper *msg, const bool optional_val);
void log_msg(const char *msg_level, const __FlashStringHelper *msg, unsigned optional_val);
void log_msg(const char *msg_level, const char *msg);
// void log_msg(const char *msg_level, const char *msg, const bool optional_val);
// void log_msg(const char *msg_level, const char *msg, unsigned optional_val);
void cycle_leds();
const char* state_to_string(states s);
bool validate_message(const char *msg);
void update_led_based_on_state();

/*
	Decrease/increase a value with the Prev/Next buttons.
	Tap for single change, hold for quick continuous change. (hold+repeat)
	Returns true if 'value' changed during the current call
*/
bool handle_value_adjust_u8(
	uint8_t *value,
	uint8_t min_val,
	uint8_t max_val,
	uint8_t step = 1,
	unsigned long hold_delay = 500,
	unsigned long repeat_interval = 100
);

bool handle_value_adjust_u16(
	uint16_t *value,
	uint16_t min_val,
	uint16_t max_val,
	uint16_t step = 1,
	unsigned long hold_delay = 500,
	unsigned long repeat_interval = 100
);

/*
	To be called for setup menus that are uint8_t
	Runs a multi-step setup UI
	- labels[i]			: text for vital being set or edited i (e.g. "SYST min:")
	- values[i]			: pointer to the uint8_t being set or edited
	- lo[i], hi[i]		: min/max bounds of each vital sign being set or edited
	- addrs[i]			: address of start of EEPROM memory location for the vital being set or edited
	- count				: how many steps
	- current_state		: the current state's enum value (e.g. SETUP_BP)
	- previous_state	: state to return to when done (e.g. SETUP)

	On each call it will:
	1) clear + redraw the one line if value changed or step just advanced
	2) call handle_value_adjust() to bump the current value
	3) on SELECT rising-edge will write to EEPROM and advance to the next vital for this setup (e.g. SETUP_BP [systolic min] -> SETUP_BP [systolic max])
	4) when step == count, reset and return the previous_state
*/
states multi_threshold_setup_u8(
	const char*		prompts[],
	uint8_t*		values[],
	const uint8_t	lo[],
	const uint8_t	hi[],
	const uint8_t	addrs[],
	uint8_t			count,
	states			current_state,
	states			previous_state
);

/*
	To be called for temperature setup menu (temperature is uint16_t)
	Runs a multi-step setup UI
	- labels[i]			: text for vital being set or edited i (e.g. "SYST min:")
	- values[i]			: pointer to the uint8_t being set or edited
	- lo[i], hi[i]		: min/max bounds of each vital sign being set or edited
	- addrs[i]			: address of start of EEPROM memory location for the vital being set or edited
	- count				: how many steps
	- current_state		: the current state's enum value (e.g. SETUP_BP)
	- previous_state	: state to return to when done (e.g. SETUP)

	On each call it will:
	1) clear + redraw the one line if value changed or step just advanced
	2) call handle_value_adjust() to bump the current value
	3) on SELECT rising-edge will write to EEPROM and advance to the next vital for this setup (e.g. SETUP_BP [systolic min] -> SETUP_BP [systolic max])
	4) when step == count, reset and return the previous_state
*/

states multi_threshold_setup_u16(
	const char*		prompts[],
	uint16_t*		values[],
	const uint16_t	lo[],
	const uint16_t	hi[],
	const uint16_t	addrs[],
	uint16_t		count,
	states			current_state,
	states			previous_state
);

void message(const uint8_t *payload, size_t length, port_t port);

#endif
