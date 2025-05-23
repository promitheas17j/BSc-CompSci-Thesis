// states.cpp

#include "states.h"
#include "globals.h"
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
	return multi_threshold_setup_u8(
		bp_prompts, bp_values, bp_lo, bp_hi, bp_addr,
		sizeof(bp_prompts) / sizeof(bp_prompts[0]),
		SETUP_BP,      // this state’s enum
		// g_current_state,
		// SETUP          // return here when done
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
	return multi_threshold_setup_u16(
		temp_prompts,
		temp_values,
		temp_lo,
		temp_hi,
		temp_addr,
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
	return multi_threshold_setup_u8(
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
	log_msg("INFO", F("Bluetooth device connected."));
	// digitalWrite(LED_BLUE, HIGH);
	return CONNECTED;
}

states state_reading() {
	char input_buffer[G_RECEIVED_DATA_BUFFER_SIZE];
	uint8_t index = 0;
	static unsigned long last_high_time = 0;
	const unsigned long disconnect_threshold = 2000; // 2 seconds of LOW before triggering
	static uint8_t attempt_count = 0;
	uint8_t bt_pin_state = digitalRead(BT_STATE);
	if (bt_pin_state == HIGH) {
		last_high_time = millis();
	}
	else {
		if ((millis() - last_high_time) > disconnect_threshold) {
			log_msg("WARN", F("Bluetooth disconnected during READING"));
			return DISCONNECTED;
		}
	}
	while (HM10_UART.available()) {
		if (attempt_count >= 3) { // too many failed attempts, return to previous state
			log_msg("WARN", F("Too many failed attempts. Cancelling"));
			attempt_count = 0;
			return g_previous_state;
		}
		log_msg("DEBUG", F("Reading data"));
		char incoming_byte = HM10_UART.read();
		// char debug_char[2] = {incoming_byte, '\0'};
		if (incoming_byte == '\n') {
			input_buffer[index] = '\0';
			size_t len = strlen(input_buffer);
			if (len > 0 && input_buffer[len - 1] == '\r') {
				input_buffer[len - 1] = '\0';
				log_msg("DEBUG", F("Stripped trailing \\r from input_buffer"));
			}
			log_msg("DEBUG", F("Received string: "));
			log_msg("DEBUG", input_buffer);
			if (validate_message(input_buffer)) {
				HM10_UART.print("ACK");
				log_msg("INFO", F("Valid data received. ACK sent."));
				strncpy(g_received_data_buffer, input_buffer, sizeof(g_received_data_buffer));
				g_received_data_buffer[sizeof(g_received_data_buffer) - 1] = '\0'; // ensure that the last character is the null terminator no matter what
				return PROCESSING;
			}
			else {
				attempt_count++;
				HM10_UART.print("RETRY");
				log_msg("INFO", F("Invalid data received. Retry request sent."));
			}
			index = 0;
		}
		else if (index < sizeof(input_buffer) - 1) {
			input_buffer[index++] = incoming_byte;
		}
		else { // Buffer overflow
			index = 0;
			log_msg("WARN", F("Input buffer overflow. Resetting."));
		}
	}
	return READING;
}

states state_processing() {
	char msg[64];
	snprintf(msg, sizeof(msg), "Data buffer contains:\n\t %s", g_received_data_buffer);
	log_msg("DEBUG", msg);
	if (strncmp(g_received_data_buffer, "BP:", 3) == 0) {
		uint8_t sys, dia;
		if (sscanf(g_received_data_buffer + 3, "%hhu/%hhu", &sys, &dia) == 2) {
			bool ok = (sys >= g_bp_systolic_threshold_min &&
					sys <= g_bp_systolic_threshold_max &&
					dia >= g_bp_diastolic_threshold_min &&
					dia <= g_bp_systolic_threshold_max
				);
			snprintf(msg, sizeof(msg), "BP %hhu/%hhu %s", sys, dia, ok ? "OK" : "ALERT");
			log_msg("INFO", msg);
			if (!ok) {
				return TRANSMITTING;
			}
		}
		else {
			log_msg("WARN", F("BP parse error."));
		}
	}
	// FIX: TEMP always is unknown data type
	else if (strncmp(g_received_data_buffer, "TEMP:", 5) == 0) {
		uint8_t whole, decimal, n;
<<<<<<< HEAD
		if ((sscanf(g_received_data_buffer + 5, "%hhu.%hhu", &whole, &decimal) == 2)) { // && (g_received_data_buffer[5 + n] == '\0')) {
=======
		if ((sscanf(g_received_data_buffer + 5, "%hhu%hhu.%hhu", &whole, &decimal, &n) == 2) && (g_received_data_buffer[5 + n] == '\0')) {
>>>>>>> 7e96a231bb48141aadbb787da72ed0244871e0d5
			uint16_t temperature = whole * 10 + decimal;
			bool ok = (temperature >= g_temp_threshold_min && temperature <= g_temp_threshold_max);
			snprintf(msg, sizeof(msg), "TEMP %u.%u %s", whole, decimal, ok ? "OK" : "ALERT");
			log_msg("INFO", msg);
			if (!ok) {
				return TRANSMITTING;
			}
		}
		else {
			log_msg("WARN", F("TEMP parse error."));
		}
	}
	else if (strncmp(g_received_data_buffer, "HR:", 3) == 0) {
		// char *p = g_received_data_buffer + 3;
		uint8_t hr = (uint8_t)atoi(g_received_data_buffer + 3);
		bool ok = (hr >= g_hr_threshold_min && hr <= g_hr_threshold_max);
		snprintf(msg, sizeof(msg), "HR %u %s", hr, ok ? "OK" : "ALERT");
		log_msg("INFO", msg);
		if (!ok) {
			return TRANSMITTING;
		}
	}
	else {
		log_msg("WARN", F("Unknown data type"));
	}
	return CONNECTED;
}

states state_transmitting() {
	// FIX: Not testing for ACK. Will set up dashboard and try again
	log_msg("INFO", F("Ready to send data to cloud using LoRaWAN"));
	bool success = false;
	for (uint8_t attempt = 1; attempt <= 3; ++attempt) {
		log_msg("INFO", F("Sending data..."));
		bool send_success = ttn.sendBytes((const uint8_t*)g_received_data_buffer, strlen((const char*)g_received_data_buffer), 1);  // Port 1
		uint32_t start = millis();
		while (millis() - start < 10000) {
			if (send_success == true) {
				success = true;
				break;
			}
		}
		if (success) {
			log_msg("INFO", F("Data sent successfully"));
			break;
		} else {
			log_msg("WARN", F("Transmission timed out. Retrying..."));
		}
	}
	if (!success) {
		log_msg("ERROR", F("Failed to send after 3 attempts"));
		add_to_tx_retry_queue((const uint8_t*)g_received_data_buffer, strlen((const char*)g_received_data_buffer));
		lcd.clear();
		lcd.setCursor(0, 0);
		lcd.send_string("Send failed");
		delay(5000);
	}
	return CONNECTED;
}

// states state_transmitting() {
// 	log_msg("INFO", F("Ready to send data to cloud using LoRaWAN"));
// 	uint8_t attempt_count = 0;
// 	bool send_success = false;
// 	while (attempt_count < max_attempts && ! send_success) {
// 		log_msg("INFO", F("Attempting transmission..."));
// 		if (ttn.sendBytes(g_received_data_buffer, strlen((const char*)g_received_data_buffer, 1))) {
// 			log_msg("INFO", F("Data sent successfully"));
// 			// wait for downlink ACK here
// 			send_success = true;
// 		}
// 		else {
// 			log_msg("WARN", F("Sending failed"));
// 			attempt_count++;
// 			delay(10000); // wait 10s before next attempt
// 		}
// 	}
// 	if (!send_success) {
// 		log_msg("ERROR", F("All Tx attempts failed. Queueing data"));
// 		add_to_tx_retry_queue(g_received_data_buffer, strlen(((const char*)g_received_data_buffer)));
// 		lcd.clear();
// 		lcd.setCursor(0,0);
// 		lcd.print(F("Error:"));
// 		lcd.setCursor(0, 1);
// 		lcd.print("Tx Send Fail");
// 		delay(5000);
// 	}
// 	return CONNECTED;
// }

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
	char msg[64];
	snprintf(msg, sizeof(msg), "State: %s", state_to_string(g_current_state));
	log_msg("INFO", msg);
	if (new_state == READING) {
		while (HM10_UART.available()) {
			HM10_UART.read();
		}
		log_msg("DEBUG", F("Flushed UART buffer before entering READING state"));
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
			log_msg("DEBUG", F("BT connection detected."));
			return CONNECTED;
		}
	}
	else {
		last_high_time = millis();
		if (is_connected) {
			is_connected = false;
			log_msg("DEBUG", F("BT disconnection detected."));
			return DISCONNECTED;
		}
	}
	return current_state;
}
