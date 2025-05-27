// utils.h

// #ifndef UTILS_H
// #define UTILS_H

#pragma once

#include "globals.h"
#include "states.h"
#include <stdint.h>
// #include <EEPROM.h>
// #include <WString.h>
// #include <TheThingsNetwork.h>

struct ButtonDebounce {
	uint8_t stable_state;
	uint8_t last_reading;
	unsigned long last_debounce_time;
};

enum EventType {
	EVT_BT_CONNECTED,
	EVT_BT_DISCONNECTED,
	EVT_INVALID_VALUE,
	EVT_FAILED_READING,
	EVT_OUT_OF_BOUNDS,
	EVT_REQUEST_RECEIVED,
	EVT_THRESHOLDS_UPDATED,
	EVT_THRESHOLDS_ERROR,
	EVT_TX_FAILED,
	EVT_TX_SUCCESS
};

uint8_t debounceReadButton(uint8_t pin, struct ButtonDebounce* btn);
void log_msg(const char *msg_level, const char *msg);
void cycle_leds();
inline const char* state_to_string(states s);
bool validate_message(const char *msg);
void update_led_based_on_state();

// Unified value adjustment
template <typename T>
bool handle_value_adjust(
	T* value,
	T min_val,
	T max_val,
	T step = 1,
	unsigned long hold_delay = 500,
	unsigned long repeat_interval = 100
);

template <typename T>
inline bool handle_value_adjust(
	T* value,
	T min_val,
	T max_val,
	T step,
	unsigned long hold_delay,
	unsigned long repeat_interval
) {
	static bool prev_holding = false, next_holding = false;
	static uint8_t last_prev = LOW, last_next = LOW;
	static unsigned long prev_time = 0, next_time = 0;
	bool changed = false;
	unsigned long now = millis();
	// PREV button
	if (g_prev_button_state && !last_prev) {
		*value = (*value > min_val + step) ? *value - step : min_val;
		changed = true; prev_time = now; prev_holding = false;
	}
	else if (g_prev_button_state && last_prev) {
		if (!prev_holding && (now - prev_time) > hold_delay) {
			prev_holding = true; prev_time = now;
		}
		if (prev_holding && (now - prev_time) > repeat_interval) {
			*value = (*value > min_val + step) ? *value - step : min_val;
			changed = true; prev_time = now;
		}
	}
	else {
		prev_holding = false;
	}
	last_prev = g_prev_button_state;
	// NEXT button
	if (g_next_button_state && !last_next) {
		*value = (*value < max_val - step) ? *value + step : max_val;
		changed = true; next_time = now; next_holding = false;
	}
	else if (g_next_button_state && last_next) {
		if (!next_holding && (now - next_time) > hold_delay) {
			next_holding = true; next_time = now;
		}
		if (next_holding && (now - next_time) > repeat_interval) {
			*value = (*value < max_val - step) ? *value + step : max_val;
			changed = true; next_time = now;
		}
	}
	else {
		next_holding = false;
	}
	last_next = g_next_button_state;
	return changed;
}

/*
	Decrease/increase a value with the Prev/Next buttons.
	Tap for single change, hold for quick continuous change. (hold+repeat)
	Returns true if 'value' changed during the current call
*/
// Shared threshold setup
template <typename T>
inline states multi_threshold_setup(
	const __FlashStringHelper* prompts[],
	T* values[],
	const T lo[],
	const T hi[],
	const uint8_t addrs[],
	uint8_t count,
	states current_state,
	states previous_state
);

template <typename T> states multi_threshold_setup(
	const char* prompts[],
	T* values[],
	const T lo[],
	const T hi[],
	const uint8_t addrs[],
	uint8_t count,
	states current_state,
	states previous_state
) {
	static uint8_t step = 0, last_select = 0;
	static T last_drawn = ~T(0);  // Ensures redraw at startup
	if (g_multi_reset) {
		step = 0;
		last_drawn = ~T(0);
		last_select = g_select_button_state;
		for (uint8_t i = 0; i < count; i++) {
			if (*values[i] < lo[i] || *values[i] > hi[i]) {
				*values[i] = lo[i];
			}
		}
		g_multi_reset = false;
	}
	if (last_drawn != *values[step]) {
		lcd.clear();
		lcd.setCursor(0, 0);
		lcd.send_string(prompts[step]);
		char buf[8];
		if (sizeof(T) == 2) {
			snprintf(buf, sizeof(buf), "%2u.%1u", (*values[step] / 10), (*values[step] % 10));
		}
		else {
			snprintf(buf, sizeof(buf), "%3u", *values[step]);
		}
		lcd.setCursor(10, 0);
		lcd.send_string(buf);
		lcd.setCursor(0, 1);
		lcd.send_string(STR_THRESHOLD_NAV);
		last_drawn = *values[step];
	}
	if (handle_value_adjust(values[step], lo[step], hi[step])) {
		char buf[8];
		if (sizeof(T) == 2) {
			snprintf(buf, sizeof(buf), "%2u.%1u", (*values[step] / 10), (*values[step] % 10));
		}
		else {
			snprintf(buf, sizeof(buf), "%3u", *values[step]);
		}
		lcd.setCursor(10, 0);
		lcd.send_string(buf);
		last_drawn = *values[step];
	}
	if (g_select_button_state && !last_select) {
		if ((step & 1) && (*values[step] < *values[step - 1])) {
			lcd.clear();
			lcd.setCursor(0, 0);
			lcd.send_string("Err:Max  <  Min");
			lcd.setCursor(0, 1);
			lcd.send_string("Re-enter!");
			delay(2000);
			step--;
			last_drawn = ~T(0);
			last_select = g_select_button_state;
			return current_state;
		}
		EEPROM.write(addrs[step], (uint8_t)*values[step]);
		if (sizeof(T) == 2) {
			EEPROM.write(addrs[step] + 1, (uint8_t)(*values[step] >> 8));
		}
		step++;
		last_drawn = ~T(0);
		if (step >= count) {
			step = 0;
			return previous_state;
		}
	}
	last_select = g_select_button_state;
	return current_state;
}

void onDownlinkMessage(const uint8_t *payload, size_t length, port_t port);
void add_to_tx_retry_queue(const uint8_t *data, uint8_t len);
void alert_request_read(const char* vital);
void send_empty_uplink();
void handle_scheduled_readings();
void notify_event(EventType event);
void beep(uint16_t freq, uint16_t duration);
void blink_led(uint8_t pin, uint8_t count, uint16_t duration);

// #endif
