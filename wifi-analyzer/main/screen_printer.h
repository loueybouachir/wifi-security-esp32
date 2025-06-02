/*
 * screen_printer.h
 *
 *  Created on: 29 avr. 2025
 *      Author: HP
 */

#ifndef MAIN_SCREEN_PRINTER_H_
#define MAIN_SCREEN_PRINTER_H_

#include <stdint.h>

void init_lcd();

void clear_lcd(void);

void clear_line(int line);

void lcd_command(uint8_t cmd);

void print_on_line_1(const char *text);

void print_on_line_2(const char *text);

void scroll_text(const char *str, uint8_t line);

#endif /* MAIN_SCREEN_PRINTER_H_ */
