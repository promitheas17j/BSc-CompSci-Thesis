#include "Waveshare_LCD1602.h"
#include <Wire.h>

Waveshare_LCD1602 lcd(16,2);

String current_option = "Martin";

void lcd_print_menu(String option) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.send_string(option.c_str());   
  lcd.setCursor(0, 1);
  lcd.send_string("<    SELECT    >");
}

void setup() {
  Serial.begin(9600);
  lcd.init();
  lcd.clear();
  lcd_print_menu(current_option);
}

void loop() {
  if (Serial.available() > 0) {
    char btn_pressed = Serial.read();

    if (btn_pressed == '1') { // Left
      lcd_print_menu("Opt 3");
    }
    else if (btn_pressed == '2') { // Select
    lcd.clear();
      lcd.setCursor(0, 0);
      lcd.send_string("SELECTED!!!");
    }
    else if (btn_pressed == '3') { // Right
      lcd_print_menu("Opt 1");
    }
    else {
      lcd_print_menu("INVALID INPUT!");
    }
  }
  
  delay(150);
}
