/*
 * unothorized_channel.c
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
#include "unothorized_channel.h"

#define TAG "UnothorizedChannel"

void start_unothorized_channel_ap() {


    // Configure WiFi in AP mode
    wifi_country_t country = {
            .cc = "JP",
            .schan = 1,
            .nchan = 14,
            .policy = WIFI_COUNTRY_POLICY_MANUAL
        };
        ESP_ERROR_CHECK(esp_wifi_set_country(&country));

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));


    // Configure AP settings
    wifi_config_t ap_config = {
        .ap = {
            .ssid = "ESP_AP",
            .ssid_len = 6,
            .channel = 14,
            .password = "password",
            .max_connection = 4,
            .authmode = WIFI_AUTH_WPA2_PSK
        },
    };

    // Apply configuration and start AP
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_AP, &ap_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "Unothorized Channel AP Started");
}



