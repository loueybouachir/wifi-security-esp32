/*
 * results_analyzer.c
 *
 *  Created on: 21 avr. 2025
 *      Author: HP
 */

#include "results_analyzer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "esp_log.h"
#include "esp_spiffs.h"
#include "http_server.h"
#include "wifi_init.h"
#include "whatsapp_sender.h"
#include "buzzer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "wifi_connect.h"


static const char *TAG = "ANALYZER";
static const char *COUNTER_FILE = "/spiffs/counter.csv";
static const char *COMBINED_FILE = "/spiffs/combined_results.csv";


static WifiNetwork *networks = NULL;
static int network_count = 0;

// History arrays
static WifiNetwork history[MAX_CYCLES][MAX_NETWORKS];
static int history_count[MAX_CYCLES] = {0};

// Read/write cycle counter
int read_counter(void) {
    FILE *f = fopen(COUNTER_FILE, "r");
    int c = 0;
    if (f) { fscanf(f, "%d", &c); fclose(f); }
    return c;
}
static void write_counter(int c) {
    FILE *f = fopen(COUNTER_FILE, "w");
    if (f) { fprintf(f, "%d", c); fclose(f); }
}


static const char* evaluate_auth(int mode) {
    switch(mode) {
        case 0: return "Open";
        case 1: return "WEP";
        case 2: return "WPA_PSK";
        case 3: return "WPA2_PSK";
        case 4: return "WPA_WPA2_PSK";
        case 5: return "WPA2_ENTERPRISE";
        case 6: return "WPA3_PSK";
        case 7: return "WPA2_WPA3_PSK";
        default: return "Unknown";
    }
}
static const char* evaluate_security(int mode) {
    switch(mode) {
        case 0: return "Not secured";
        case 1: return "Very vulnerable";
        case 2: return "Vulnerable";
        case 3: return "Correct";
        case 4: return "Correct";
        case 5: return "Secured";
        case 6: return "Very secured";
        case 7: return "Very secured";
        default: return "Unknown";
    }
}

static void read_combined_csv(void) {
    FILE *f = fopen(COMBINED_FILE, "r");
    if (!f) { ESP_LOGE(TAG, "Cannot open %s", COMBINED_FILE); network_count = 0; return; }
    free(networks);
    networks = calloc(MAX_NETWORKS, sizeof(*networks));
    char line[256];
    fgets(line, sizeof(line), f);
    network_count = 0;
    while (fgets(line, sizeof(line), f) && network_count < MAX_NETWORKS) {
        WifiNetwork *n = &networks[network_count];
        memset(n, 0, sizeof(*n));
        if (sscanf(line, "%32[^,],%17[^,],%d,%d,%d,%d",
                   n->ssid, n->bssid, &n->rssi,
                   &n->authmode, &n->channel, &n->freq_offset) >= 5) {
            n->is_mesh = false;
            n->potential_evil_twin = false;
            n->deauth_attack = false;
            n->authorized = (n->channel <= 11) ? 1 : 0;
            network_count++;
        }
    }
    fclose(f);
}

// Mark deauth attacks
static void detect_deauth_packets(void) {
    FILE *f = fopen(COMBINED_FILE, "r");
    if (!f) return;
    char line[256];
    fgets(line, sizeof(line), f);
    while (fgets(line, sizeof(line), f)) {
        char *dup = strdup(line);
        char *tok = strtok(dup, ","); // channel
        tok = strtok(NULL, ",");     // rssi
        tok = strtok(NULL, ",");     // type
        if (tok && strcmp(tok, "Deauthentication") == 0) {
            char *b = strtok(NULL, ",");
            if (b) {
                for (int i = 0; i < network_count; i++) {
                    if (!strcmp(networks[i].bssid, b)) {
                        networks[i].deauth_attack = true;
                        break;
                    }
                }
            }
        }
        free(dup);
    }
    fclose(f);
}

// Evil twin detection
static void detect_evil_twins(void) {
    for (int i = 0; i < network_count; i++) {
        for (int j = i+1; j < network_count; j++) {
            if (strcmp(networks[i].ssid, networks[j].ssid)==0 &&
                strcmp(networks[i].bssid, networks[j].bssid)!=0) {
                bool auth_diff = networks[i].authmode != networks[j].authmode;
                bool chan_diff = abs(networks[i].channel - networks[j].channel) > 3;
                bool rssi_diff = abs(networks[i].rssi - networks[j].rssi) > 10;
                if (auth_diff || chan_diff || rssi_diff) {
                    networks[i].potential_evil_twin = true;
                    networks[j].potential_evil_twin = true;
                }
            }
        }
    }
}

