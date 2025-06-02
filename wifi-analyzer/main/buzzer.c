/*
 * buzzer.c
 *
 *  Created on: 28 avr. 2025
 *      Author: HP
 */


#include "buzzer.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>
#include <stdio.h>
#include "networks_status.h"

#define BUZZER_GPIO GPIO_NUM_19
#define BUZZER_ON_TIME_MS 2000
#define BUZZER_CHECK_INTERVAL_MS 1000
#define CSV_FILE_PATH   "/spiffs/comparative_report.csv"


// Initialize the buzzer GPIO
void buzzer_init(void)
{
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << BUZZER_GPIO),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf);
}

void buzzer_beep()
{
    gpio_set_level(BUZZER_GPIO, 1);
    vTaskDelay(500 / portTICK_PERIOD_MS);
    gpio_set_level(BUZZER_GPIO, 0);
    vTaskDelay(500 / portTICK_PERIOD_MS);
    gpio_set_level(BUZZER_GPIO, 1);
    vTaskDelay(500 / portTICK_PERIOD_MS);
    gpio_set_level(BUZZER_GPIO, 0);
    vTaskDelay(500 / portTICK_PERIOD_MS);
}

void buzzer_test(void) {
    buzzer_init();

        gpio_set_level(BUZZER_GPIO, 1);
        vTaskDelay(500 / portTICK_PERIOD_MS);
        gpio_set_level(BUZZER_GPIO, 0);
        vTaskDelay(500 / portTICK_PERIOD_MS);
        gpio_set_level(BUZZER_GPIO, 1);
        vTaskDelay(500 / portTICK_PERIOD_MS);
        gpio_set_level(BUZZER_GPIO, 0);
        vTaskDelay(500 / portTICK_PERIOD_MS);
}
