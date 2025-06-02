/*
 * led_manager.h
 *
 *  Created on: 29 avr. 2025
 *      Author: HP
 */

#ifndef MAIN_LED_MANAGER_H_
#define MAIN_LED_MANAGER_H_

#include <stdbool.h>

void safe_led_init(void);

void danhger_led_init(void);

void danger_led_on(void);

void danger_led_off(void);

void danger_led_test(void);

void safe_led_on(void);

void safe_led_off(void);

void safe_led_test(void);

#endif /* MAIN_LED_MANAGER_H_ */
