// utils.h

#ifndef UTILS_H
#define UTILS_H

#include "states.h"
#include <stdint.h>

struct ButtonDebounce {
	uint8_t stable_state;
	uint8_t last_reading;
	unsigned long last_debounce_time;
};

uint8_t debounceReadButton(uint8_t pin, struct ButtonDebounce* btn);
void log_msg(const char *msg_level, const char *msg);
void log_msg(const char *msg_level, const char *msg, const bool optional_val);
void cycle_leds();
const char* state_to_string(states s);

#endif
