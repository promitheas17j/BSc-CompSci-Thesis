#include "Waveshare_LCD1602.h"

Waveshare_LCD1602 lcd(16,2);

const uint8_t BTN_PREV = 4;
const uint8_t BTN_SELECT = 7;
const uint8_t BTN_NEXT = 8;
const uint8_t LED_BLUE = A5;
const uint8_t LED_GREEN = A4;
const uint8_t LED_YELLOW = A3;
const uint8_t LED_RED = A2;

const char *menu_disconnected[] = {"Scan", "Setup"};
const char *menu_connected[] = {"Read", "Disconnect"};
const char *menu_reading[] = {"Cancel reading"};
const char *menu_processing[] = {"Abort process"};
const char *menu_transmitting[] = {"Abort Tx"};

struct ButtonDebounce {
	uint8_t stable_state;
	uint8_t last_reading;
	unsigned long last_debounce_time;
};

struct Menu {
	const char **options;
	uint8_t num_options;
};

Menu menu_table[] = {
	{menu_disconnected, sizeof(menu_disconnected) / sizeof(menu_disconnected[0])},
	{menu_connected, sizeof(menu_connected) / sizeof(menu_connected[0])},
	{menu_reading, sizeof(menu_reading) / sizeof(menu_reading[0])},
	{menu_processing, sizeof(menu_processing) / sizeof(menu_processing[0])},
	{menu_transmitting, sizeof(menu_transmitting) / sizeof(menu_transmitting[0])}
};

enum class states : uint8_t {
	DISCONNECTED = 0,
	CONNECTED,
	READING,
	PROCESSING,
	TRANSMITTING
};

typedef states (*StateFunc)();
states state_disconnected();
states state_connected();
states state_reading();
states state_processing();
states state_transmitting();

struct StateTable {
	states state;
	StateFunc func;
};

StateTable stateTable[] = {
	{states::DISCONNECTED, state_disconnected},
	{states::CONNECTED, state_connected},
	{states::READING, state_reading},
	{states::PROCESSING, state_processing},
	{states::TRANSMITTING, state_transmitting}
};


// Function prototypes
uint8_t debounceReadButton(uint8_t pin);
void lcd_print_line(const char *option);
void handle_menu(states current_state);
uint8_t handle_menu_options_buttons(const char *options[], uint8_t g_current_option_index);
void log(char **msg_level, char **msg);
// void cycle_leds();

const uint8_t NUM_STATES = sizeof(stateTable) / sizeof(stateTable[0]);

// Global variables
bool debug_enabled = true;
char str_current_option[] = "Booting...";
uint8_t g_prev_button_state = 0;
uint8_t g_select_button_state = 0;
uint8_t g_next_button_state = 0;
states g_current_state = states::DISCONNECTED;
uint8_t g_current_option_index = 0;
uint8_t g_last_option_index_displayed = 255;
ButtonDebounce g_prev_button = {LOW, LOW, 0};
ButtonDebounce g_select_button = {LOW, LOW, 0};
ButtonDebounce g_next_button = {LOW, LOW, 0};


