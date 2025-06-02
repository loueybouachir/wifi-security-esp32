/*
 * open_network.c
 *
 *  Created on: 25 mai 2025
 *      Author: HP
 */

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "string.h"
#include "open_network.h"

#define TAG "OpenNetwork"

void start_open_ap() {

    // Configure AP settings
    wifi_config_t ap_config = {
        .ap = {
            .ssid = "ESP_AP",
            .ssid_len = 6,
            .channel = 1,
            .password = "",
            .max_connection = 4,
            .authmode = WIFI_AUTH_OPEN // Open authentication
        },
    };

    // Apply configuration and start AP
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_AP, &ap_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "Open AP Started");
}


