/*
 * wifi_scan.c
 *
 *  Created on: 19 mars 2025
 *      Author: HP
 */
#include <stdio.h>
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"
#include "esp_wifi.h"
#include "storage.h"
void wifi_scan() {
    printf("\n--- Lancement du scan Wi-Fi ---\n");

    wifi_scan_config_t scan_config = {
        .ssid = NULL,
        .bssid = NULL,
        .channel = 0,
        .show_hidden = true
    };

    esp_err_t ret = esp_wifi_scan_start(&scan_config, true);
    if (ret != ESP_OK) {
        printf(" Erreur lors du demarrage du scan Wi-Fi: %s\n", esp_err_to_name(ret));
        return;
    }

    uint16_t num_ap = 0;
    esp_wifi_scan_get_ap_num(&num_ap);

    if (num_ap == 0) {
        printf(" Aucun reseau Wi-Fi detecte.\n");
        return;
    }

    wifi_ap_record_t ap_records[num_ap];
    if (esp_wifi_scan_get_ap_records(&num_ap, ap_records) != ESP_OK) {
        printf(" Erreur lors de la recuperation des rsultats du scan Wi-Fi\n");
        return;
    }

    reset_network_counter();

    printf("\n Reseaux Wi-Fi detectes :\n");
    for (int i = 0; i < num_ap; i++) {
        if (ap_records[i].ssid[0] == '\0') {
        	save_scan_result("Hidden Network", ap_records[i].rssi, ap_records[i].authmode, ap_records[i].primary, ap_records[i].bssid);
            continue;
        }

        save_scan_result((const char*)ap_records[i].ssid, ap_records[i].rssi, ap_records[i].authmode, ap_records[i].primary, ap_records[i].bssid);
    }

    print_scan_results();

    save_results_to_spiffs();

    read_results_from_spiffs();
}
