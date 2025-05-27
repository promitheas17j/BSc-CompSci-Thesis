// menu.h

#pragma once

#include <stdint.h>
#include "states.h"

struct Menu {
	const char **options;
	uint8_t num_options;
};

extern struct Menu menu_table[];

states handle_menu(states current_state);
uint8_t handle_menu_options_buttons(const char **options, uint8_t num_options);
void lcd_print_line(const char *option, bool is_interactive);