// Mesh detection
static void detect_mesh_networks(void) {
    for (int i = 0; i < network_count; i++) {
        if (networks[i].potential_evil_twin) continue;
        for (int j = i+1; j < network_count; j++) {
            if (!networks[j].potential_evil_twin &&
                strcmp(networks[i].ssid, networks[j].ssid)==0 &&
                abs(networks[i].channel - networks[j].channel) <= 3) {
                networks[i].is_mesh = true;
                networks[j].is_mesh = true;
            }
        }
    }
}

// RSSI averages
static void calculate_rssi_averages(void) {
    static NetworkTracking track[MAX_NETWORKS];
    static int track_count = 0;
    for (int i = 0; i < network_count; i++) {
        bool found = false;
        for (int j = 0; j < track_count; j++) {
            if (!strcmp(networks[i].bssid, track[j].bssid)) {
                track[j].rssi_avg =
                  (track[j].rssi_avg * track[j].detection_count + networks[i].rssi)
                  / (track[j].detection_count + 1);
                track[j].detection_count++;
                networks[i].rssi_avg = track[j].rssi_avg;
                found = true;
                break;
            }
        }
        if (!found && track_count < MAX_NETWORKS) {
            strcpy(track[track_count].bssid, networks[i].bssid);
            track[track_count].rssi_avg = networks[i].rssi;
            track[track_count].detection_count = 1;
            networks[i].rssi_avg = networks[i].rssi;
            track_count++;
        }
    }
}

// Save detailed CSV
static void save_detailed_report(int slot) {
    char path[64];
    snprintf(path, sizeof(path), "/spiffs/scan_report%d.csv", slot);
    FILE *f = fopen(path, "w");
    if (!f) return;
    fprintf(f,"SSID,BSSID,Channel,RSSI,RSSI_Avg,Auth,Security,Mesh,EvTwin,Deauth,Authorized\n");
    for (int i = 0; i < network_count; i++) {
        WifiNetwork *n = &networks[i];
        fprintf(f,"%s,%s,%d,%d,%.1f,%s,%s,%d,%d,%d,%d\n",
                n->ssid, n->bssid, n->channel, n->rssi,
                n->rssi_avg,
                evaluate_auth(n->authmode),
                evaluate_security(n->authmode),
                n->is_mesh, n->potential_evil_twin, n->deauth_attack ,n->authorized);
    }
    fclose(f);
}

void do_simple_analysis(int cycle) {
    printf("\n--- Simple Analysis #%d (%d APs) ---\n", cycle+1, network_count);
    detect_evil_twins();
    detect_mesh_networks();
    calculate_rssi_averages();
    detect_deauth_packets();
    save_detailed_report(cycle % MAX_CYCLES);
    int slot = cycle % MAX_CYCLES;
    memcpy(history[slot], networks, sizeof(WifiNetwork)*network_count);
    history_count[slot] = network_count;
}

