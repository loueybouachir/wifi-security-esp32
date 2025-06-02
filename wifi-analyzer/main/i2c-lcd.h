/*
 * i2c-lcd.h
 *
 *  Created on: 29 avr. 2025
 *      Author: HP
 */

#ifndef MAIN_I2C_LCD_H_
#define MAIN_I2C_LCD_H_

#include "driver/i2c_master.h"
#include <stdint.h>


void lcd_init (void);   // initialize lcd

void lcd_send_cmd (char cmd);  // send command to the lcd

void lcd_send_data (char data);  // send data to the lcd

void lcd_send_string (const char *str);  // Send string to the LCD

void lcd_put_cur(int row, int col);  // put cursor at the entered position row (0 or 1), col (0-15);

void lcd_clear (void);

#endif /* MAIN_I2C_LCD_H_ */
