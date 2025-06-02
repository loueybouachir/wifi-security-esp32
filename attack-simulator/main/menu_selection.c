/*
 * menu_selection.c
 *
 *  Created on: 25 mai 2025
 *      Author: HP
 */

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_task_wdt.h"

static const char *TAG = "menu_selection";

int select_attack_menu(void) {
    char input_buffer[128];
    char cmd[32];
    TickType_t last_yield_time = xTaskGetTickCount();

    ESP_LOGI(TAG, "=== Attack Selection Menu ===");

    while (1) {
        // Reset watchdog to prevent timeout
        esp_task_wdt_reset();

        // Display menu
        printf("\n=== Attack Selection Menu ===\n");
        printf("1. Unauthorized Channel Attack\n");
        printf("2. Evil Twin Attack\n");
        printf("3. Hidden Network Attack\n");
        printf("4. Deauthentication Attack\n");
        printf("5. Open Network Attack\n");
        printf("6. Stop and exit\n");
        printf("Enter command (1-6): ");

        // Non-blocking input reading
        int input_ready = 0;
        int chars_read = 0;
        memset(input_buffer, 0, sizeof(input_buffer));

        while (!input_ready) {
            int ch = getchar();
            if (ch != EOF && ch != 0xFF) {
                if (ch == '\n' || ch == '\r') {
                    input_ready = 1;
                    putchar('\n'); // Echo newline
                } else if (chars_read < sizeof(input_buffer) - 1) {
                    input_buffer[chars_read++] = (char)ch;
                    putchar(ch); // Echo character
                }
            }

            // Prevent watchdog timeout
            if (xTaskGetTickCount() - last_yield_time > pdMS_TO_TICKS(500)) {
                esp_task_wdt_reset();
                last_yield_time = xTaskGetTickCount();
                vTaskDelay(1); // Yield CPU to other tasks
            }

            vTaskDelay(10 / portTICK_PERIOD_MS);
        }

        if (chars_read == 0) {
            continue; // No input, loop again
        }

        input_buffer[chars_read] = '\0'; // Null-terminate input

        // Parse command
        memset(cmd, 0, sizeof(cmd));
        int parsed = sscanf(input_buffer, "%31s", cmd);

        if (parsed < 1) {
            ESP_LOGE(TAG, "No command entered");
            continue;
        }

        // Process command
        if (strcmp(cmd, "1") == 0) {
            ESP_LOGI(TAG, "Selected: Unauthorized Channel Attack");
            return 1;
        } else if (strcmp(cmd, "2") == 0) {
            ESP_LOGI(TAG, "Selected: Evil Twin Attack");
            return 2;
        } else if (strcmp(cmd, "3") == 0) {
            ESP_LOGI(TAG, "Selected: Hidden Network Attack");
            return 3;
        } else if (strcmp(cmd, "4") == 0) {
            ESP_LOGI(TAG, "Selected: Deauthentication Attack");
            return 4;
        } else if (strcmp(cmd, "5") == 0) {
            ESP_LOGI(TAG, "Selected: Open Network Attack");
            return 5;
        } else if (strcmp(cmd, "6") == 0) {
            ESP_LOGI(TAG, "Selected: Stop");
            return 6;
        } else {
            ESP_LOGE(TAG, "Invalid command: %s. Please enter 1-6.", cmd);
        }
    }

    ESP_LOGE(TAG, "Unexpected exit from menu");
    return -1;
}
