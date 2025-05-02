#include "globals.h"
#include "menu.h"
#include "utils.h"
#include "states.h"
#include "Waveshare_LCD1602.h"
#include <SoftwareSerial.h>

Waveshare_LCD1602 lcd(16,2);
SoftwareSerial HM10_UART(9, 10);

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
	while (!Serial) {
		;
	}
	// HM10_UART.begin(115200);
	HM10_UART.begin(9600);
	delay(500);
	HM10_UART.println("AT+RESET");
	delay(500);
	while (HM10_UART.available()) {
		Serial.write(HM10_UART.read());
	}
	// while (!HM10_UART) {
	// 	;
	// }
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
	pinMode(11, INPUT);
	cycle_leds();
	digitalWrite(LED_BLUE, LOW);
	digitalWrite(LED_GREEN, LOW);
	digitalWrite(LED_YELLOW, LOW);
	digitalWrite(LED_RED, LOW);
	char msg[64];
	snprintf(msg, sizeof(msg), "STATE pin: %d", digitalRead(BT_STATE));
	log_msg("DEBUG", msg);
}

void loop() {
	// char msg[64];
	// snprintf(msg, sizeof(msg), "STATE pin: %d", digitalRead(BT_STATE));
	// log_msg("DEBUG", msg);

	if (Serial.available()) {
		HM10_UART.write(Serial.read());
	}
	if (HM10_UART.available()) {
		Serial.write(HM10_UART.read());
	}

	if (digitalRead(BT_STATE) == HIGH) {
		digitalWrite(LED_BLUE, HIGH);
	}
	else {
		digitalWrite(LED_BLUE, LOW);
	}
	// g_prev_button_state = debounceReadButton(BTN_PREV, &g_prev_button);
	// g_select_button_state = debounceReadButton(BTN_SELECT, &g_select_button);
	// g_next_button_state = debounceReadButton(BTN_NEXT, &g_next_button);

	// handle_menu(g_current_state);
}
