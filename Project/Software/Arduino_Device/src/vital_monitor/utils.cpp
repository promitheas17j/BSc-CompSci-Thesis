// utils.cpp

#include "globals.h"
#include "utils.h"
#include "states.h"
#include <Arduino.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <TheThingsNetwork.h>

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

void log_msg(const char *msg_level, const __FlashStringHelper *msg) {
	if (!debug_enabled && strcmp(msg_level, "DEBUG") == 0) {
		return;
	}
	Serial.print("[");
	Serial.print(msg_level);
	Serial.print("]: ");
	Serial.println(msg);
}

void log_msg(const char *msg_level, const __FlashStringHelper *msg, const bool optional_val) {
	Serial.print("[");
	Serial.print(msg_level);
	Serial.print("] ");
	Serial.print(msg);
	Serial.println(optional_val ? "true" : "false");
}

void log_msg(const char *msg_level, const __FlashStringHelper *msg, unsigned optional_val) {
	Serial.print("[");
	Serial.print(msg_level);
	Serial.print("] ");
	Serial.print(msg);
	Serial.println(optional_val);
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
		case SETUP: return "SETUP";
		case SETUP_BP: return "SETUP_BP";
		case SETUP_TEMP: return "SETUP_TEMP";
		case SETUP_HR: return "SETUP_HR";
		case CONNECTED: return "CONNECTED";
		case READING: return "READING";
		case PROCESSING: return "PROCESSING";
		case TRANSMITTING: return "TRANSMITTING";
		default: return "UNKNOWN";
	}
}

