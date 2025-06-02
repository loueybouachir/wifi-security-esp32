/*
 * storage.c
 *
 *  Created on: 19 mars 2025
 *      Author: HP
 */

#include <stdio.h>
#include <string.h>
#include "esp_spiffs.h"
#include "storage.h"
#include "utils.h"
#include "esp_log.h"

#define TAG "STORAGE"
#define FILE_PATH "/spiffs/wifi_scan_results.txt"
#define MAX_LINE_LENGTH 256
#define MAX_ENTRY_STRING 256
wifi_network_t networks[MAX_NETWORKS];
int network_count = 0;


void init_storage(void) {

    esp_vfs_spiffs_unregister("storage");

    esp_vfs_spiffs_conf_t conf = {
        .base_path = "/spiffs",
        .partition_label = "storage",
        .max_files = 5,
        .format_if_mount_failed = true
    };

    esp_spiffs_format(conf.partition_label);

    esp_err_t ret = esp_vfs_spiffs_register(&conf);
    if (ret != ESP_OK) {
        printf("Erreur montage SPIFFS: %s\n", esp_err_to_name(ret));
        return;
    }

    // Verification
    size_t total = 0, used = 0;
    ret = esp_spiffs_info(conf.partition_label, &total, &used);
    if (ret != ESP_OK) {
        printf("Erreur info SPIFFS: %s\n", esp_err_to_name(ret));
    } else {
        printf("SPIFFS monte. Taille: %d, Utilise: %d\n", total, used);


    }
}
void reset_network_counter(void){

		    memset(networks, 0, sizeof(networks));
		    network_count = 0;
}
void save_scan_result(const char* ssid, int rssi, int authmode, int channel, const uint8_t* bssid) {

    if (network_count < MAX_NETWORKS) {
        strncpy(networks[network_count].ssid, ssid, sizeof(networks[network_count].ssid) - 1);
        networks[network_count].rssi = rssi;
        networks[network_count].authmode = authmode;
        networks[network_count].channel = channel;
        memcpy(networks[network_count].bssid, bssid, 6);
        network_count++;
        printf("Reseau enregistre en memoire : %s\n", ssid);
    } else {
        printf("Limite de reseaux atteinte, impossible d'enregistrer : %s\n", ssid);
    }
}

void print_scan_results(void) {
    /*printf("\n--- Resultats du scan Wi-Fi ---\n");
    for (int i = 0; i < network_count; i++) {
        printf("%d. SSID: %s | RSSI: %d | MAC: %02x:%02x:%02x:%02x:%02x:%02x | Securite: ",
               i + 1, networks[i].ssid, networks[i].rssi,
               networks[i].bssid[0], networks[i].bssid[1], networks[i].bssid[2],
               networks[i].bssid[3], networks[i].bssid[4], networks[i].bssid[5]);
        print_auth_mode(networks[i].authmode); // Afficher le mode de sécurité en texte
        printf("\n");
    }*/
}

