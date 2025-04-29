#include "Waveshare_LCD1602.h"
// #include <Wire.h>

const int BTN_PREV = 4;
const int BTN_SELECT = 7;
const int BTN_NEXT = 8;
const int LED_BLUE = A5;
const int LED_GREEN = A4;
const int LED_YELLOW = A3;
const int LED_RED = A2;

Waveshare_LCD1602 lcd(16,2);

char str_current_option[] = "Booting...";
int prev_button_state = 0;
int select_button_state = 0;
int next_button_state = 0;

void lcd_print_menu(char *option) {
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
	prev_button_state = digitalRead(BTN_PREV);
	select_button_state = digitalRead(BTN_SELECT);
	next_button_state = digitalRead(BTN_NEXT);

	Serial.println("PREV: " + String(prev_button_state) + " SELECT: " + String(select_button_state) + " NEXT: " + String(next_button_state));
	if (prev_button_state == HIGH) {
		Serial.println("BTN_PREV -> HIGH");
		// cycle_leds();
		digitalWrite(LED_BLUE, HIGH);
		digitalWrite(LED_GREEN, HIGH);
		digitalWrite(LED_YELLOW, LOW);
		digitalWrite(LED_RED, LOW);
	}
	else {
		digitalWrite(LED_BLUE, LOW);
		digitalWrite(LED_GREEN, LOW);
	}

	if (select_button_state == HIGH) {
		Serial.println("BTN_SELECT -> HIGH");
		// cycle_leds();
		digitalWrite(LED_GREEN, HIGH);
		digitalWrite(LED_YELLOW, HIGH);
		digitalWrite(LED_BLUE, LOW);
		digitalWrite(LED_RED, LOW);
	}
	else {
		digitalWrite(LED_GREEN, LOW);
		digitalWrite(LED_YELLOW, LOW);
	}

	if (next_button_state == HIGH) {
		Serial.println("BTN_NEXT -> HIGH");
		// cycle_leds();
		digitalWrite(LED_YELLOW, HIGH);
		digitalWrite(LED_RED, HIGH);
		digitalWrite(LED_BLUE, LOW);
		digitalWrite(LED_GREEN, LOW);
	}
	else {
		digitalWrite(LED_YELLOW, LOW);
		digitalWrite(LED_RED, LOW);
	}

	// delay(1000);
}
