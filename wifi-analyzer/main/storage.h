/*
 * storage.h
 *
 *  Created on: 19 mars 2025
 *      Author: HP
 */

#ifndef MAIN_STORAGE_H_
#define MAIN_STORAGE_H_

#include <stdint.h>
#include "esp_err.h"

#define MAX_NETWORKS 20 // Nombre maximal de réseaux à enregistrer
#define FILE_PATH "/spiffs/wifi_scan_results.txt" // Chemin du fichier SPIFFS

typedef struct {
    char ssid[32];    // SSID du réseau
    int rssi;         // Puissance du signal
    int authmode;     // Mode d'authentification
    uint8_t bssid[6]; // Adresse MAC (BSSID)
    int channel;	  // Chaine utilisé
} wifi_network_t;

typedef struct {
    char ssid[64];
    char bssid[32];
    char security[32];
} wifi_simple_info_t;

void init_storage(void);

void save_scan_result(const char* ssid, int rssi, int authmode, int channel, const uint8_t* bssid);

void print_scan_results(void);

void save_results_to_spiffs(void);

void read_results_from_spiffs(void);

void storage_write_file(const char* path, const char* data);

void save_scan_results_csv(void);

void reset_network_counter(void);

char *get_simple_analyse_results(int cycle);


#endif /* MAIN_STORAGE_H_ */