// Comparative CSV
void do_comparative_analysis(int cycle) {
    FILE *f = fopen("/spiffs/comparative_report.csv","w");
    if (!f) return;
    fprintf(f,"Type,SSID,BSSID,Channel,CurRSSI,AvgRSSI,PrevAuth,CurAuth,EvTwin,Deauth,SecurityEvaluation,Authorized\n");

    int current = (read_counter()-1) % MAX_CYCLES;
    char seen[MAX_CYCLES*MAX_NETWORKS][18]; int seen_cnt = 0;

    for (int prev=0; prev<MAX_CYCLES; prev++) {
        if (prev==current || history_count[prev]==0) continue;
        // new/changed
        for (int i=0; i<history_count[current]; i++) {
            WifiNetwork *cur = &history[current][i];
            bool skip = false;
            for (int s=0; s<seen_cnt; s++) if (!strcmp(seen[s],cur->bssid)) { skip=true; break; }
            if (skip) continue;
            strcpy(seen[seen_cnt++], cur->bssid);
            bool found=false;
            for (int j=0; j<history_count[prev]; j++) {
                WifiNetwork *old = &history[prev][j];
                if (!strcmp(cur->bssid, old->bssid)) {
                    found=true;
                    const char *type="Unchanged";
                    if (cur->authorized == 0) type="Unauthorized";
                    else if (cur->potential_evil_twin && !old->potential_evil_twin) type="New Evil Twin";
                    else if (!cur->potential_evil_twin && old->potential_evil_twin) type="Resolved Evil Twin";
                    else if (cur->authmode!=old->authmode) type="SecChange";
                    else if (cur->deauth_attack && !old->deauth_attack) type="DeauthAttack";
                    fprintf(f,"%s,%s,%s,%d,%d,%.1f,%s,%s,%d,%d,%s,%d\n",
                            type, cur->ssid, cur->bssid, cur->channel,
                            cur->rssi, cur->rssi_avg,
                            evaluate_auth(old->authmode),
                            evaluate_auth(cur->authmode),
                            cur->potential_evil_twin,
                            cur->deauth_attack,
                            evaluate_security(cur->authmode),
							cur->authorized);
                    break;
                }
            }
            if (!found) {
                const char *type = cur->authorized == 0 ? "Unauthorized" : "New";
                fprintf(f,"%s,%s,%s,%d,%d,%.1f,-,%s,%d,%d,%s,%d\n",
                        type, cur->ssid, cur->bssid, cur->channel, cur->rssi, cur->rssi_avg,
                        evaluate_auth(cur->authmode), cur->potential_evil_twin, cur->deauth_attack, evaluate_security(cur->authmode),
                        cur->authorized);
            }
        }
        // disappeared
        for (int j=0; j<history_count[prev]; j++) {
            WifiNetwork *old = &history[prev][j];
            bool still=false;
            for (int i=0; i<history_count[current]; i++) if (!strcmp(old->bssid,history[current][i].bssid)) { still=true; break; }
            if (!still) {
                const char *type = old->authorized == 0 ? "Unauthorized" : "Disappeared";
                fprintf(f,"%s,%s,%s,%d,%d,%.1f,%s,-,%d,%d,%s,%d\n",
                        type, old->ssid, old->bssid, old->channel, old->rssi, old->rssi_avg,
                        evaluate_auth(old->authmode), old->potential_evil_twin, old->deauth_attack, evaluate_security(old->authmode),
                        old->authorized);
            }
        }
    }
    fclose(f);
}

// Initialize SPIFFS
void results_analyzer_init(void) {
    esp_vfs_spiffs_conf_t conf={"/spiffs","storage",5,true};
    esp_vfs_spiffs_unregister(conf.partition_label);
    if (esp_vfs_spiffs_register(&conf)!=ESP_OK) {
        ESP_LOGE(TAG,"SPIFFS mount failed"); return;
    }
}

void read_simple_analyse_results_from_spiffs(int cycle) {
    char filename[64];
    snprintf(filename, sizeof(filename), "/spiffs/scan_report%d.csv", cycle % MAX_CYCLES);
    FILE *f = fopen(filename, "r");
    if (!f) {
        printf("No report\n");
        return;
    }
    char buf[128];
    while (fgets(buf, sizeof(buf), f)) {
        printf("%s", buf);
    }
    fclose(f);
}
void read_analyse_results_from_spiffs(void) {
    FILE *f=fopen("/spiffs/comparative_report.csv","r");
    if (!f) { printf("No report\n"); return; }
    char buf[128];
    printf("\n--- Comparative Report ---\n");
    while(fgets(buf,sizeof(buf),f)) printf("%s",buf);
    fclose(f);
}
void print_coombined_results_from_spiffs(void) {
    FILE *f=fopen("/spiffs/combined_results.csv","r");
    if (!f) { printf("No Result\n"); return; }
    char buf[128];
    printf("\n--- Combined Results ---\n");
    while(fgets(buf,sizeof(buf),f)) printf("%s",buf);
    fclose(f);
}
void results_analyzer_run(void) {
    int cycle=read_counter();
    read_combined_csv();
    if (!network_count) { ESP_LOGE(TAG,"No networks"); return; }
    do_simple_analysis(cycle);
    read_simple_analyse_results_from_spiffs(cycle);
    //print_coombined_results_from_spiffs();
    write_counter(cycle+1);

}
void final_analyzer_run(void){
	int cycle=read_counter();
    if ((cycle)%MAX_CYCLES==0) {
        do_comparative_analysis(cycle+1);
        read_analyse_results_from_spiffs();

    }else{
    	printf("NO ANALYSE");
    }
}
