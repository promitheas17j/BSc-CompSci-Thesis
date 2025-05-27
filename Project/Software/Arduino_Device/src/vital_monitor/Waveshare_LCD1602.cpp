#include <Arduino.h>
#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include <Wire.h>

#include "Waveshare_LCD1602.h"

// Constructor to initialize the LCD with specified columns and rows
Waveshare_LCD1602::Waveshare_LCD1602(uint8_t lcd_cols, uint8_t lcd_rows)
{
	_cols = lcd_cols;  // Set the number of columns
	_rows = lcd_rows;  // Set the number of rows
}

// Initialize the LCD and set up I2C communication
void Waveshare_LCD1602::init()
{
#if defined(__AVR_ATmega2560__) || defined(__AVR_ATmega328P__) || defined(ARDUINO_UNOR4_MINIMA) || defined(ARDUINO_UNOR4_WIFI)
	Wire.begin();	   // Begin I2C communication
#elif defined(ARDUINO_ARCH_RP2040) || defined(ARDUINO_ARCH_RP2350)
	Wire.setSDA(4);    // Set the I2C data pin (SDA) to GPIO4
	Wire.setSCL(5);    // Set the I2C clock pin (SCL) to GPIO5
	Wire.begin();	   // Begin I2C communication
#else
	// Set the I2C data pin (SDA) to GPIO4
	// Set the I2C clock pin (SCL) to GPIO5
	// Wire.setPins(4, 5);
	Wire.begin();	   // Begin I2C communication
#endif
	if (_cols == 0) _cols = 16;
	if (_rows == 0) _rows = 2;
	_showfunction = LCD_4BITMODE | LCD_1LINE | LCD_5x8DOTS;  // Set the display function: 4-bit mode, 1 line, 5x8 character font
	begin(_cols, _rows);  // Initialize the LCD with the specified number of columns and rows
	// led_begin();		   // Initialize the LED backlight (if applicable)
}

// Set up the LCD with the desired number of rows and columns
void Waveshare_LCD1602::begin(uint8_t cols, uint8_t lines)
{
	if (lines > 1)	// If more than 1 row is specified, enable the 2-line mode
	{
		_showfunction |= LCD_2LINE;
	}
	_numlines = lines;	// Set the number of lines
	_currline = 0;		// Start with the first line

	delay(50);	// Wait for at least 50ms after powering up (ensures the display is ready)

	// Send the function set command to initialize the display
	command(LCD_FUNCTIONSET | _showfunction);  
	delay(5);  // Wait more than 4.1ms (as per the datasheet)

	// Send the function set command again to ensure it’s applied
	command(LCD_FUNCTIONSET | _showfunction);  
	delay(5);

	// Send the function set command one more time
	command(LCD_FUNCTIONSET | _showfunction);

	// Turn on the display without cursor or blinking
	_showcontrol = LCD_DISPLAYON | LCD_CURSOROFF | LCD_BLINKOFF;
	display();	// Apply the display control settings

	clear();  // Clear the display and set the cursor to the home position

	// Set the entry mode for text direction and cursor movement (default left to right, no auto-scroll)
	_showmode = LCD_ENTRYLEFT | LCD_ENTRYSHIFTDECREMENT;
	command(LCD_ENTRYMODESET | _showmode);	// Set the entry mode for the display
}

// Send a command to the LCD
inline void Waveshare_LCD1602::command(uint8_t value)
{
	uint8_t data[3] = {0x80, value};  // 0x80 indicates a command
	lcd_send(data, 2);	// Send the command to the LCD
}

// Send data to the LCD over I2C
void Waveshare_LCD1602::lcd_send(uint8_t *data, uint8_t len)
{
	Wire.beginTransmission(LCD_ADDRESS);  // Start I2C communication with the LCD device (at LCD_ADDRESS)
	for (int i = 0; i < len; i++)
	{
		Wire.write(data[i]);  // Send each byte of data
		delay(5);  // Small delay between each byte to ensure proper transmission
	}
	Wire.endTransmission();  // End the I2C transmission
}

// Turn on the display
void Waveshare_LCD1602::display()
{
	_showcontrol |= LCD_DISPLAYON;	// Set the display to "on"
	command(LCD_DISPLAYCONTROL | _showcontrol);  // Apply the display control settings
}

