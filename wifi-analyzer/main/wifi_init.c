/*
 * wifi_init.c
 *
 *  Created on: 23 mars 2025
 *      Author: HP
 */
#include "wifi_init.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "esp_log.h"
#include <string.h>
static const char *TAG = "WiFi Init";

void disconnect_wifi(){
    esp_err_t err;

    err = esp_wifi_disconnect();
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "Wi-Fi disconnected successfully.");
    } else if (err == ESP_ERR_WIFI_NOT_CONNECT) {
        ESP_LOGW(TAG, "Wi-Fi was not connected.");
    } else {
        ESP_LOGE(TAG, "Failed to disconnect Wi-Fi: %s", esp_err_to_name(err));
    }
}


void reset_wifi() {

	disconnect_wifi();
    esp_wifi_stop();

    esp_netif_t* sta_netif = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
    esp_netif_t* ap_netif = esp_netif_get_handle_from_ifkey("WIFI_AP_DEF");
    if (sta_netif) {
        esp_netif_destroy(sta_netif);
    }
    if (ap_netif) {
        esp_netif_destroy(ap_netif);
    }

    esp_wifi_deinit();

    esp_event_loop_delete_default();

    esp_netif_deinit();

    nvs_handle_t nvs;
    if (nvs_open("nvs", NVS_READWRITE, &nvs) == ESP_OK) {
        nvs_erase_key(nvs, "wifi_sta_config");
        nvs_erase_key(nvs, "wifi_ap_config");
        nvs_commit(nvs);
        nvs_close(nvs);
    }

     nvs_flash_erase_partition("nvs");
     nvs_flash_init();

    printf("[Wi-Fi] Complete reinitialization and configuration reset\n");
}


void initialize_wifi() {

    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));

    ESP_ERROR_CHECK(esp_wifi_start());
}

static bool wifi_initialized = false;
static esp_netif_t *sta_netif = NULL;

void wifi_connect_to_ap() {
    wifi_config_t sta_config = {
        .sta = {
            .ssid = "Louey's Laptop",
            .password = "123456789",
            .scan_method = WIFI_FAST_SCAN,
            .failure_retry_cnt = 3,
            .threshold.authmode = WIFI_AUTH_WPA2_PSK
        },
    };

    if (!wifi_initialized) {

        esp_err_t ret = nvs_flash_init();
        if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
            nvs_flash_erase();
            nvs_flash_init();
        }

        ESP_ERROR_CHECK(esp_netif_init());
        ESP_ERROR_CHECK(esp_event_loop_create_default());

        sta_netif = esp_netif_create_default_wifi_sta();
        assert(sta_netif);

        wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
        ESP_ERROR_CHECK(esp_wifi_init(&cfg));
        ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));

        wifi_initialized = true;
    }

    esp_wifi_disconnect();

    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &sta_config));

    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_ERROR_CHECK(esp_wifi_connect());

    ESP_LOGI(TAG, "Connecting to SSID: %s", sta_config.sta.ssid);
}

char* get_ip_address(){
    static char ip_str[16];

    esp_netif_ip_info_t ip_info;
    esp_netif_t *netif = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");

    if (netif == NULL) {
        ESP_LOGE(TAG, "Failed to get netif handle");
        strcpy(ip_str, "0.0.0.0");
        return ip_str;
    }

    if (esp_netif_get_ip_info(netif, &ip_info) == ESP_OK) {
        snprintf(ip_str, sizeof(ip_str), IPSTR, IP2STR(&ip_info.ip));
    } else {
        strcpy(ip_str, "0.0.0.0");
    }

    return ip_str;
}
