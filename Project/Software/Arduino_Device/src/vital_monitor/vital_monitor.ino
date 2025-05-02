#include "globals.h"
#include "menu.h"
#include "utils.h"
#include "states.h"
#include "Waveshare_LCD1602.h"

Waveshare_LCD1602 lcd(16,2);

// Global variable initialisations
uint8_t g_prev_button_state = 0;
uint8_t g_select_button_state = 0;
uint8_t g_next_button_state = 0;
states g_current_state = DISCONNECTED;
uint8_t g_current_option_index = 0;
uint8_t g_last_option_index_displayed = 255;
bool g_selection_pending = true;

struct ButtonDebounce g_prev_button = {LOW, LOW, 0};
struct ButtonDebounce g_select_button = {LOW, LOW, 0};
struct ButtonDebounce g_next_button = {LOW, LOW, 0};

bool debug_enabled = true;

void setup() {
	Serial.begin(9600);
	lcd.init();
	lcd.clear();
	lcd.setCursor(0, 0);
	lcd.send_string("Booting...");
	lcd.setCursor(0, 1);
	lcd.send_string("");
	pinMode(BTN_PREV, INPUT);
	pinMode(BTN_SELECT, INPUT);
	pinMode(BTN_NEXT, INPUT);
	pinMode(LED_BLUE, OUTPUT);
	pinMode(LED_GREEN, OUTPUT);
	pinMode(LED_YELLOW, OUTPUT);
	pinMode(LED_RED, OUTPUT);
	cycle_leds();
	digitalWrite(LED_BLUE, LOW);
	digitalWrite(LED_GREEN, LOW);
	digitalWrite(LED_YELLOW, LOW);
	digitalWrite(LED_RED, LOW);
}

void loop() {
	// char msg[64];
	// snprintf(msg, sizeof(msg), "Current state: %s", state_to_string(g_current_state));
	// log_msg("DEBUG", msg);
	g_prev_button_state = debounceReadButton(BTN_PREV, &g_prev_button);
	g_select_button_state = debounceReadButton(BTN_SELECT, &g_select_button);
	g_next_button_state = debounceReadButton(BTN_NEXT, &g_next_button);
	// Serial.print("\n\tPREV: "); Serial.print(g_prev_button_state);
	// Serial.print("\n\tNEXT: "); Serial.print(g_next_button_state);
	// Serial.print("\n\tSELECT: "); Serial.print(g_select_button_state);

	handle_menu(g_current_state);
}
