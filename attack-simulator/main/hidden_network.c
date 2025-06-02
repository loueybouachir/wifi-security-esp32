/*
 * hidden_network.c
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
#include "hidden_network.h"

#define TAG "HiddenNetwork"

void start_hidden_ap() {

    // Configure AP settings
    wifi_config_t ap_config = {
        .ap = {
            .ssid = "Hidden",
            .ssid_len = strlen("Hidden"),
			.ssid_hidden = 1,
            .channel = 1,
            .password = "password",
            .max_connection = 4,
            .authmode = WIFI_AUTH_WPA2_PSK
        },
    };

    // Apply configuration and start AP
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_AP, &ap_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "Hidden AP Started");
}



