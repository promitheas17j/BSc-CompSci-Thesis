// states.cpp

#include "globals.h"
#include "states.h"
#include "menu.h"
#include "utils.h"
#include <string.h>
#include <stdio.h>
#include <Arduino.h>
#include <EEPROM.h>

struct StateTable stateTable[] = {
	{DISCONNECTED, state_disconnected},
	{SETUP, state_setup},
	{SETUP_BP, state_setup_bp},
	{SETUP_TEMP, state_setup_temp},
	{SETUP_HR, state_setup_hr},
	{CONNECTED, state_connected},
	{READING, state_reading},
	{PROCESSING, state_processing},
	{TRANSMITTING, state_transmitting}
};

const uint8_t NUM_STATES = sizeof(stateTable) / sizeof(stateTable[0]);

states state_disconnected() {
	// TODO: Check if wiring Vcc of HM-10 to a pin on the arduino and bringing low for a second here can disconnect all devices
	// digitalWrite(LED_BLUE, LOW);
	return DISCONNECTED;
}

states state_setup() {
	digitalWrite(LED_BLUE, digitalRead(BT_STATE));
	return SETUP;
}

states state_setup_bp() {
	static const char*  bp_prompts[4] = {
		"SYS min:", "SYS max:",
		"DIA min:", "DIA max:"
	};
	static uint8_t*    bp_values[4] = {
		&g_bp_systolic_threshold_min,
		&g_bp_systolic_threshold_max,
		&g_bp_diastolic_threshold_min,
		&g_bp_diastolic_threshold_max
	};
	static const uint8_t bp_lo[4] = {
		G_BP_SYSTOLIC_THRESHOLD_MIN,
		G_BP_SYSTOLIC_THRESHOLD_MIN,
		G_BP_DIASTOLIC_THRESHOLD_MIN,
		G_BP_DIASTOLIC_THRESHOLD_MIN
	};
	static const uint8_t bp_hi[4] = {
		G_BP_SYSTOLIC_THRESHOLD_MAX,
		G_BP_SYSTOLIC_THRESHOLD_MAX,
		G_BP_DIASTOLIC_THRESHOLD_MAX,
		G_BP_DIASTOLIC_THRESHOLD_MAX
	};
	static const uint8_t bp_addr[4] = {
		G_BP_SYS_MIN_ADDR,
		G_BP_SYS_MAX_ADDR,
		G_BP_DIAS_MIN_ADDR,
		G_BP_DIAS_MAX_ADDR
	};
	return multi_threshold_setup<uint8_t>(
		bp_prompts, bp_values, bp_lo, bp_hi, bp_addr,
		sizeof(bp_prompts) / sizeof(bp_prompts[0]),
		SETUP_BP,      // this state’s enum
		g_previous_state
	);
}

states state_setup_temp() {
	static const char* temp_prompts[2] = {
		"TEMP min:",
		"TEMP max:"
	};
	static uint16_t* temp_values[2] = {
		&g_temp_threshold_min,
		&g_temp_threshold_max
	};
	static const uint16_t temp_lo[2] = {
		G_TEMP_THRESHOLD_MIN,
		G_TEMP_THRESHOLD_MIN
	};
	static const uint16_t temp_hi[2] = {
		G_TEMP_THRESHOLD_MAX,
		G_TEMP_THRESHOLD_MAX
	};
	static const uint16_t temp_addr[2] = {
		G_TEMP_MIN_ADDR,
		G_TEMP_MAX_ADDR
	};
	return multi_threshold_setup<uint16_t>(
		temp_prompts,
		temp_values,
		temp_lo,
		temp_hi,
		reinterpret_cast<const uint8_t*>(temp_addr),
		sizeof(temp_prompts) / sizeof(temp_prompts[0]),
		SETUP_TEMP,
		g_previous_state
	);
	return SETUP_TEMP;
}

