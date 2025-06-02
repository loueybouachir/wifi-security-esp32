/*
 * packet_capture.c
 *
 *  Created on: 27 mars 2025
 *      Author: HP
 */

#include "packet_capture.h"
#include "esp_wifi.h"
#include "esp_system.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "storage.h"
#include <stdio.h>
#include <string.h>

#define TAG "PACKET_SNIFFER"
#define MAX_CHANNELS 14
#define MAX_SSID_LENGTH 32

static packet_info_t captured_packets[500];
static int packet_count = 0;

// Improved packet handler with proper SSID extraction
static void wifi_sniffer_packet_handler(void *buff, wifi_promiscuous_pkt_type_t type) {
    if (packet_count >= 500) return;

    const wifi_promiscuous_pkt_t *pkt = (wifi_promiscuous_pkt_t *)buff;
    const wifi_pkt_rx_ctrl_t *ctrl = &pkt->rx_ctrl;
    const uint8_t *frame = pkt->payload;

    packet_info_t *p = &captured_packets[packet_count];

    // Set basic info
    p->channel = ctrl->channel;
    p->rssi = ctrl->rssi;
    p->length = ctrl->sig_len;



    // Set MAC source
    if (type == WIFI_PKT_MGMT) {
        snprintf(p->mac_source, sizeof(p->mac_source),
                "%02x:%02x:%02x:%02x:%02x:%02x",
                frame[10], frame[11], frame[12],
                frame[13], frame[14], frame[15]);
    } else {
        snprintf(p->mac_source, sizeof(p->mac_source),
                "%02x:%02x:%02x:%02x:%02x:%02x",
                frame[4], frame[5], frame[6],
                frame[7], frame[8], frame[9]);
    }

    // Set packet type
    switch(type) {
        case WIFI_PKT_MGMT:
            if (frame[0] == 0x80) { // Beacon frame
                strcpy(p->type, "Beacon");
                uint8_t ssid_length = frame[37];
                if (ssid_length > 0 && ssid_length <= MAX_SSID_LENGTH) {
                    memcpy(p->ssid, &frame[38], ssid_length);
                    p->ssid[ssid_length] = '\0'; // Ensure null-termination
                    ESP_LOGD(TAG, "Beacon frame with SSID: %s (length: %d)", p->ssid, ssid_length);
                }
            } else if (frame[0] == 0x40) { // Probe Request
                strcpy(p->type, "Probe Request");
            } else if (frame[0] == 0x50) { // Probe Response
                strcpy(p->type, "Probe Response");
                uint8_t ssid_length = frame[37];
                if (ssid_length > 0 && ssid_length <= MAX_SSID_LENGTH) {
                    memcpy(p->ssid, &frame[38], ssid_length);
                    p->ssid[ssid_length] = '\0';
                }
            } else if (frame[0] == 0xC0) { // Deauthentication
                strcpy(p->type, "Deauthentication");
            }else {
                snprintf(p->type, sizeof(p->type), "MGMT 0x%02X", frame[0]);
            }
            break;

        case WIFI_PKT_CTRL:
            strcpy(p->type, "Control");
            break;

        case WIFI_PKT_DATA:
            strcpy(p->type, "Data");

            break;

        default:
            strcpy(p->type, "Unknown");
    }

    packet_count++;
    ESP_LOGD(TAG, "Packet %d: Ch:%d RSSI:%d %s %s SSID:%s",
            packet_count, p->channel, p->rssi, p->type, p->mac_source, p->ssid);
}

void wifi_sniffer_init(void) {
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_NULL));
    ESP_ERROR_CHECK(esp_wifi_start());
}

void wifi_sniffer_start(int channel) {
    ESP_ERROR_CHECK(esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE));
    ESP_ERROR_CHECK(esp_wifi_set_promiscuous(true));
    ESP_ERROR_CHECK(esp_wifi_set_promiscuous_rx_cb(&wifi_sniffer_packet_handler));
    ESP_LOGI(TAG, "Started capture on channel %d", channel);
}

void wifi_sniffer_stop(void) {
    save_packets_to_csv();
    ESP_ERROR_CHECK(esp_wifi_set_promiscuous(false));
    ESP_LOGI(TAG, "Stopped capture. Saved %d packets", packet_count);
    packet_count = 0;
}

void save_packets_to_csv() {
    if (packet_count == 0) {
        ESP_LOGI(TAG, "No packets to save");
        return;
    }

    // Save to channel specific file
    char filename[50];
    snprintf(filename, sizeof(filename), "/spiffs/packets_ch%d.csv", captured_packets[0].channel);

    FILE* f = fopen(filename, "w");
    if (f == NULL) {
        ESP_LOGE(TAG, "Failed to open file %s", filename);
        return;
    }

    // CSV header
    fprintf(f, "Channel,RSSI,Type,SourceMAC,DestinationMAC,SSID,Length\n");

    for (int i = 0; i < packet_count; i++) {
        packet_info_t *p = &captured_packets[i];
        fprintf(f, "%d,%d,%s,%s,%s,%s,%d\n",
                p->channel ? p->channel : 0,
                p->rssi ? p->rssi : 0,
                p->type[0] ? p->type : "No Type",
                p->mac_source[0] ? p->mac_source : "No MAC Source",
                p->mac_dest[0] ? p->mac_dest : "No MAC Destination",
                p->ssid[0] ? p->ssid : "No SSID",
                p->length ? p->length : 0);
    }

    fclose(f);
    ESP_LOGI(TAG, "Saved %d packets to %s", packet_count, filename);

    // Append packets to combined file
    char combined_filename[50] = "/spiffs/combined_results.csv";

    FILE* f_combined = fopen(combined_filename, "a");
    if (f_combined == NULL) {
        ESP_LOGE(TAG, "Failed to open file %s", combined_filename);
        return;
    }

    fseek(f_combined, 0, SEEK_END);
    if (ftell(f_combined) == 0) {
        fprintf(f_combined, "Channel,RSSI,Type,SourceMAC,DestinationMAC,SSID,Length\n");
    }

    for (int i = 0; i < packet_count; i++) {
        packet_info_t *p = &captured_packets[i];
        fprintf(f_combined, "%d,%d,%s,%s,%s,%s,%d\n",
                p->channel ? p->channel : 0,
                p->rssi ? p->rssi : 0,
                p->type[0] ? p->type : "No Type",
                p->mac_source[0] ? p->mac_source : "No MAC Source",
                p->mac_dest[0] ? p->mac_dest : "No MAC Destination",
                p->ssid[0] ? p->ssid : "No SSID",
                p->length ? p->length : 0);
    }

    fclose(f_combined);
    ESP_LOGI(TAG, "Appended %d packets to %s", packet_count, combined_filename);
}
