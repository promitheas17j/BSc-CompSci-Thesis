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

// void log_msg(const char *msg_level, const char *msg) {
// 	if (!debug_enabled && strcmp(msg_level, "DEBUG") == 0) {
// 		return;
// 	}
// 	char buffer[64];
// 	snprintf(buffer, sizeof(buffer), "[%s]: %s", msg_level, msg);
// 	Serial.print(buffer);
// 	Serial.print("\n");
// }

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
			// payload ends at '\0' if no slash seen by then or digit_count is less than 2 then its not a properly formatted BP string
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

// Callback to handle downlink messages
void onDownlinkMessage(const uint8_t *payload, size_t length, port_t port) {
	// If this is a time sync
	if (length == 3 && payload[0] < 24 && payload[1] < 60 && payload[2] < 60) {
		myRTC.setHour(payload[0]);
		myRTC.setMinute(payload[1]);
		myRTC.setSecond(payload[2]);
		// Serial.println("Time synced via downlink.");
	}
	if (port != 1 || length < 2) {
		return;
	}
	uint8_t cmd   = payload[0];
	uint8_t field = payload[1];
	if (cmd == 0x20) {	// Set Thresholds
		switch (field) {
			case 0x01: // HR Min/Max (1 byte each)
				if (length >= 4) {
					g_hr_threshold_min = payload[2];
					g_hr_threshold_max = payload[3];
					EEPROM.write(G_HR_MIN_ADDR, g_hr_threshold_min);
					EEPROM.write(G_HR_MAX_ADDR, g_hr_threshold_max);
				}
				break;
			case 0x02: // TEMP Min/Max (2 bytes each)
				if (length >= 6) {
					g_temp_threshold_min = (payload[2] << 8) | payload[3];
					g_temp_threshold_max = (payload[4] << 8) | payload[5];
					EEPROM.write(G_TEMP_MIN_ADDR, (uint8_t)g_temp_threshold_min);
					EEPROM.write(G_TEMP_MIN_ADDR + 1, (uint8_t)(g_temp_threshold_min >> 8));
					EEPROM.write(G_TEMP_MAX_ADDR, (uint8_t)g_temp_threshold_max);
					EEPROM.write(G_TEMP_MAX_ADDR + 1, (uint8_t)(g_temp_threshold_max >> 8));
				}
				break;
			case 0x03: // BP SYS/DIA Min/Max (1 byte each)
				if (length >= 6) {
					g_bp_systolic_threshold_min  = payload[2];
					g_bp_systolic_threshold_max  = payload[3];
					g_bp_diastolic_threshold_min = payload[4];
					g_bp_diastolic_threshold_max = payload[5];
					EEPROM.write(G_BP_SYS_MIN_ADDR, g_bp_systolic_threshold_min);
					EEPROM.write(G_BP_SYS_MAX_ADDR, g_bp_systolic_threshold_max);
					EEPROM.write(G_BP_DIAS_MIN_ADDR, g_bp_diastolic_threshold_min);
					EEPROM.write(G_BP_DIAS_MAX_ADDR, g_bp_diastolic_threshold_max);
				}
				break;
			default:
				break;
		}
	}
	else if (cmd == 0x30) {  // Request Reading
		switch (field) {
			case 0x01:
				alert_request_read("bp");
				break;
			case 0x02:
				alert_request_read("temp");
				break;
			case 0x03:
				alert_request_read("hr");
				break;
			default:
				break;
		}
	}
}

// void add_to_tx_retry_queue(const uint8_t *data, uint8_t len) {
// 	if (len > MAX_MSG_SIZE) {
// 		len = MAX_MSG_SIZE;
// 	}

// 	if (tx_retry_count < MAX_QUEUE_ITEMS) {
// 		memcpy(tx_retry_queue[tx_retry_tail], data, len);
// 		tx_retry_lengths[tx_retry_tail] = len;

// 		tx_retry_tail = (tx_retry_tail + 1) % MAX_QUEUE_ITEMS;
// 		tx_retry_count++;

// 		log_msg("INFO", "Added message to retry queue");
// 	} else {
// 		log_msg("WARN", "Retry queue full, message dropped");
// 	}
// }

