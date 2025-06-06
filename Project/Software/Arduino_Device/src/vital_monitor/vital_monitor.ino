// vital_monitor.ino

#include "globals.h"
#include "menu.h"
#include "utils.h"
#include "states.h"
#include <TheThingsNetwork.h>

Waveshare_LCD1602 lcd(16,2);
SoftwareSerial HM10_UART(9, 10);
DS3231 myRTC;
TheThingsNetwork ttn(Serial1, Serial, TTN_FP_EU868);

const char *appEui = "0004A30B001BF78C";
const char *appKey = "493BE84C0FE8713FCD58F382A522A63F";

// Global variable initialisations
uint8_t g_prev_button_state = 0, g_select_button_state = 0, g_next_button_state = 0;
states g_current_state = DISCONNECTED, g_previous_state = DISCONNECTED, g_setup_caller_state = DISCONNECTED;
uint8_t g_current_option_index = 0, g_last_option_index_displayed = 255;
bool g_selection_pending = true;
const unsigned long g_startup_time = millis();

uint8_t g_bp_systolic_threshold_min;
uint8_t g_bp_systolic_threshold_max;
uint8_t g_bp_diastolic_threshold_min;
uint8_t g_bp_diastolic_threshold_max;
uint8_t g_hr_threshold_min;
uint8_t g_hr_threshold_max;
uint8_t g_hr_readings_taken_this_hour = 0;

uint16_t g_temp_threshold_min;
uint16_t g_temp_threshold_max;
uint16_t g_hr_readings_sum = 0;
uint8_t g_hr_target_minute = 0;

uint8_t g_last_uplink_minute;

struct ButtonDebounce g_prev_button = {LOW, LOW, 0};
struct ButtonDebounce g_select_button = {LOW, LOW, 0};
struct ButtonDebounce g_next_button = {LOW, LOW, 0};

bool g_multi_reset = false;
bool g_waiting_for_reading_bp = false;
bool g_waiting_for_reading_temp = false;
bool g_waiting_for_reading_hr = false;
bool g_avg_hr_sent_this_hour = false;

bool debug_enabled = true;

bool g_time_synched = false;

char g_received_data_buffer[G_RECEIVED_DATA_BUFFER_SIZE];

uint8_t tx_retry_queue[MAX_QUEUE_ITEMS][MAX_MSG_SIZE];
uint8_t tx_retry_lengths[MAX_QUEUE_ITEMS];
uint8_t tx_retry_head = 0;
uint8_t tx_retry_tail = 0;
uint8_t tx_retry_count = 0;