void save_scan_results_csv() {
    if (network_count == 0) {
        ESP_LOGI(TAG, "No networks to save");
        return;
    }
    FILE* f = fopen("/spiffs/scan_results.csv", "w");
    if (f == NULL) {
        ESP_LOGE(TAG, "Failed to open scan_results.csv");
        return;
    }

    // Write CSV header
    fprintf(f, "SSID,BSSID,RSSI,AuthMode,Channel\n");

    // Write network data
    for (int i = 0; i < network_count; i++) {
        wifi_network_t *n = &networks[i];
        fprintf(f, "%s,%02x:%02x:%02x:%02x:%02x:%02x,%d,%d,%d\n",
                n->ssid,
                n->bssid[0], n->bssid[1], n->bssid[2],
                n->bssid[3], n->bssid[4], n->bssid[5],
                n->rssi, n->authmode, n->channel);
    }

    fclose(f);
    ESP_LOGI(TAG, "Saved %d networks to scan_results.csv", network_count);

    // Create combined_results.csv
    FILE* f_combined = fopen("/spiffs/combined_results.csv", "w");
    if (f_combined == NULL) {
        ESP_LOGE(TAG, "Failed to open combined_results.csv");
        return;
    }

    fseek(f_combined, 0, SEEK_END);
    if (ftell(f_combined) == 0) {
        fprintf(f_combined, "SSID,BSSID,RSSI,AuthMode,Channel\n");
    }

    // Append network data
    for (int i = 0; i < network_count; i++) {
        wifi_network_t *n = &networks[i];
        fprintf(f_combined, "%s,%02x:%02x:%02x:%02x:%02x:%02x,%d,%d,%d\n",
                n->ssid,
                n->bssid[0], n->bssid[1], n->bssid[2],
                n->bssid[3], n->bssid[4], n->bssid[5],
                n->rssi, n->authmode, n->channel);
    }

    fclose(f_combined);
    ESP_LOGI(TAG, "Appended %d networks to combined_results.csv", network_count);
}
void save_results_to_spiffs(void) {
    save_scan_results_csv();

    FILE* f = fopen(FILE_PATH, "w");
    if (f == NULL) {
        printf("Error opening file!\n");
        return;
    }

    for (int i = 0; i < network_count; i++) {
        fprintf(f, "%d. SSID: %s | RSSI: %d | MAC: %02x:%02x:%02x:%02x:%02x:%02x | Security: ",
                i+1, networks[i].ssid, networks[i].rssi,
                networks[i].bssid[0], networks[i].bssid[1], networks[i].bssid[2],
                networks[i].bssid[3], networks[i].bssid[4], networks[i].bssid[5]);

        switch(networks[i].authmode) {
            case 0: fprintf(f, "Open\n"); break;
            case 1: fprintf(f, "WEP\n"); break;
            case 2: fprintf(f, "WPA\n"); break;
            case 3: fprintf(f, "WPA2\n"); break;
            case 4: fprintf(f, "WPA/WPA2\n"); break;
            case 5: fprintf(f, "WPA3\n"); break;
            default: fprintf(f, "Unknown\n");
        }
    }
    fclose(f);
}

void read_results_from_spiffs(void) {
    FILE* f = fopen(FILE_PATH, "r");
    if (f == NULL) {
        printf("Erreur lors de l'ouverture du fichier SPIFFS !\n");
        return;
    }

    char buffer[128];
    printf("\n--- Resultats du scan Wi-Fi (depuis SPIFFS) ---\n");
    while (fgets(buffer, sizeof(buffer), f)) {
        printf("%s", buffer);
    }

    fclose(f);
}

void storage_write_file(const char* path, const char* data) {
    FILE* f = fopen(path, "a");
    if (f == NULL) {
        ESP_LOGE(TAG, "Failed to open file %s for writing", path);
        return;
    }

    fprintf(f, "%s", data);
    fclose(f);

    ESP_LOGI(TAG, "Data saved to %s", path);
}


char *get_simple_analyse_results(int cycle) {
    char file_path[64];
    snprintf(file_path, sizeof(file_path), "/spiffs/scan_report%d.csv", cycle);

    FILE *file = fopen(file_path, "r");
    if (!file) {
        ESP_LOGE("ANALYSE", "Failed to open file: %s", file_path);
        return NULL;
    }

    char line[MAX_LINE_LENGTH];
    int line_num = 0;

    char *result_buffer = malloc(MAX_NETWORKS * MAX_ENTRY_STRING);
    if (!result_buffer) {
        fclose(file);
        ESP_LOGE("ANALYSE", "Memory allocation failed.");
        return NULL;
    }
    result_buffer[0] = '\0';

    while (fgets(line, sizeof(line), file)) {
        if (line_num++ == 0) continue;

        line[strcspn(line, "\r\n")] = 0;

        // Parse line
        char ssid[64] = {0};
        char bssid[32] = {0};
        char security[32] = {0};

        char *token = strtok(line, ",");
        int field_index = 0;

        while (token != NULL) {
            switch (field_index) {
                case 0: strncpy(ssid, token, sizeof(ssid) - 1); break;
                case 1: strncpy(bssid, token, sizeof(bssid) - 1); break;
                case 6: strncpy(security, token, sizeof(security) - 1); break;
            }
            field_index++;
            token = strtok(NULL, ",");
        }

        if (strlen(ssid) && strlen(bssid) && strlen(security)) {
            char entry[MAX_ENTRY_STRING];
            snprintf(entry, sizeof(entry), "%s |Sec:%s \n", ssid, security);
            strncat(result_buffer, entry, MAX_NETWORKS * MAX_ENTRY_STRING - strlen(result_buffer) - 1);
        }
    }

    fclose(file);
    return result_buffer;
}