// Clear the display and reset the cursor position
void Waveshare_LCD1602::clear()
{
	command(LCD_CLEARDISPLAY);	// Send the clear display command
	delayMicroseconds(2000);  // Wait for 2ms to allow the command to complete (this takes time)
}


// Set the cursor position on the LCD display
void Waveshare_LCD1602::setCursor(uint8_t col, uint8_t row)
{
	// Depending on the row, set the appropriate base address for the column
	col = (row == 0 ? col | 0x80 : col | 0xc0);  // 0x80 for row 0, 0xc0 for row 1
	uint8_t data[3] = {0x80, col};	// 0x80 indicates a command

	lcd_send(data, 2);	// Send the command to set the cursor position
}

// Write data to the LCD
void Waveshare_LCD1602::data(uint8_t value)
{
	uint8_t data[3] = {0x40, value};  // 0x40 indicates data (not a command)
	lcd_send(data, 2);	// Send the data byte to the LCD
}

// Send a string to the LCD display
void Waveshare_LCD1602::send_string(const char *str)
{
	uint8_t i;
	// Loop through each character of the string and send it to the display
	for (i = 0; str[i] != '\0'; i++)
		data(str[i]);  // Send each character as data
}

void Waveshare_LCD1602::send_string(const __FlashStringHelper *fstr) {
	PGM_P p = reinterpret_cast<PGM_P>(fstr);
	char c;
	while ((c = pgm_read_byte(p++)) != 0) {
		data(c);  // reuse your existing send-character method
	}
}

// Disable the cursor (hide the cursor)
// void Waveshare_LCD1602::noCursor()
// {
//	_showcontrol &= ~LCD_CURSORON;	// Clear the LCD_CURSORON bit to disable the cursor
//	command(LCD_DISPLAYCONTROL | _showcontrol);  // Send the command to update the display control
// }

// Enable the cursor (show the cursor)
// void Waveshare_LCD1602::cursor()
// {
//	_showcontrol |= LCD_CURSORON;  // Set the LCD_CURSORON bit to enable the cursor
//	command(LCD_DISPLAYCONTROL | _showcontrol);  // Send the command to update the display control
// }

// Scroll the display content to the left
// void Waveshare_LCD1602::scrollDisplayLeft(void)
// {
//	command(LCD_CURSORSHIFT | LCD_DISPLAYMOVE | LCD_MOVELEFT);	// Shift the display left
// }

// // Scroll the display content to the right
// void Waveshare_LCD1602::scrollDisplayRight(void)
// {
//	command(LCD_CURSORSHIFT | LCD_DISPLAYMOVE | LCD_MOVERIGHT);  // Shift the display right
// }

// Set the text direction to left-to-right
// void Waveshare_LCD1602::leftToRight(void)
// {
//	_showmode |= LCD_ENTRYLEFT;  // Set the entry mode for left-to-right direction
//	command(LCD_ENTRYMODESET | _showmode);	// Send the command to update the entry mode
// }

// Set the text direction to right-to-left
// void Waveshare_LCD1602::rightToLeft(void)
// {
//	_showmode &= ~LCD_ENTRYLEFT;  // Clear the LCD_ENTRYLEFT bit for right-to-left direction
//	command(LCD_ENTRYMODESET | _showmode);	// Send the command to update the entry mode
// }

// Disable auto-scrolling of text
// void Waveshare_LCD1602::noAutoscroll(void)
// {
//	_showmode &= ~LCD_ENTRYSHIFTINCREMENT;	// Clear the LCD_ENTRYSHIFTINCREMENT bit
//	command(LCD_ENTRYMODESET | _showmode);	// Send the command to update the entry mode
// }

// Enable auto-scrolling of text
// void Waveshare_LCD1602::autoscroll(void)
// {
//	_showmode |= LCD_ENTRYSHIFTINCREMENT;  // Set the LCD_ENTRYSHIFTINCREMENT bit to enable auto-scroll
//	command(LCD_ENTRYMODESET | _showmode);	// Send the command to update the entry mode
// }

// Create a custom character on the LCD
// void Waveshare_LCD1602::createChar(uint8_t location, uint8_t charmap[])
// {
//	location &= 0x7;  // We only have 8 locations (0-7) for custom characters
//	command(LCD_SETCGRAMADDR | (location << 3));  // Set the address for the custom character in CGRAM

