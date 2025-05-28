// globals.h

#pragma once

// #include "utils.h"
#include "states.h"
#include <stdint.h>
#include <stdbool.h>
#include <SoftwareSerial.h>
#include "Waveshare_LCD1602.h"
#include <EEPROM.h>
#include <DS3231.h>
#include <TheThingsNetwork.h>

#define BRK_PIN 5
#define BTN_PREV 4
#define BTN_SELECT 7
#define BTN_NEXT 8
#define BUZZER A1
#define LED_RED A2
#define LED_YELLOW A3
#define LED_GREEN A4
#define LED_BLUE A5
#define BT_STATE 11

#define G_RECEIVED_DATA_BUFFER_SIZE 11
#define G_BP_SYS_MIN_ADDR 0
#define G_BP_SYS_MAX_ADDR 1
#define G_BP_DIAS_MIN_ADDR 2
#define G_BP_DIAS_MAX_ADDR 3
#define G_TEMP_MIN_ADDR 4
#define G_TEMP_MAX_ADDR 6
#define G_HR_MIN_ADDR 8
#define G_HR_MAX_ADDR 9
#define G_BP_SYSTOLIC_THRESHOLD_MIN 90
#define G_BP_SYSTOLIC_THRESHOLD_MAX 140
#define G_BP_DIASTOLIC_THRESHOLD_MIN 60
#define G_BP_DIASTOLIC_THRESHOLD_MAX 90
#define G_TEMP_THRESHOLD_MIN 350
#define G_TEMP_THRESHOLD_MAX 450
#define G_HR_THRESHOLD_MIN 60
#define G_HR_THRESHOLD_MAX 100
#define MAX_QUEUE_ITEMS 4
#define MAX_MSG_SIZE 11

#define STR_MENU_NAV F("<    SELECT    >")
#define STR_THRESHOLD_NAV F("< -  SELECT  + >")

extern uint8_t tx_retry_queue[MAX_QUEUE_ITEMS][MAX_MSG_SIZE];
extern uint8_t tx_retry_lengths[MAX_QUEUE_ITEMS];
extern uint8_t tx_retry_head;
extern uint8_t tx_retry_tail;
extern uint8_t tx_retry_count;

extern Waveshare_LCD1602 lcd;
extern SoftwareSerial HM10_UART;
extern DS3231 myRTC;
extern TheThingsNetwork ttn;

extern uint8_t g_prev_button_state;
extern uint8_t g_select_button_state;
extern uint8_t g_next_button_state;
extern uint8_t g_current_option_index;
extern uint8_t g_last_option_index_displayed;

extern uint8_t g_bp_systolic_threshold_min;
extern uint8_t g_bp_systolic_threshold_max;
extern uint8_t g_bp_diastolic_threshold_min;
extern uint8_t g_bp_diastolic_threshold_max;
extern uint8_t g_hr_threshold_min;
extern uint8_t g_hr_threshold_max;

extern uint8_t g_hr_readings_taken_this_hour;
extern uint8_t g_hr_target_minute;

extern uint16_t g_temp_threshold_min;
extern uint16_t g_temp_threshold_max;
extern uint16_t g_hr_readings_sum;

extern uint8_t g_last_uplink_minute;

extern char g_received_data_buffer[G_RECEIVED_DATA_BUFFER_SIZE];

extern const unsigned long g_startup_time;

extern states g_current_state;
extern states g_previous_state;
extern states g_setup_caller_state;

extern struct ButtonDebounce g_prev_button;
extern struct ButtonDebounce g_select_button;
extern struct ButtonDebounce g_next_button;

extern bool debug_enabled;
extern bool g_selection_pending;
extern bool g_multi_reset;
extern bool g_waiting_for_reading_bp;
extern bool g_waiting_for_reading_temp;
extern bool g_waiting_for_reading_hr;
extern bool g_time_synched;