void alert_request_read(const char* vital) {
	if (strncmp(vital, "bp", 3) == 0) {
		g_waiting_for_reading_bp = true;
		lcd.clear();
		lcd.setCursor(0, 0);
		lcd.send_string("Please measure:");
		lcd.setCursor(0, 1);
		lcd.send_string("Blood Pressure");
		notify_event(EVT_REQUEST_RECEIVED);
	}
	else if (strncmp(vital, "temp", 5) == 0) {
		g_waiting_for_reading_temp = true;
		lcd.clear();
		lcd.setCursor(0, 0);
		lcd.send_string("Please measure:");
		lcd.setCursor(0, 1);
		lcd.send_string("Temperature");
		notify_event(EVT_REQUEST_RECEIVED);
	}
	else if (strncmp(vital, "hr", 3) == 0) {
		g_waiting_for_reading_hr = true;
		lcd.clear();
		lcd.setCursor(0, 0);
		lcd.send_string("Please measure:");
		lcd.setCursor(0, 1);
		lcd.send_string("Heart Rate");
		notify_event(EVT_REQUEST_RECEIVED);
	}
	else {
		// log_msg("DEBUG", "Invalid vital sign passed to request reading func");
	}
	delay(5000);
	lcd.clear();
	g_last_option_index_displayed = 255;
}

void send_empty_uplink() {
	DateTime now = RTClib::now();
	static uint8_t last_checked_minute = 255;
	if (now.minute() ==  last_checked_minute) {
		return;
	}
	last_checked_minute = now.minute();
	if ((now.minute() % 5 == 0) && (now.minute() != g_last_uplink_minute)) {
		ttn.sendBytes((const uint8_t[]){0x00}, 1, 1);
		g_last_uplink_minute = now.minute();
	}
}

void handle_scheduled_readings() {
	DateTime now = RTClib::now();
	// Serial.print(now.hour());
	// Serial.print(":");
	// Serial.print(now.minute());
	// Serial.print(":");
	// Serial.println(now.second());
	if (g_current_state == READING || g_current_state == PROCESSING || g_current_state == TRANSMITTING) {
		return;
	}
	// BP once at 08:21 (to not conflict with HR reading which is at 08:00 and on even minutes)
	if (((now.hour() == 8 && now.minute() == 21) ||
		 (now.hour() == 13 && now.minute() == 15)) && // NOTE: second time is for debugging purposes. Remove when ready
		!g_waiting_for_reading_bp) {
		g_waiting_for_reading_bp = true;
		alert_request_read("bp");
		// g_previous_state = CONNECTED;
		// g_current_state = READING;
		return;
	}
	// TEMP 5x/12h
	if (((now.hour() == 8 && now.minute() == 31) ||
		 (now.hour() == 11 && now.minute() == 1) ||
		 (now.hour() == 14 && now.minute() == 1) ||
		 (now.hour() == 17 && now.minute() == 1) ||
		 (now.hour() == 20 && now.minute() == 1) ||
		 (now.hour() == 13 && now.minute() == 21)) && // NOTE: last time is for debugging purposes. Remove when ready
		!g_waiting_for_reading_temp) {
		g_waiting_for_reading_temp = true;
		alert_request_read("temp");
		// g_previous_state = CONNECTED;
		// g_current_state = READING;
		return;
	}
	// HR every hour, 3 readings spaced 2 minutes apart, take the average
	// Track scheduling state for HR
	// static uint8_t hr_last_triggered_minute = 255;
	// static uint8_t hr_last_reading_hour = 255;
	// If new hour, reset HR reading progress
	// if (now.hour() != hr_last_reading_hour) {
		// Serial.println("POINT 1: hour() != last_reading_hour");
		// hr_last_reading_hour = now.hour();
		// g_hr_readings_taken_this_hour = 0;
		// g_hr_readings_sum = 0;
		// hr_last_triggered_minute = 255;
		// g_hr_target_minute = now.minute(); // start as soon as possible
		// if ((g_hr_target_minute % 2) != 0) { // align to next even minute
			// g_hr_target_minute++;
		// }
	// }
	// if 3 readings not taken yet, and its the correct time and not duplicate
	static unsigned long last_hr_reading_ms = 0;
	static unsigned long hr_reading_interval_ms = 2UL * 60 * 1000; // 2 minutes
	if (now.minute() == 0) {
		g_hr_readings_taken_this_hour = 0;
	}
	if ((g_hr_readings_taken_this_hour < 3) && (millis() - last_hr_reading_ms >= hr_reading_interval_ms)) {
		last_hr_reading_ms = millis();
		g_waiting_for_reading_hr = true;
		alert_request_read("hr");
		g_hr_readings_taken_this_hour++;
	}
	// if ((g_hr_readings_taken_this_hour < 3) && (now.minute() == g_hr_target_minute) && (now.minute() != hr_last_triggered_minute)) {
		// Serial.println("POINT 2");
		// hr_last_triggered_minute = now.minute();
		// g_waiting_for_reading_hr = true;
		// alert_request_read("hr");
		// g_hr_target_minute += 2; // schedule next attempt in 2 minutes
		// // Serial.print("")
	// }
	return;
}