states state_setup_hr() {
	static const char* hr_prompts[2] = {
		"HR min:",
		"HR max:"
	};
	static uint8_t* hr_values[2] = {
		&g_hr_threshold_min,
		&g_hr_threshold_max
	};
	static const uint8_t hr_lo[2] = {
		G_HR_THRESHOLD_MIN,
		G_HR_THRESHOLD_MIN
	};
	static const uint8_t hr_hi[2] = {
		G_HR_THRESHOLD_MAX,
		G_HR_THRESHOLD_MAX
	};
	static const uint8_t hr_addr[2] = {
		G_HR_MIN_ADDR,
		G_HR_MAX_ADDR
	};
	return multi_threshold_setup<uint8_t>(
		hr_prompts,
		hr_values,
		hr_lo,
		hr_hi,
		hr_addr,
		sizeof(hr_prompts) / sizeof(hr_prompts[0]),
		SETUP_HR,
		g_previous_state
	);

	return SETUP_HR;
}

states state_connected() {
	// log_msg("INFO", "Bluetooth device connected.");
	// digitalWrite(LED_BLUE, HIGH);
	return CONNECTED;
}

states state_reading() {
	static char input_buffer[G_RECEIVED_DATA_BUFFER_SIZE];
	static uint8_t index = 0;
	static unsigned long last_high_time = 0;
	const unsigned long disconnect_threshold = 2000; // 2 seconds of LOW before triggering
	static uint8_t attempt_count = 0;
	uint8_t bt_pin_state = digitalRead(BT_STATE);
	static unsigned long entry_time = 0;
	static bool first_entry = true;
	if (first_entry) {
		// log_msg("INFO", F("First entry into READING"));
		while (HM10_UART.available()) {
			HM10_UART.read();
		}
		index = 0;
		memset(input_buffer, 0, sizeof(input_buffer));
		entry_time = millis();
		first_entry = false;
	}
	if ((millis() - entry_time) > 10000) { // timeout timer
		Serial.println("Timeout");
		notify_event(EVT_FAILED_READING);
		first_entry = true;
		attempt_count = 0;
		return g_previous_state;
	}
	if (bt_pin_state == HIGH) {
		last_high_time = millis();
	}
	else {
		if ((millis() - last_high_time) > disconnect_threshold) {
			// log_msg("WARN", F("Bluetooth disconnected during READING"));
			notify_event(EVT_BT_DISCONNECTED);
			first_entry = false;
			return DISCONNECTED;
		}
	}
	// Serial.println("Pre-HM10-UART");
	while (HM10_UART.available()) {
		// Serial.print("attempt_count: ");
		// Serial.println(attempt_count);
		if (attempt_count >= 3) { // too many failed attempts, return to previous state
			// log_msg("WARN", F("Too many failed attempts. Cancelling"));
			notify_event(EVT_FAILED_READING);
			attempt_count = 0;
			first_entry = false;
			return g_previous_state;
		}
		char incoming_byte = HM10_UART.read();
		// Serial.print("Recvd byte: ");
		// Serial.println(incoming_byte, HEX);
		// Serial.print("\n");
		// log_msg("DEBUG-1", input_buffer);
		// char debug_char[2] = {incoming_byte, '\0'};
		if (incoming_byte == '\n') {
			input_buffer[index] = '\0';
			size_t len = strlen(input_buffer);
			if (len > 0 && input_buffer[len - 1] == '\r') {
				input_buffer[len - 1] = '\0';
			}
			// DEBUGGING
			// Serial.print("DEBUG-2 ");
			// Serial.println(input_buffer);
			// char dbg[32];
			// snprintf(dbg, sizeof(dbg), "LEN=%u", strlen(input_buffer));
			// log_msg("DEBUG", dbg);
			// for (size_t i = 0; i < strlen(input_buffer); ++i) {
			// 	char cinfo[16];
			// 	sprintf(cinfo, "[%u]=%02X (%c)", i, input_buffer[i], input_buffer[i]);
			// 	log_msg("BYTE", cinfo);
			// }
			// END DEBUGGING
			if (len > 0 && input_buffer[len - 1] == '\r') {
				input_buffer[len - 1] = '\0';
			}
			// Strip surrounding quotes if present
			if (input_buffer[0] == '"' && input_buffer[strlen(input_buffer) - 1] == '"') {
				memmove(input_buffer, input_buffer + 1, strlen(input_buffer)); // shift left
				input_buffer[strlen(input_buffer) - 1] = '\0'; // remove trailing "
			}
			// Serial.print("Raw: ");
			// Serial.println(input_buffer);
			if (validate_message(input_buffer)) {
				HM10_UART.print("ACK");
				// log_msg("INFO", F("Valid data received. ACK sent."));
				strncpy(g_received_data_buffer, input_buffer, sizeof(g_received_data_buffer));
				g_received_data_buffer[sizeof(g_received_data_buffer) - 1] = '\0'; // ensure that the last character is the null terminator no matter what
				first_entry = true;
				// Serial.print("READING: ");
				// Serial.println(g_received_data_buffer);
				return PROCESSING;
			}
			else {
				attempt_count++;
				HM10_UART.print("RETRY");
				// log_msg("INFO", F("Invalid data received. Retry request sent."));
				notify_event(EVT_INVALID_VALUE);
			}
			index = 0;
		}
		else if (index < sizeof(input_buffer) - 1) {
			input_buffer[index++] = incoming_byte;
		}
		else { // Buffer overflow
			index = 0;
			// log_msg("WARN", F("Input buffer overflow. Resetting."));
			notify_event(EVT_FAILED_READING);
		}
	}
	return READING;
}