bool validate_message(const char *msg) {
	if (strncmp(msg, "BP:", 3) == 0) {
		// BP:xyz/pq\n\0
		if (strlen(msg) > G_RECEIVED_DATA_BUFFER_SIZE) {
			return false;
		}
		const char *p = msg + 3;
		uint8_t digit_count = 0;
		bool seen_slash = false;
		for ( ; *p != '\0'; ++p) {
			if (isdigit((unsigned char)*p)) {
				// count consecutive digits
				digit_count++;
				// if we ever exceed 3 then fail immediately
				if (digit_count > 3) {
					return false;
				}
			}
			else if (*p == '/' && !seen_slash) {
				if (digit_count < 2) {
					return false;
				}
				// reset
				digit_count = 0;
				seen_slash = true;
			}
			else {
				return false;
			}
		}
		if (!seen_slash || digit_count < 2) {
			// loop ends at '\0' if no slash seen by then or digit_count is less than 2 then its not a properly formatted BP string
			return false;
		}
		return true;
	}
	else if (strncmp(msg, "TEMP:", 5) == 0) {
		// TEMP:xy.z\n\0
		if (strlen(msg) > G_RECEIVED_DATA_BUFFER_SIZE) {
			return false;
		}
		const char *p = msg + 5;
		if (strlen(p) != 4) {
			return false;
		}
		if (!isdigit((unsigned char)p[0]) || !isdigit((unsigned char)p[1])) {
			return false;
		}
		if (p[2] != '.') {
			return false;
		}
		if (!isdigit((unsigned char)p[3])) {
			return false;
		}
		return true;
	}
	else if (strncmp(msg, "HR:", 3) == 0) {
		// HR:xy\n\0
		if (strlen(msg) > 7) { // Other 2 use G_RECEIVED_DATA_BUFFER_SIZE because they are both the same length, HR is only 7 characters total
			return false;
		}
		const char *p = msg + 3;
		uint8_t digit_count = 0;
		for ( ; *p != '\0'; ++p) {
			if (!isdigit((unsigned char)*p)) {
				return false;
			}
			if (++digit_count > 3) {
				return false;
			}
		}
		if (digit_count < 2) {
			return false;
		}
		return true;
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
}


// bool handle_value_adjust_u8(uint8_t *value, uint8_t min_val, uint8_t max_val, uint8_t step, unsigned long hold_delay, unsigned long repeat_interval) {
// 	static bool prev_holding = false, next_holding = false;
// 	static uint8_t last_previous_btn = LOW, last_next_btn = LOW;
// 	static unsigned long prev_time = 0, next_time = 0;
// 	// static unsigned long previous_last_repeat_time = 0, next_last_repeat_time = 0;
// 	bool changed = false;
// 	unsigned long now = millis();
// 	// PREV (decrement)
// 	if (g_prev_button_state && !last_previous_btn) {
// 		// rising edge: one step
// 		if (*value > min_val + step) {
// 			*value -= step;
// 		}
// 		else {
// 			*value = min_val;
// 		}
// 		prev_holding = false;
// 		prev_time = now;
// 		changed = true;
// 	}
// 	else if (g_prev_button_state && last_previous_btn) {
// 		// held?
// 		if (!prev_holding && (now - prev_time) > hold_delay) {
// 			prev_holding = true;
// 			prev_time = now;
// 		}
// 		if (prev_holding && (now - prev_time) > repeat_interval) {
// 			if (*value > min_val + step) {
// 				*value -= step;
// 			}
// 			else {
// 				*value = min_val;
// 			}
// 			changed = true;
// 			prev_time = now;
// 		}
// 	}
// 	else {
// 		// released
// 		prev_holding = false;
// 	}
// 	last_previous_btn = g_prev_button_state;
// 	// NEXT (increment)
// 	if (g_next_button_state && !last_next_btn) {
// 		// rising edge: one step
// 		if (*value < (max_val - step)) {
// 			*value += step;
// 		}
// 		else {
// 			*value = max_val;
// 		}
// 		changed = true;
// 		next_holding = false;
// 		next_time = now;
// 	}
// 	else if (g_next_button_state && last_next_btn) {
// 		// held?
// 		if (!next_holding && (now - next_time) > hold_delay) {
// 			next_holding = true;
// 			next_time = now;
// 		}
// 		if (next_holding && (now - next_time) > repeat_interval) {
// 			if (*value < (max_val - step)) {
// 				*value += step;
// 			}
// 			else {
// 				*value = max_val;
// 			}
// 			changed = true;
// 			next_time = now;
// 		}
// 	}
// 	else {
// 		// released
// 		next_holding = false;
// 	}
// 	last_next_btn = g_next_button_state;
// 	return changed;
// }

// bool handle_value_adjust_u16(uint16_t *value, uint16_t min_val, uint16_t max_val, uint16_t step, unsigned long hold_delay, unsigned long repeat_interval) {
// 	static bool prev_holding = false, next_holding = false;
// 	static uint8_t last_previous_btn = LOW, last_next_btn = LOW;
// 	static unsigned long prev_time = 0, next_time = 0;
// 	// static unsigned long previous_last_repeat_time = 0, next_last_repeat_time = 0;
// 	bool changed = false;
// 	unsigned long now = millis();
// 	// PREV (decrement)
// 	if (g_prev_button_state && !last_previous_btn) {
// 		// rising edge: one step
// 		if (*value > min_val + step) {
// 			*value -= step;
// 		}
// 		else {
// 			*value = min_val;
// 		}
// 		prev_holding = false;
// 		prev_time = now;
// 		changed = true;
// 	}
// 	else if (g_prev_button_state && last_previous_btn) {
// 		// held?
// 		if (!prev_holding && (now - prev_time) > hold_delay) {
// 			prev_holding = true;
// 			prev_time = now;
// 		}
// 		if (prev_holding && (now - prev_time) > repeat_interval) {
// 			if (*value > min_val + step) {
// 				*value -= step;
// 			}
// 			else {
// 				*value = min_val;
// 			}
// 			changed = true;
// 			prev_time = now;
// 		}
// 	}
// 	else {
// 		// released
// 		prev_holding = false;
// 	}
// 	last_previous_btn = g_prev_button_state;
// 	// NEXT (increment)
// 	if (g_next_button_state && !last_next_btn) {
// 		// rising edge: one step
// 		if (*value < (max_val - step)) {
// 			*value += step;
// 		}
// 		else {
// 			*value = max_val;
// 		}
// 		changed = true;
// 		next_holding = false;
// 		next_time = now;
// 	}
// 	else if (g_next_button_state && last_next_btn) {
// 		// held?
// 		if (!next_holding && (now - next_time) > hold_delay) {
// 			next_holding = true;
// 			next_time = now;
// 		}
// 		if (next_holding && (now - next_time) > repeat_interval) {
// 			if (*value < (max_val - step)) {
// 				*value += step;
// 			}
// 			else {
// 				*value = max_val;
// 			}
// 			changed = true;
// 			next_time = now;
// 		}
// 	}
// 	else {
// 		// released
// 		next_holding = false;
// 	}
// 	last_next_btn = g_next_button_state;
// 	return changed;
// }


// states multi_threshold_setup_u8(
// 	const char*		prompts[],
// 	uint8_t*		values[],
// 	const uint8_t	lo[],
// 	const uint8_t	hi[],
// 	const uint8_t	addrs[],
// 	uint8_t			count,
// 	states			current_state,
// 	states			previous_state
// 	) {
// 	// remember which step we're on and track redraw
// 	static uint8_t step, last_drawn, last_select;
// 	const uint8_t step_size = 1;
// 	// track select-button edge
// 	if (g_multi_reset) {
// 		step = 0;
// 		last_drawn = 0xFF;
// 		last_select = g_select_button_state;
// 		for (uint8_t i = 0; i < count; i++) {
// 			if (*values[i] < lo[i] || *values[i] > hi[i]) {
// 				*values[i] = lo[i];
// 			}
// 		}
// 		g_multi_reset = false;
// 	}
// 	// if we need to redraw the menu line:
// 	if (last_drawn != *values[step]) {
// 		lcd.clear();
// 		lcd.setCursor(0, 0);
// 		lcd.send_string(prompts[step]);
// 		// right-justify 3 digits at column 10
// 		char buf[4];
// 		snprintf(buf, sizeof(buf), "%3u", *values[step]);
// 		lcd.setCursor(10, 0);
// 		lcd.send_string(buf);
// 		lcd.setCursor(0, 1);
// 		lcd.send_string("< -  SELECT  + >");
// 		last_drawn = *values[step];
// 	}
// 	// allow -/+ tap or hold
// 	if (handle_value_adjust_u8(values[step], lo[step], hi[step], step_size)) {
// 		// value moved --> just rewrite the digits
// 		char buf[4];
// 		snprintf(buf, sizeof(buf), "%3u", *values[step]);
// 		lcd.setCursor(10, 0);
// 		lcd.send_string(buf);
// 		last_drawn = *values[step];
// 	}
// 	// SELECT rising edge
// 	if (g_select_button_state && !last_select) {
// 		if ((step & 1) && (*values[step] < *values[step - 1])) {
// 			log_msg("WARN", F("Max must be >= Min. Re-enter"));
// 			lcd.clear();
// 			lcd.setCursor(0, 0);
// 			lcd.send_string("Err:Max  <  Min");
// 			lcd.setCursor(0, 1);
// 			lcd.send_string("Re-enter!");
// 			delay(2000);
// 			step--;
// 			last_drawn = 0xFF;
// 			last_select = g_select_button_state;
// 			return current_state;
// 		}
// 		// save to EEPROM
// 		for (uint8_t i = 0; i < count; i++) {
// 			uint8_t addr = addrs[i], before = EEPROM.read(addr), after = *values[i];
// 			if (before != after) {
// 				EEPROM.write(addr, after);
// 				char msg[64];
// 				snprintf(msg, sizeof(msg), "Set %s %u", prompts[i], after);
// 				log_msg("INFO", msg);
// 			}
// 		}
// 		step++;
// 		last_drawn = 0xFF;
// 		if (step >= count) {
// 			// all done
// 			step = 0;
// 			return previous_state;
// 		}
// 	}
// 	last_select = g_select_button_state;
// 	// stay in this sub-state until done
// 	return current_state;
// }

// states multi_threshold_setup_u16(
// 	const char*		prompts[],
// 	uint16_t*		values[],
// 	const uint16_t	lo[],
// 	const uint16_t	hi[],
// 	const uint16_t	addrs[],
// 	uint16_t		count,
// 	states			current_state,
// 	states			previous_state
// 	) {
// 	// remember which step we're on and track redraw
// 	static uint8_t step, last_select;
// 	static uint16_t last_drawn;
// 	const uint8_t step_size = 1;
// 	if (g_multi_reset) {
// 		step = 0;
// 		last_drawn = 0xFFFF;
// 		last_select = g_select_button_state;
// 		for (uint8_t i = 0; i < count; i++) {
// 			if (*values[i] < lo[i] || *values[i] > hi[i]) {
// 				*values[i] = lo[i];
// 			}
// 		}
// 		g_multi_reset = false;
// 	}
// 	// if we need to redraw the menu line:
// 	if (last_drawn != *values[step]) {
// 		lcd.clear();
// 		lcd.setCursor(0, 0);
// 		lcd.send_string(prompts[step]);
// 		// right-justify 3 digits at column 10
// 		// the next couple lines assume the only 16-bit value it will ever support is temperature as displaying it in the setup menu is a bit hardcoded for temperature
// 		// if will add another vital sign which needs to be 16-bit based and this function will be used, will need to edit this next part to account for that one as well
// 		char buf[6];
// 		snprintf(buf, sizeof(buf), "%2u.%1u", (*values[step] / 10), (*values[step] % 10));
// 		lcd.setCursor(10, 0);
// 		lcd.send_string(buf);
// 		lcd.setCursor(0, 1);
// 		lcd.send_string("< -  SELECT  + >");
// 		last_drawn = *values[step];
// 	}
// 	// allow -/+ tap or hold
// 	if (handle_value_adjust_u16(values[step], lo[step], hi[step], step_size)) {
// 		// value moved --> just rewrite the digits
// 		char buf[6];
// 		snprintf(buf, sizeof(buf), "%2u.%1u", (*values[step] / 10), (*values[step] % 10));
// 		lcd.setCursor(10, 0);
// 		lcd.send_string(buf);
// 		last_drawn = *values[step];
// 	}
// 	// SELECT rising edge
// 	if (g_select_button_state && !last_select) {
// 		if ((step & 1) && (*values[step] < *values[step - 1])) {
// 			log_msg("WARN", F("Max must be >= Min. Re-enter"));
// 			lcd.clear();
// 			lcd.setCursor(0, 0);
// 			lcd.send_string("Err:Max  <  Min");
// 			lcd.setCursor(0, 1);
// 			lcd.send_string("Re-enter!");
// 			delay(2000);
// 			step--;
// 			last_drawn = 0xFF;
// 			last_select = g_select_button_state;
// 			return current_state;
// 		}
// 		// save to EEPROM (16-bit version)
// 		for (uint8_t i = 0; i < count; i++) {
// 			uint8_t addr = addrs[i];
// 			uint16_t before = EEPROM.read(addr), after = *values[i];
// 			if (before != after) {
// 				EEPROM.write(addr, (uint8_t)after);
// 				EEPROM.write(addr + 1, (uint8_t)(after >> 8));
// 				char msg[64];
// 				snprintf(msg, sizeof(msg), "Set %s %u", prompts[i], after);
// 				log_msg("INFO", msg);
// 			}
// 		}
// 		step++;
// 		last_drawn = 0xFF;
// 		if (step >= count) {
// 			// all done
// 			step = 0;
// 			return previous_state;
// 		}
// 	}
// 	last_select = g_select_button_state;
// 	// stay in this sub-state until done
// 	return current_state;
// }

// Callback to handle downlink messages
void onDownlinkMessage(const uint8_t *payload, size_t length, port_t port) {
	if (port != 1 || length < 2) {
		return;
	}
	uint8_t cmd = payload[0];
	uint8_t field = payload[1];
	switch (cmd) {
		case 0x20: // Set threshold
			log_msg("DEBUG", F("0x20 - Set threshold cmd"));
			if (field >= 0x01 && field <= 0x08) {
				// Handle 1-byte values
				if (length == 3) {
					uint8_t value = payload[2];
					switch (field) {
						case 0x01:
							log_msg("DEBUG", F("0x01 - HR Min"));
							// Set min hr to value
							EEPROM.write(G_HR_MIN_ADDR, value);
							g_hr_threshold_min = value;
							break;
						case 0x02:
							log_msg("DEBUG", F("0x02 - HR Max"));
							// Set max hr to value
							EEPROM.write(G_HR_MAX_ADDR, value);
							g_hr_threshold_max = value;
							break;
						case 0x05:
							log_msg("DEBUG", F("0x05 - Sys Min"));
							// set min sys to value
							EEPROM.write(G_BP_SYS_MIN_ADDR, value);
							g_bp_systolic_threshold_min = value;
							break;
						case 0x06:
							log_msg("DEBUG", F("0x06 - Sys Max"));
							// set max sys to value
							EEPROM.write(G_BP_SYS_MAX_ADDR, value);
							g_bp_systolic_threshold_max = value;
							break;
						case 0x07:
							log_msg("DEBUG", F("0x07 - Dia Min"));
							// set min dia to value
							EEPROM.write(G_BP_DIAS_MIN_ADDR, value);
							g_bp_diastolic_threshold_min = value;
							break;
						case 0x08:
							log_msg("DEBUG", F("0x08 - Dia Max"));
							// set max dia to value
							EEPROM.write(G_BP_DIAS_MAX_ADDR, value);
							g_bp_diastolic_threshold_max = value;
							break;
						default:
							break;
					}
				}
				// Handle 2-byte temperature
				else if (length == 4 && (field == 0x03 || field == 0x04)) {
					uint16_t value = (payload[2] << 8) | payload[3];
					if (field == 0x03) {
						log_msg("DEBUG", F("0x03 - Temp Min"));
						// set min temp to value
						EEPROM.write(G_TEMP_MIN_ADDR, value);
						g_temp_threshold_min = value;
					}
					else if (field == 0x04) {
						log_msg("DEBUG", F("0x04 - Temp Max"));
						// set max temp to value
						EEPROM.write(G_TEMP_MAX_ADDR, value);
						g_temp_threshold_max = value;
					}
				}
			}
			break;
		case 0x30: // Request reading
			log_msg("DEBUG", F("0x30 - Request reading cmd"));
			switch (field) {
				case 0x01:
					log_msg("DEBUG", F("0x01 - Request BP"));
					// read_bp = true;
					alert_request_read_bp();
					break;
				case 0x02:
					log_msg("DEBUG", F("0x02 - Request TEMP"));
					// read_temp = true;
					alert_request_read_temp();
					break;
				case 0x03:
					log_msg("DEBUG", F("0x03 - Request HR"));
					// read_hr = true;
					alert_request_read_hr();
					break;
			}
			break;
		default:
			break;
	}
}

void add_to_tx_retry_queue(const uint8_t *data, uint8_t len) {
	if (len > MAX_MSG_SIZE) {
		len = MAX_MSG_SIZE;
	}

	if (tx_retry_count < MAX_QUEUE_ITEMS) {
		memcpy(tx_retry_queue[tx_retry_tail], data, len);
		tx_retry_lengths[tx_retry_tail] = len;

		tx_retry_tail = (tx_retry_tail + 1) % MAX_QUEUE_ITEMS;
		tx_retry_count++;

		log_msg("INFO", F("Added message to retry queue"));
	} else {
		log_msg("WARN", F("Retry queue full, message dropped"));
	}
}

void alert_request_read_bp() {
	log_msg("DEBUG", F("Request to measure BP"));
}

void alert_request_read_temp() {
	log_msg("DEBUG", F("Request to measure TEMP"));
}

void alert_request_read_hr() {
	log_msg("DEBUG", F("Request to measure HR"));
}

void send_empty_uplink() {
	DateTime now = RTClib::now();
	// Serial.print("Timestamp: ");
	// Serial.print(now.day());
	// Serial.print("/");
	// Serial.print(now.month());
	// Serial.print("/");
	// Serial.print(now.year());
	// Serial.print(" ");
	// Serial.print(now.hour());
	// Serial.print(":");
	// Serial.print(now.minute());
	// Serial.print(":");
	// Serial.println(now.second());
	uint32_t current_minute = now.hour() * 60 + now.minute();
	if ((current_minute % 5 == 0) && (current_minute != g_last_uplink_minute)) {
		ttn.sendBytes((const uint8_t[]){0x00}, 1, 1);
		g_last_uplink_minute = current_minute;
	}
}