// void handle_scheduled_readings() {
//	DateTime now = RTClib::now();
//	static uint8_t hr_last_triggered_minute = 255;
//	static uint8_t hr_last_reading_hour = 255;
//	if (g_current_state == READING || g_current_state == PROCESSING || g_current_state == TRANSMITTING) {
//		return;
//	}
//	// BP once at 08:20 (to not conflict with HR reading which is at 08:00)
//	if ((
//		((now.hour() == 8) && now.minute() == 20) ||
//		(now.hour() == 20 && now.minute() == 10)) ||
//		(now.hour() == 20 && now.minute() == 15) &&
//		!g_waiting_for_reading_bp) {
//		g_waiting_for_reading_bp = true;
//		alert_request_read("bp");
//		g_previous_state = CONNECTED;
//		g_current_state = READING;
//		return;
//	}
//	// TEMP 5x/12h
//	if ((
//		(now.hour() == 8 && now.minute() == 30) ||
//		(now.hour() == 11 && now.minute() == 0) ||
//		(now.hour() == 14 && now.minute() == 0) ||
//		(now.hour() == 17 && now.minute() == 0) ||
//		(now.hour() == 20 && now.minute() == 0) ||
//		(now.hour() == 21 && now.minute() == 0) &&
//		!g_waiting_for_reading_temp)) {
//		alert_request_read("temp");
//		g_previous_state = CONNECTED;
//		g_current_state = READING;
//		return;
//	}
//	// HR every hour, 3 readings spaced 2 minutes apart, take the average
//	if (now.minute() == 0 && now.hour() != hr_last_reading_hour) {
//		hr_last_reading_hour = now.hour();
//		g_hr_readings_taken_this_hour = 0;
//		g_hr_readings_sum = 0;
//		hr_last_triggered_minute = 255;
//	}
//	if ((now.minute() % 2) == 0 && now.minute() != hr_last_triggered_minute && g_hr_readings_taken_this_hour < 3) {
//		hr_last_triggered_minute = now.minute();
//		g_waiting_for_reading_hr = true;
//		alert_request_read("hr");
//	}
//	if (now.minute() == 59) {
//		g_hr_readings_taken_this_hour = 0;
//	}
// }

void blink_led(uint8_t pin, uint8_t count, uint16_t duration) {
	for (uint8_t i = 0; i < count; i++) {
		digitalWrite(pin, HIGH);
		delay(duration);
		digitalWrite(pin, LOW);
		delay(duration);
	}
}

void beep(uint16_t freq, uint16_t duration) {
	tone(BUZZER, freq, duration);
	delay(duration);
	noTone(BUZZER);
}

void notify_event(EventType event) {
	switch (event) {
		case EVT_BT_CONNECTED:
			beep(1000, 100);
			digitalWrite(LED_GREEN, HIGH);
			delay(100);
			digitalWrite(LED_GREEN, LOW);
			break;
		case EVT_BT_DISCONNECTED:
			blink_led(LED_RED, 2, 300);
			beep(400, 300);
			break;
		case EVT_INVALID_VALUE:
			beep(600, 150);
			blink_led(LED_YELLOW, 1, 150);
			break;
		case EVT_FAILED_READING:
			blink_led(LED_RED, 3, 100);
			beep(500, 100);
			break;
		case EVT_OUT_OF_BOUNDS:
			blink_led(LED_YELLOW, 2, 200);
			beep(800, 200);
			break;
		case EVT_REQUEST_RECEIVED:
			beep(700, 100);
			delay(50);
			beep(1000, 100);
			blink_led(LED_GREEN, 1, 100);
			break;
		case EVT_THRESHOLDS_UPDATED:
			blink_led(LED_GREEN, 2, 150);
			beep(1200, 150);
			break;
		case EVT_THRESHOLDS_ERROR:
			digitalWrite(LED_RED, HIGH);
			beep(300, 1000);
			digitalWrite(LED_RED, LOW);
			break;
		case EVT_TX_FAILED:
			digitalWrite(LED_RED, HIGH);
			beep(500, 800);
			digitalWrite(LED_RED, LOW);
			break;
		case EVT_TX_SUCCESS:
			digitalWrite(LED_GREEN, HIGH);
			delay(150);
			digitalWrite(LED_GREEN, LOW);
			break;
		default:
			// log_msg("WARN", "Invalid event");
			break;
	}
}