states state_processing() {
	DateTime now = RTClib::now();
	Serial.print("HR readings taken: ");
	Serial.println(g_hr_readings_taken_this_hour);
	if (strncmp(g_received_data_buffer, "BP:", 3) == 0) {
		uint8_t sys, dia;
		if (sscanf(g_received_data_buffer + 3, "%hhu/%hhu", &sys, &dia) == 2) {
			if (g_waiting_for_reading_bp) {
				return TRANSMITTING;
			}
			bool ok = (sys >= g_bp_systolic_threshold_min &&
					sys <= g_bp_systolic_threshold_max &&
					dia >= g_bp_diastolic_threshold_min &&
					dia <= g_bp_systolic_threshold_max
			);
			if (!ok) {
				return TRANSMITTING;
			}
		}
		else {
			// log_msg("WARN", F("BP parse error."));
			notify_event(EVT_INVALID_VALUE);
		}
	}
	else if (strncmp(g_received_data_buffer, "TEMP:", 5) == 0) {
		uint8_t whole, decimal;
		if ((sscanf(g_received_data_buffer + 5, "%hhu.%hhu", &whole, &decimal) == 2)) { // && (g_received_data_buffer[5 + n] == '\0')) {
			if (g_waiting_for_reading_temp) {
				return TRANSMITTING;
			}
			uint16_t temperature = whole * 10 + decimal;
			bool ok = (temperature >= g_temp_threshold_min && temperature <= g_temp_threshold_max);
			if (!ok) {
				return TRANSMITTING;
			}
		}
		else {
			// log_msg("WARN", F("TEMP parse error."));
			notify_event(EVT_INVALID_VALUE);
		}
	}
	else if (strncmp(g_received_data_buffer, "HR:", 3) == 0) {
		uint8_t hr = (uint8_t)atoi(g_received_data_buffer + 3); // NOTE: hr reading from the received data buffer means that it will always just pull the last value read
		Serial.print("hr: ");
		Serial.println(hr);
		// Serial.print("hr sum A: ");
		// Serial.println(g_hr_readings_sum);
		if (g_hr_readings_taken_this_hour < 3) {
			g_hr_readings_sum += hr;
			// g_hr_readings_taken_this_hour++;
			// g_waiting_for_reading_hr = false;
			// Serial.print("waiting, readings taken: ");
			// Serial.print((g_waiting_for_reading_hr ? "True" : "False"));
			// Serial.print(", ");
			// Serial.println(g_hr_readings_taken_this_hour);
			// g_hr_target_minute += 2;
			// Serial.println("Scheduled HR reading");
			// if (now.minute() == g_hr_target_minute) {
			// 	g_hr_readings_sum += hr;
			// 	Serial.print("hr sum: ");
			// 	Serial.println(g_hr_readings_sum);
			// 	g_hr_readings_taken_this_hour++;
			// 	g_hr_target_minute += 2;
			// 	// log_msg("DEBUG", "Scheduled HR reading");
			// }
			// Serial.print("readings taken: ");
			// Serial.println(g_hr_readings_taken_this_hour);
			return CONNECTED;
		}
		if (g_hr_readings_taken_this_hour == 3) {
			g_hr_readings_sum += hr; // last addition doesnt happen in upper if block as readings taken is 3
			snprintf(g_received_data_buffer, sizeof(g_received_data_buffer), "HR:%u", (g_hr_readings_sum + 1) / 3);
			return TRANSMITTING;
		}
		g_hr_readings_taken_this_hour++;
		bool ok = (hr >= g_hr_threshold_min && hr <= g_hr_threshold_max);
		if (!ok) {
			notify_event(EVT_OUT_OF_BOUNDS);
		}
	}
	return TRANSMITTING;
	// else if (strncmp(g_received_data_buffer, "HR:", 3) == 0) {
	// 	uint8_t hr = (uint8_t)atoi(g_received_data_buffer + 3);
	// 	if (g_waiting_for_reading_hr) {
	// 		log_msg("DEBUG", "Scheduled HR received");
	// 		g_hr_readings_sum += hr;
	// 		g_hr_readings_taken_this_hour++;
	// 		// char debug_msg[40];
	// 		// snprintf(debug_msg, sizeof(debug_msg), "[DEBUG]: HR reading #%u = %u", g_hr_readings_taken_this_hour, hr);
	// 		// Serial.println(debug_msg);
	// 		if (g_hr_readings_taken_this_hour == 3) {
	// 			snprintf(g_received_data_buffer, sizeof(g_received_data_buffer), "HR:%u", (g_hr_readings_sum + 1) / 3);
	// 			Serial.println("HR average ready. Txing " + (uint8_t)(g_hr_readings_sum + 1) / 3);
	// 			return TRANSMITTING;
	// 		}
	// 		else {
	// 			// log_msg("DEBUG", "Waiting for more HR readings");
	// 			return CONNECTED;
	// 		}
	// 	}
	// 	bool ok = (hr >= g_hr_threshold_min && hr <= g_hr_threshold_max);
	// 	if (!ok) {
	// 		return TRANSMITTING;
	// 	}
	// }
	// else {
	// 	notify_event(EVT_INVALID_VALUE);
	// }
	// return CONNECTED;
}

