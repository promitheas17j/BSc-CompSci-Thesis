#ifndef __Waveshare_LCD1602_H__
#define __Waveshare_LCD1602_H__

/*!
 *   LCD1602 Device I2C Address
 */
#define LCD_ADDRESS     (0x7c>>1)  // The I2C address of the LCD1602 (shifted by 1 for 7-bit addressing)

/*!
 *   LCD Command Set
 */
#define LCD_CLEARDISPLAY 0x01       // Command to clear the display
#define LCD_RETURNHOME 0x02         // Command to return the cursor to home position
#define LCD_ENTRYMODESET 0x04       // Command to set entry mode
#define LCD_DISPLAYCONTROL 0x08     // Command to control the display settings
#define LCD_CURSORSHIFT 0x10       // Command to shift cursor or display
#define LCD_FUNCTIONSET 0x20       // Command to set the function (like display mode)
#define LCD_SETCGRAMADDR 0x40      // Command to set the CGRAM address
#define LCD_SETDDRAMADDR 0x80      // Command to set the DDRAM address

/*!
 *   Entry Mode Flags for LCD
 */
#define LCD_ENTRYRIGHT 0x00        // Cursor moves right
#define LCD_ENTRYLEFT 0x02         // Cursor moves left
#define LCD_ENTRYSHIFTINCREMENT 0x01 // Shift the display to the right automatically
#define LCD_ENTRYSHIFTDECREMENT 0x00 // Shift the display to the left automatically

/*!
 *   Display Control Flags
 */
#define LCD_DISPLAYON 0x04         // Turn the display on
#define LCD_DISPLAYOFF 0x00        // Turn the display off
#define LCD_CURSORON 0x02          // Show the cursor
#define LCD_CURSOROFF 0x00         // Hide the cursor
#define LCD_BLINKON 0x01           // Blink the cursor
#define LCD_BLINKOFF 0x00          // Do not blink the cursor

/*!
 *   Cursor and Display Shift Flags
 */
#define LCD_DISPLAYMOVE 0x08       // Move the display
#define LCD_CURSORMOVE 0x00        // Move the cursor
#define LCD_MOVERIGHT 0x04         // Move the display/cursor to the right
#define LCD_MOVELEFT 0x00          // Move the display/cursor to the left

/*!
 *   Function Set Flags
 */
#define LCD_8BITMODE 0x10          // Set 8-bit mode
#define LCD_4BITMODE 0x00          // Set 4-bit mode
#define LCD_2LINE 0x08             // Use 2 lines
#define LCD_1LINE 0x00             // Use 1 line
#define LCD_5x8DOTS 0x00           // Set 5x8 dots for character display

/*!
 *   LED Device I2C Address
 */
#define SN3193_IIC_ADDRESS  0x6B   // I2C address of the SN3193 backlight controller

// SN3193 Register Definitions
#define SHUTDOWN_REG  0x00         // Register for software shutdown mode
#define BREATING_CONTROL_REG  0x01 // Register for breathing effect control
#define LED_MODE_REG  0x02        // Register for LED mode control
#define LED_NORNAL_MODE  0x00     // Normal mode for LED operation
#define LED_BREATH_MODE  0x20     // Breathing mode for LED operation

#define CURRENT_SETTING_REG  0x03 // Register to set the output current
#define PWM_1_REG  0x04          // PWM duty cycle data for channel 1
#define PWM_2_REG  0x05          // PWM duty cycle data for channel 2
#define PWM_3_REG  0x06          // PWM duty cycle data for channel 3
#define PWM_UPDATE_REG  0x07     // Register to load PWM settings

#define T0_1_REG  0x0A           // Set T0 time for OUT1
#define T0_2_REG  0x0B           // Set T0 time for OUT2
#define T0_3_REG  0x0C           // Set T0 time for OUT3

#define T1T2_1_REG  0x10         // Set T1&T2 time for OUT1
#define T1T2_2_REG  0x11         // Set T1&T2 time for OUT2
#define T1T2_3_REG  0x12         // Set T1&T2 time for OUT3

#define T3T4_1_REG  0x16         // Set T3&T4 time for OUT1
#define T3T4_2_REG  0x17         // Set T3&T4 time for OUT2
#define T3T4_3_REG  0x18         // Set T3&T4 time for OUT3

#define TIME_UPDATE_REG  0x1C    // Register to load time register data
#define LED_CONTROL_REG  0x1D    // Register to enable OUT1~OUT3 (LED outputs)
#define RESET_REG  0x2F          // Register to reset all registers to default values

/**
 * @brief Class representing the Waveshare LCD1602 I2C display and associated functions
 */
class Waveshare_LCD1602
{
public:
    Waveshare_LCD1602(uint8_t lcd_cols, uint8_t lcd_rows); // Constructor to initialize the LCD with the specified columns and rows
    
    void init();                           // Initialize the LCD
    void display();                        // Turn on the display
    void command(uint8_t value);           // Send a command to the LCD
    void lcd_send(uint8_t *data, uint8_t len);  // Send a series of data bytes to the LCD
    void setCursor(uint8_t col, uint8_t row);  // Set the cursor position on the display
    void clear();                          // Clear the display
    void data(uint8_t value);              // Send data to the LCD
    void send_string(const char *str);     // Display a string on the LCD
    void noCursor();                       // Hide the cursor
    void cursor();                         // Show the cursor
    void scrollDisplayLeft();              // Scroll the display content to the left
    void scrollDisplayRight();             // Scroll the display content to the right
    void leftToRight();                    // Set text flow from left to right
    void rightToLeft();                   // Set text flow from right to left
    void noAutoscroll();                   // Disable auto-scrolling of text
    void autoscroll();                     // Enable auto-scrolling of text
    void createChar(uint8_t location, uint8_t charmap[]);  // Create a custom character in the LCD's memory

    // SN3193 Backlight Control
    void led_send(uint8_t cmd, uint8_t data);   // Send command and data to the SN3193 LED controller
    void set_brightness(uint8_t value);         // Set the backlight brightness (0-100)
    void set_led_mode(uint8_t mode);            // Set the backlight LED mode (Normal or Breathing)

private:
    void begin(uint8_t cols, uint8_t rows);      // Initialize the display with column and row settings
    uint8_t _showfunction;                       // Display function configuration (e.g., mode, lines)
    uint8_t _showcontrol;                        // Display control flags (e.g., cursor, blink)
    uint8_t _showmode;                           // Entry mode flags (e.g., left-right movement)
    uint8_t _initialized;                        // Flag indicating whether the LCD has been initialized
    uint8_t _numlines, _currline;                // Number of lines and the current line for the display
    uint8_t _lcdAddr;                            // I2C address for the LCD
    uint8_t _ledAddr;                            // I2C address for the LED controller
    uint8_t _cols;                               // Number of columns on the LCD
    uint8_t _rows;                               // Number of rows on the LCD
    uint8_t _backlightval;                       // Backlight value for the LCD

    // SN3193 initialization for backlight
    void led_begin();                           // Initialize the SN3193 LED controller
};

#endif