uint8_t debounceReadButton(uint8_t pin, ButtonDebounce* btn) {
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

// uint8_t debounceReadButton(uint8_t pin) {
// 	static uint8_t state = LOW;
// 	static unsigned long last_debounce_time = 0;
// 	const unsigned long debounce_delay = 20;

// 	uint8_t reading = digitalRead(pin);

// 	if reading != state) {
// 		last_debounce_time = millis();
// 	}

// 	if ((millis() - last_debounce_time) > debounce_delay) {
// 		state = reading;
// 	}

// 	return state;
// }

void log(const char *msg_level, const char *msg) {
	if (!debug_enabled && strcmp(msg_level, "DEBUG") == 0) {
		return;
	}
	char buffer[64];
	snprintf(buffer, sizeof(buffer), "[%s]: %s", msg_level, msg);
	Serial.println(buffer);
}

void handle_menu(states current_state) {
	uint8_t state_index = static_cast<uint8_t>(current_state);
	const char **options = menu_table[state_index].options;
	uint8_t num_options = menu_table[state_index].num_options;

	uint8_t result = handle_menu_options_buttons(options, num_options);

	if (result != 255) {
		char msg[64];
		snprintf(msg, sizeof(msg), "Selected option: %s", options[result]);
		log("INFO", msg);
		// TODO: Execute option action here
	}
}

uint8_t handle_menu_options_buttons(const char **options, uint8_t num_options) {
	if (g_current_option_index != g_last_option_index_displayed) {
		lcd_print_line(options[g_current_option_index]);
		g_last_option_index_displayed = g_current_option_index;
	}

	// Edge detection - remember last button states (only do something when button state changes)
	static uint8_t last_prev_btn_state = LOW;
	static uint8_t last_next_btn_state = LOW;
	static uint8_t last_select_btn_state = LOW;

	// PREV button rising edge
	if (g_prev_button_state == HIGH && last_prev_btn_state == LOW) {
		log("INFO", "PREV PRESSED");
		g_current_option_index = (g_current_option_index == 0) ? num_options - 1 : g_current_option_index - 1;
	}
	
	// NEXT button rising edge
	if (g_next_button_state == HIGH && last_next_btn_state == LOW) {
		log("INFO", "NEXT PRESSED");
		g_current_option_index = (g_current_option_index + 1) % num_options;
	}
	
	// SELECT button rising edge
	if (g_select_button_state == HIGH && last_select_btn_state == LOW) {
		log("INFO", "SELECT PRESSED");
		return g_current_option_index;
	}

	// Update last button states
	last_prev_btn_state = g_prev_button_state;
	last_next_btn_state = g_next_button_state;
	last_select_btn_state = g_select_button_state;

	return 255; // 255 = nothing selected yet
}

void setup() {
	Serial.begin(9600);
	lcd.init();
	lcd.clear();
	lcd.setCursor(0, 0);
	lcd.send_string(str_current_option);
	lcd.setCursor(0, 1);
	lcd.send_string("");
	pinMode(BTN_PREV, INPUT);
	pinMode(BTN_SELECT, INPUT);
	pinMode(BTN_NEXT, INPUT);
	pinMode(LED_BLUE, OUTPUT);
	pinMode(LED_GREEN, OUTPUT);
	pinMode(LED_YELLOW, OUTPUT);
	pinMode(LED_RED, OUTPUT);
}

void loop() {
	g_prev_button_state = debounceReadButton(BTN_PREV, &g_prev_button);
	g_select_button_state = debounceReadButton(BTN_SELECT, &g_select_button);
	g_next_button_state = debounceReadButton(BTN_NEXT, &g_next_button);
	Serial.print("\n\tPREV: "); Serial.print(g_prev_button_state);
	Serial.print("\n\tNEXT: "); Serial.print(g_next_button_state);
	Serial.print("\n\tSELECT: "); Serial.print(g_select_button_state);

	handle_menu(g_current_state);
}

void lcd_print_line(const char *option) {
	lcd.clear();
	lcd.setCursor(0, 0);
	lcd.send_string(option);  
	lcd.setCursor(0, 1);
	lcd.send_string("<    SELECT    >");
}

void cycle_leds() {
	digitalWrite(LED_BLUE, HIGH);
	delay(200);
	digitalWrite(LED_BLUE, LOW);
	delay(200);
	digitalWrite(LED_GREEN, HIGH);
	delay(200);
	digitalWrite(LED_GREEN, LOW);
	delay(200);
	digitalWrite(LED_YELLOW, HIGH);
	delay(200);
	digitalWrite(LED_YELLOW, LOW);
	delay(200);
	digitalWrite(LED_RED, HIGH);
	delay(200);
	digitalWrite(LED_RED, LOW);
}

states state_disconnected() {
	log("INFO", "State: DISCONNECTED");
	return states::DISCONNECTED;
}

states state_connected() {
	log("INFO", "State: CONNECTED");
	return states::CONNECTED;
}

states state_reading() {
	log("INFO", "State: READING");
	return states::READING;
}

states state_processing() {
	log("INFO", "State: PROCESSING");
	return states::PROCESSING;
}

states state_transmitting() {
	log("INFO", "State: TRANSMITTING");
	return states::TRANSMITTING;
}