states state_transmitting() {
	// Serial.print("Sending payload: ");
	// Serial.println(g_received_data_buffer);
	// bool success = false;
	for (uint8_t attempt = 1; attempt <= 3; ++attempt) {
		// bool send_success = ttn.sendBytes((const uint8_t*)g_received_data_buffer, strlen((const char*)g_received_data_buffer), 1);  // Port 1
		bool success = ttn.sendBytes((const uint8_t*)g_received_data_buffer, strlen((const char*)g_received_data_buffer), 1);
		// lcd.clear();
		// lcd.setCursor(0, 0);
		// lcd.send_string((send_success) ? "True" : "False");
		// delay(2000);
		uint32_t start = millis();
		delay(1000);
		// while (millis() - start < 10000) {
		// 	if (send_success == true) {
		// 		success = true;
		// 		break;
		// 	}
		// }
		if (success) {
			if (strncmp(g_received_data_buffer, "BP:", 3) == 0 && g_waiting_for_reading_bp) {
				g_waiting_for_reading_bp = false;
			}
			else if (strncmp(g_received_data_buffer, "TEMP:", 5) == 0 && g_waiting_for_reading_temp) {
				g_waiting_for_reading_temp = false;
			}
			else if (strncmp(g_received_data_buffer, "HR:", 3) == 0 && g_waiting_for_reading_hr) {
				g_waiting_for_reading_hr = false;
				g_hr_readings_sum = 0;
				g_hr_readings_taken_this_hour = 0;
				// Serial.println("Tx success check clear");
				return CONNECTED;
			}
			// Serial.print("Tx payload (cleared after printing): ");
			// Serial.println(g_received_data_buffer);
			g_received_data_buffer[0] = '\0';
			lcd.clear();
			lcd.setCursor(0, 0);
			lcd.send_string("Send success");
			notify_event(EVT_TX_SUCCESS);
			delay(2000);
			return CONNECTED;
		}
		else {
			lcd.clear();
			lcd.setCursor(0, 0);
			lcd.send_string("Send failed");
			notify_event(EVT_TX_FAILED);
			delay(5000);
		}
	}
	return CONNECTED;
}

