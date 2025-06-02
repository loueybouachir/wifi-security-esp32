/*
 * wifi_connect.c
 *
 *  Created on: 30 avr. 2025
 *      Author: HP
 */

#include "wifi_connect.h"
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "esp_log.h"

static const char *TAG = "wifi_connect";
static EventGroupHandle_t s_wifi_event_group;
#define WIFI_CONNECTED_BIT   BIT0
#define WIFI_FAIL_BIT        BIT1
static int s_retry_num = 0;
#ifndef MAX_RETRY
#define MAX_RETRY            10
#endif

static char ssid_buffer[32];
static char pass_buffer[64];
static const char *ssid_global = NULL;
static const char *pass_global = NULL;
static esp_netif_t *sta_netif = NULL;
static esp_event_handler_instance_t wifi_evt_instance;
static esp_event_handler_instance_t ip_evt_instance;

// Wi-Fi and IP event handler
static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                               int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
        ESP_LOGI(TAG, "WIFI_EVENT_STA_START: connecting");
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (s_retry_num < MAX_RETRY) {
            cleanup_and_reconnect();
            s_retry_num++;
            ESP_LOGW(TAG, "Retry %d/%d", s_retry_num, MAX_RETRY);
        } else {
            if (s_wifi_event_group != NULL) {
                xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
            }
        }
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_CONNECTED) {
        esp_netif_dhcpc_start(sta_netif);
        ESP_LOGI(TAG, "WIFI_EVENT_STA_CONNECTED: DHCP started");
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "Got IP: " IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
        if (s_wifi_event_group != NULL) {
            xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
        }
    }
}

void cleanup_and_reconnect(void)
{
    esp_event_handler_instance_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, wifi_evt_instance);
    esp_event_handler_instance_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, ip_evt_instance);

    if (sta_netif) esp_netif_dhcpc_stop(sta_netif);

    esp_wifi_disconnect();
    esp_wifi_stop();
    esp_wifi_deinit();

    esp_event_loop_delete_default();
    if (sta_netif) {
        esp_netif_destroy(sta_netif);
        sta_netif = NULL;
    }
    vTaskDelay(pdMS_TO_TICKS(100));

    esp_netif_init();
    esp_event_loop_create_default();
    sta_netif = esp_netif_create_default_wifi_sta();
    esp_netif_set_default_netif(sta_netif);

    esp_event_handler_instance_register(
        WIFI_EVENT, ESP_EVENT_ANY_ID, wifi_event_handler, NULL, &wifi_evt_instance);
    esp_event_handler_instance_register(
        IP_EVENT, IP_EVENT_STA_GOT_IP, wifi_event_handler, NULL, &ip_evt_instance);

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);
    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_set_ps(WIFI_PS_NONE);

    wifi_config_t wcfg = {
        .sta = {
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
        }
    };
    strlcpy(ssid_buffer, ssid_global, sizeof(ssid_buffer));
    strlcpy(pass_buffer, pass_global, sizeof(pass_buffer));
    strlcpy((char*)wcfg.sta.ssid, ssid_buffer, sizeof(wcfg.sta.ssid));
    strlcpy((char*)wcfg.sta.password, pass_buffer, sizeof(wcfg.sta.password));
    esp_wifi_set_config(WIFI_IF_STA, &wcfg);
    esp_wifi_start();
}

esp_err_t wifi_connect_sta(const char* ssid, const char* password)
{
    strlcpy(ssid_buffer, ssid, sizeof(ssid_buffer));
    strlcpy(pass_buffer, password, sizeof(pass_buffer));
    ssid_global = ssid_buffer;
    pass_global = pass_buffer;
    s_retry_num = 0;

    s_wifi_event_group = xEventGroupCreate();

    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        nvs_flash_erase();
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    esp_netif_init();
    esp_event_loop_create_default();

    sta_netif = esp_netif_create_default_wifi_sta();
    assert(sta_netif);
    esp_netif_set_default_netif(sta_netif);

    esp_event_handler_instance_register(
        WIFI_EVENT, ESP_EVENT_ANY_ID, wifi_event_handler, NULL, &wifi_evt_instance);
    esp_event_handler_instance_register(
        IP_EVENT, IP_EVENT_STA_GOT_IP, wifi_event_handler, NULL, &ip_evt_instance);

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);
    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_set_ps(WIFI_PS_NONE);

    wifi_config_t wifi_config = {
        .sta = {
            .scan_method = WIFI_FAST_SCAN,
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
        }
    };
    strlcpy((char*)wifi_config.sta.ssid, ssid_buffer, sizeof(wifi_config.sta.ssid));
    strlcpy((char*)wifi_config.sta.password, pass_buffer, sizeof(wifi_config.sta.password));
    esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
    esp_wifi_start();

    ESP_LOGI(TAG, "Connecting to %s (DHCP)", ssid_buffer);

    EventBits_t bits = xEventGroupWaitBits(
        s_wifi_event_group,
        WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
        pdFALSE, pdFALSE,
        portMAX_DELAY);

    esp_err_t result = (bits & WIFI_CONNECTED_BIT) ? ESP_OK : ESP_FAIL;
    if (result == ESP_OK) ESP_LOGI(TAG, "Connected via DHCP");
    else ESP_LOGE(TAG, "Failed DHCP after %d retries", MAX_RETRY);

    vEventGroupDelete(s_wifi_event_group);
    return result;
}

esp_err_t wifi_connect_static(const char* ssid, const char* password,
                              esp_netif_ip_info_t ip_info)
{

    strlcpy(ssid_buffer, ssid, sizeof(ssid_buffer));
    strlcpy(pass_buffer, password, sizeof(pass_buffer));
    ssid_global = ssid_buffer;
    pass_global = pass_buffer;

    if (sta_netif) {
        esp_netif_dhcpc_stop(sta_netif);
    } else {
        sta_netif = esp_netif_create_default_wifi_sta();
        esp_netif_set_default_netif(sta_netif);
    }

    esp_netif_set_ip_info(sta_netif, &ip_info);

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);
    esp_wifi_set_mode(WIFI_MODE_STA);

    static bool handlers_registered = false;
    if (!handlers_registered) {
        esp_event_handler_instance_register(
            WIFI_EVENT, ESP_EVENT_ANY_ID, wifi_event_handler, NULL, &wifi_evt_instance);
        esp_event_handler_instance_register(
            IP_EVENT, IP_EVENT_STA_GOT_IP, wifi_event_handler, NULL, &ip_evt_instance);
        handlers_registered = true;
    }

    wifi_config_t wcfg = {
        .sta = { .threshold.authmode = WIFI_AUTH_WPA2_PSK }
    };
    strlcpy((char*)wcfg.sta.ssid, ssid_buffer, sizeof(wcfg.sta.ssid));
    strlcpy((char*)wcfg.sta.password, pass_buffer, sizeof(wcfg.sta.password));
    esp_wifi_set_config(WIFI_IF_STA, &wcfg);
    esp_wifi_start();
    esp_wifi_connect();

    ESP_LOGI(TAG, "Connecting to %s (static IP)", ssid_buffer);
    return ESP_OK;
}
