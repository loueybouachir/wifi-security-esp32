/*
 * networks_status.c
 *
 *  Created on: 27 avr. 2025
 *      Author: HP
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include "esp_log.h"
#include "networks_status.h"

#define MAX_LINE_LENGTH 256
#define CSV_FILE_PATH   "/spiffs/comparative_report.csv"

typedef struct {
    char type[32];
    char ssid[64];
    char bssid[32];
    int channel;
    float cur_rssi;
    float avg_rssi;
    char prev_auth[32];
    char cur_auth[32];
    int ev_twin;
    int deauth;
    char security_evaluation[32];
    int authorized;
} NetworkEntry;


static bool parse_csv_line(const char *line, NetworkEntry *entry) {
    return (sscanf(line, "%31[^,],%63[^,],%31[^,],%d,%f,%f,%31[^,],%31[^,],%d,%d,%31[^,],%d",
                   entry->type, entry->ssid, entry->bssid, &entry->channel,
                   &entry->cur_rssi, &entry->avg_rssi,
                   entry->prev_auth, entry->cur_auth,
                   &entry->ev_twin, &entry->deauth, entry->security_evaluation, &entry->authorized) == 12);
}

char *analyze_comparative_report(const char *filepath) {
    FILE *file = fopen(filepath, "r");
    if (!file) {
        ESP_LOGE("CSV_ANALYSIS", "Failed to open file: %s", filepath);
        return strdup("Error: Cannot open file.\n");
    }

    char line[MAX_LINE_LENGTH];
    bool first_line = true;
    size_t buffer_size = 512;
    char *result = malloc(buffer_size);
    if (!result) {
        fclose(file);
        return strdup("Error: Memory allocation failed.\n");
    }
    result[0] = '\0';

    while (fgets(line, sizeof(line), file)) {
        if (first_line) {
            first_line = false;
            continue;
        }

        NetworkEntry entry;
        if (parse_csv_line(line, &entry)) {
            char temp[512] = {0};

            if (entry.ev_twin == 1) {
                snprintf(temp, sizeof(temp), "Evil Twin detected: SSID=%s, BSSID=%s\n", entry.ssid, entry.bssid);
            } else if (entry.deauth == 1) {
                snprintf(temp, sizeof(temp), "Deauth attack detected: SSID=%s, BSSID=%s\n", entry.ssid, entry.bssid);
            } else if (strcasecmp(entry.type, "New") == 0) {
                snprintf(temp, sizeof(temp), "New Network detected: SSID=%s, BSSID=%s\n", entry.ssid, entry.bssid);
            }else if (strcasecmp(entry.type, "Unauthorized") == 0) {
                snprintf(temp, sizeof(temp), "Unauthorized channel use detected: SSID=%s, BSSID=%s Ch=%d\n", entry.ssid, entry.bssid, entry.channel);
            } else if (strcasecmp(entry.type, "Disappeared") == 0) {
                snprintf(temp, sizeof(temp), "Disappeared Network: SSID=%s, BSSID=%s\n", entry.ssid, entry.bssid);
            }else if (strcasecmp(entry.type, "SecChange") == 0) {
                snprintf(temp, sizeof(temp), "Security Change: SSID=%s, BSSID=%s\n", entry.ssid, entry.bssid);
            }

            // If temp is not empty, append to result
            if (temp[0] != '\0') {
                if (strlen(result) + strlen(temp) + 1 > buffer_size) {
                    buffer_size *= 2;
                    char *new_result = realloc(result, buffer_size);
                    if (!new_result) {
                        free(result);
                        fclose(file);
                        return strdup("Error: Memory reallocation failed.\n");
                    }
                    result = new_result;
                }
                strcat(result, temp);
            }
        } else {
            ESP_LOGW("CSV_ANALYSIS", "Failed to parse line: %s", line);
        }
    }

    fclose(file);

    if (strlen(result) == 0) {
        strcpy(result, "No threats or new networks detected.\n");
    }

    return result;
    free(result);
}

const char* get_network_status(void) {

	char *status_message = analyze_comparative_report(CSV_FILE_PATH);
	return status_message;
}