void change_state(states new_state) {
	if (new_state == SETUP
			&& g_current_state != SETUP_BP
			&& g_current_state != SETUP_TEMP
			&& g_current_state != SETUP_HR) {
		g_setup_caller_state = g_current_state;
	}
	if (new_state != g_current_state) {
		g_previous_state = g_current_state;
	}
	g_current_state = new_state;
	if (new_state == SETUP_BP || new_state == SETUP_TEMP || new_state == SETUP_HR) {
		g_multi_reset = true;
	}
	g_current_option_index = 0;
	g_last_option_index_displayed = 255;
	g_selection_pending = false; // prevent "bouncing" of multiple menu options after selection
	if (g_current_state != SETUP_BP && g_current_state != SETUP_TEMP && g_current_state != SETUP_HR) {
		lcd_print_line(menu_table[(uint8_t)g_current_state].options[0], menu_table[(uint8_t)g_current_state].num_options > 1);
	}
	// char msg[64];
	// snprintf(msg, sizeof(msg), "State: %s", state_to_string(g_current_state));
	// log_msg("INFO", msg);
	if (new_state == READING) {
		while (HM10_UART.available()) {
			HM10_UART.read();
		}
	}
	update_led_based_on_state();
}

states check_bt_connection(states current_state) {
	// const unsigned long stable_threshold = 3000, check_interval = 500; // 3 second HIGH, only check 2 times per second
	const unsigned long stable_threshold = 1000, stabilisation_period = 5000; // 3 second HIGH, wait 5 seconds after boot before checking
	static unsigned long last_high_time = 0;
	// static unsigned long last_check_time = 0;
	static bool is_connected = false;
	uint8_t bt_pin_state = digitalRead(BT_STATE);
	if ((millis() - g_startup_time) < stabilisation_period) {
		return current_state;
	}

	// States that ignore connection changes completely:
	if (current_state == PROCESSING ||
			current_state == TRANSMITTING ||
			current_state == SETUP ||
			current_state == SETUP_BP ||
			current_state == SETUP_TEMP ||
			current_state == SETUP_HR) {
		return current_state;
	}
	if (bt_pin_state == HIGH) {
		if (!is_connected && (millis() - last_high_time > stable_threshold)) {
			is_connected = true;
			notify_event(EVT_BT_CONNECTED);
			return CONNECTED;
		}
	}
	else {
		last_high_time = millis();
		if (is_connected) {
			is_connected = false;
			notify_event(EVT_BT_DISCONNECTED);
			return DISCONNECTED;
		}
	}
	return current_state;
}
