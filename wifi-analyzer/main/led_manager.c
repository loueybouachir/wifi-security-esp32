/*
 * led_manager.c
 *
 *  Created on: 29 avr. 2025
 *      Author: HP
 */


#include "led_manager.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"


#define danger_led_pin GPIO_NUM_13
#define safe_led_pin GPIO_NUM_14
#define BLINK_DELAY_MS 500


// Initialize the LED GPIO
void danger_led_init(void)
{
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << danger_led_pin),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf);
}

void safe_led_init(void)
{
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << safe_led_pin),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf);
}

void danger_led_on(void) {
        gpio_set_level(danger_led_pin, 1);
        vTaskDelay(BLINK_DELAY_MS / portTICK_PERIOD_MS);
        gpio_set_level(danger_led_pin, 0);
        vTaskDelay(BLINK_DELAY_MS / portTICK_PERIOD_MS);
        gpio_set_level(danger_led_pin, 1);
        vTaskDelay(BLINK_DELAY_MS / portTICK_PERIOD_MS);
        gpio_set_level(danger_led_pin, 0);
        vTaskDelay(BLINK_DELAY_MS / portTICK_PERIOD_MS);
}

void danger_led_off(void) {
    gpio_set_level(danger_led_pin, 0);
}

void danger_led_test(void) {

    for (int i = 0; i < 2; i++) {
    	danger_led_init();
        gpio_set_level(danger_led_pin, 1);
        vTaskDelay(BLINK_DELAY_MS / portTICK_PERIOD_MS);
        gpio_set_level(danger_led_pin, 0);
        vTaskDelay(BLINK_DELAY_MS / portTICK_PERIOD_MS);
    }
    printf("Danger Led ON !\n");
}

void safe_led_on(void) {
    gpio_set_level(safe_led_pin, 1);
}

void safe_led_off(void) {
    gpio_set_level(safe_led_pin, 0);
}

void safe_led_test(void) {
    for (int i = 0; i < 2; i++) {
    	safe_led_init();
        gpio_set_level(safe_led_pin, 1);
        vTaskDelay(BLINK_DELAY_MS / portTICK_PERIOD_MS);
        gpio_set_level(safe_led_pin, 0);
        vTaskDelay(BLINK_DELAY_MS / portTICK_PERIOD_MS);
        gpio_set_level(safe_led_pin, 1);
    }
    printf("Safe Led ON !\n");
}