void setup() {
	Serial.begin(9600);
	Serial1.begin(57600);
	EEPROM.begin();
	HM10_UART.begin(9600);
	lcd.init();
	lcd.clear();
	lcd.setCursor(0, 0);
	lcd.send_string("Booting...");

	// Read stores thresholds from EEPROM
	g_bp_systolic_threshold_min = EEPROM.read(G_BP_SYS_MIN_ADDR);
	g_bp_systolic_threshold_max = EEPROM.read(G_BP_SYS_MAX_ADDR);
	g_bp_diastolic_threshold_min = EEPROM.read(G_BP_DIAS_MIN_ADDR);
	g_bp_diastolic_threshold_max = EEPROM.read(G_BP_DIAS_MAX_ADDR);
	/*
	   Read low byte and then OR it with (high byte shifted 8 bits left)
	   e.g.
	   temp_min = 0x6C | (0x01 << 8)
	   temp_min = 0x0100 + 0x6C
	   temp_min = 0x016C
	   temp_min = 364
	*/
	g_temp_threshold_min = EEPROM.read(G_TEMP_MIN_ADDR) | (EEPROM.read(G_TEMP_MIN_ADDR + 1) << 8);
	g_temp_threshold_max = EEPROM.read(G_TEMP_MAX_ADDR) | (EEPROM.read(G_TEMP_MAX_ADDR + 1) << 8);
	g_hr_threshold_min = EEPROM.read(G_HR_MIN_ADDR);
	g_hr_threshold_max = EEPROM.read(G_HR_MAX_ADDR);

	// Check if thresholds are uninitialised (if uninitialised then default value is likely 255)
	if (g_bp_systolic_threshold_min == 255) {
		g_bp_systolic_threshold_min = G_BP_SYSTOLIC_THRESHOLD_MIN;
	}
	if (g_bp_systolic_threshold_max == 255) {
		g_bp_systolic_threshold_max = G_BP_SYSTOLIC_THRESHOLD_MAX ;
	}
	if (g_bp_diastolic_threshold_min == 255) {
		g_bp_diastolic_threshold_min = G_BP_DIASTOLIC_THRESHOLD_MIN ;
	}
	if (g_bp_diastolic_threshold_max == 255) {
		g_bp_diastolic_threshold_max = G_BP_DIASTOLIC_THRESHOLD_MAX ;
	}
	if (g_temp_threshold_min == 0xFFFF) {
		g_temp_threshold_min = G_TEMP_THRESHOLD_MIN ;
	}
	if (g_temp_threshold_max == 0xFFFF) {
		g_temp_threshold_max = G_TEMP_THRESHOLD_MAX ;
	}
	if (g_hr_threshold_min == 255) {
		g_hr_threshold_min = G_HR_THRESHOLD_MIN ;
	}
	if (g_hr_threshold_max == 255) {
		g_hr_threshold_max = G_HR_THRESHOLD_MAX ;
	}

	pinMode(BRK_PIN, OUTPUT);
	pinMode(BTN_PREV, INPUT);
	pinMode(BTN_SELECT, INPUT);
	pinMode(BTN_NEXT, INPUT);
	pinMode(LED_BLUE, OUTPUT);
	pinMode(LED_GREEN, OUTPUT);
	pinMode(LED_YELLOW, OUTPUT);
	pinMode(LED_RED, OUTPUT);
	pinMode(BT_STATE, INPUT);
	pinMode(BUZZER, OUTPUT);
	// digitalWrite(LED_BLUE, LOW);
	// digitalWrite(LED_GREEN, LOW);
	// digitalWrite(LED_YELLOW, LOW);
	// digitalWrite(LED_RED, LOW);
	// cycle_leds();
	digitalWrite(BUZZER, HIGH);
	delay(100);
	digitalWrite(BUZZER, LOW);
	g_time_synched = false;
	ttn.onMessage(onDownlinkMessage);
	ttn.join(appEui, appKey);
	g_last_uplink_minute = 0;
	ttn.sendBytes((const uint8_t[]){0x10}, 1, 1); // Request to get accurate time from cloud
	// myRTC.setMinute(26);
}

void loop() {
	// DateTime now = RTClib::now();
	// Serial.print(now.hour());
	// Serial.print(":");
	// Serial.print(now.minute());
	// Serial.print(":");
	// Serial.println(now.second());
	static unsigned long last_schedule_check = 0;
	if ((millis() - last_schedule_check) >= 60000UL) {
		last_schedule_check = millis();
		handle_scheduled_readings();
	}
	// Serial.print("g_waiting_for_reading_hr: ");
	// Serial.println((g_waiting_for_reading_hr) ? "True" : "False");

	if (g_current_state != READING && g_current_state != PROCESSING && g_current_state != TRANSMITTING) {
		send_empty_uplink();
	}

	g_prev_button_state = debounceReadButton(BTN_PREV, &g_prev_button);
	g_select_button_state = debounceReadButton(BTN_SELECT, &g_select_button);
	g_next_button_state = debounceReadButton(BTN_NEXT, &g_next_button);

	states next_state = g_current_state;
	
	if (g_current_state == DISCONNECTED || g_current_state == CONNECTED || g_current_state == SETUP) {
		next_state = handle_menu(g_current_state);
	}
	else if (g_current_state == READING) {
		next_state = state_reading();
	}
	else if (g_current_state == PROCESSING) {
		next_state = state_processing();
	}
	else if (g_current_state == TRANSMITTING) {
		next_state = state_transmitting();
	}
	else if (g_current_state == SETUP_BP) {
		next_state = state_setup_bp();
	}
	else if (g_current_state == SETUP_TEMP) {
		next_state = state_setup_temp();
	}
	else if (g_current_state == SETUP_HR) {
		next_state = state_setup_hr();
	}

	next_state = check_bt_connection(next_state);

	if (next_state != g_current_state) {
		change_state(next_state);
	}
}