//	uint8_t data[9];
//	data[0] = 0x40;  // 0x40 indicates the start of character data
//	// Copy the character map into the data array
//	for (int i = 0; i < 8; i++)
//	{
//		data[i + 1] = charmap[i];  // Copy each byte of the character map
//	}
//	lcd_send(data, 9);	// Send the character data to the LCD
// }


// SN3193 LED control functions

// Send a command and data byte to the SN3193 chip over I2C
// void Waveshare_LCD1602::led_send(uint8_t cmd, uint8_t data)
// {
//	Wire.beginTransmission(SN3193_IIC_ADDRESS); // Start I2C transmission to the SN3193 chip
//	Wire.write(cmd);  // Send the command byte
//	delay(5);  // Small delay to ensure data is transmitted
//	Wire.write(data);  // Send the data byte
//	Wire.endTransmission(); // End the I2C transmission
// }

// // Initialize the LED controller (SN3193)
// void Waveshare_LCD1602::led_begin()
// {
//	// Configure the LED controller
//	led_send(SHUTDOWN_REG, 0x20);  // Wake up the chip from shutdown mode
//	led_send(LED_MODE_REG, LED_NORNAL_MODE);  // Set the LED to normal operation mode
//	led_send(CURRENT_SETTING_REG, 0x00);  // Set the maximum output current (42mA)
//	delay(10);	// Wait for settings to take effect

//	// Set the PWM duty cycle (0x00 to 0xFF)
//	led_send(PWM_1_REG, 0x00);	
//	delay(100);  // Wait for the PWM setting to stabilize
//	led_send(PWM_UPDATE_REG, 0x00);  // Apply the PWM settings

//	// Set time-related settings for each output channel (OUT1, OUT2, OUT3)
//	led_send(T0_1_REG, 0x40);  // Set T0 time for OUT1
//	led_send(T0_2_REG, 0x40);  // Set T0 time for OUT2
//	led_send(T0_3_REG, 0x40);  // Set T0 time for OUT3
//	delay(100);  // Wait for the time settings to take effect

//	// Set T1 and T2 times for each output channel
//	led_send(T1T2_1_REG, 0x26);  // Set T1&T2 times for OUT1
//	led_send(T1T2_2_REG, 0x26);  // Set T1&T2 times for OUT2
//	led_send(T1T2_3_REG, 0x26);  // Set T1&T2 times for OUT3
//	delay(100);  // Wait for the T1&T2 settings to take effect

//	// Set T3 and T4 times for each output channel
//	led_send(T3T4_1_REG, 0x26);  // Set T3&T4 times for OUT1
//	led_send(T3T4_2_REG, 0x26);  // Set T3&T4 times for OUT2
//	led_send(T3T4_3_REG, 0x26);  // Set T3&T4 times for OUT3
//	delay(100);  // Wait for the T3&T4 settings to take effect

//	// Enable the LED outputs (OUT1, OUT2, OUT3) and start LED operation
//	led_send(LED_CONTROL_REG, 0x01);  // Enable the outputs
//	led_send(TIME_UPDATE_REG, 0x00);  // Apply the time register settings
//	delay(100);  // Wait for the settings to take effect
// }

// // Set the brightness of the LEDs (value between 0 and 100)
// void Waveshare_LCD1602::set_brightness(uint8_t value)
// {
//	// Check if the input value is within the valid range (0-100)
//	if (value < 0 || value > 100)
//	{
//		Serial.println(F("Please enter a value between 0 and 100."));	// Print an error message if the value is invalid
//	}
//	else
//	{
//		// Set the PWM duty cycle based on the brightness percentage
//		led_send(PWM_1_REG, (int)(value * (0xFF / 100)));  // Convert percentage to 0-255 scale
//		delay(100);  // Wait for the PWM settings to take effect
//		led_send(PWM_UPDATE_REG, 0x00);  // Apply the PWM settings
//	}
// }

// // Set the operation mode of the LEDs (e.g., breathing mode or steady mode)
// void Waveshare_LCD1602::set_led_mode(uint8_t mode)
// {
//	led_send(LED_MODE_REG, mode);  // Set the LED mode
//	// Mode options: 0x20 for breathing mode, 0x00 for steady mode
// }
