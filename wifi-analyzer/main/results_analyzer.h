/*
 * results_analyzer.h
 *
 *  Created on: 21 avr. 2025
 *      Author: HP
 */

#ifndef MAIN_RESULTS_ANALYZER_H_
#define MAIN_RESULTS_ANALYZER_H_


#include <stdbool.h>

#define MAX_NETWORKS 20
#define MAX_CYCLES 3

typedef struct {
    char ssid[33];
    char bssid[18];
    int rssi;
    int authmode;
    int channel;
    int freq_offset;
    float rssi_avg;
    bool is_mesh;
    bool potential_evil_twin;
    bool deauth_attack;
    bool authorized;
} WifiNetwork;

typedef struct {
    char bssid[18];
    float rssi_avg;
    int detection_count;
    bool is_mesh;
    bool potential_evil_twin;
} NetworkTracking;

int read_counter(void);

void results_analyzer_init();

void results_analyzer_run();

void final_analyzer_run();

#endif /* MAIN_RESULTS_ANALYZER_H_ */
