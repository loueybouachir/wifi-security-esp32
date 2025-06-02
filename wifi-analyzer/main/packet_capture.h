/*
 * packet_capture.h
 *
 *  Created on: 27 mars 2025
 *      Author: HP
 */

#ifndef MAIN_PACKET_CAPTURE_H_
#define MAIN_PACKET_CAPTURE_H_

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    int channel;
    int rssi;
    char type[20];
    char mac_source[18];
    char mac_dest[18];
    char ssid[33];
    int length;
} packet_info_t;

void wifi_sniffer_init(void);

void wifi_sniffer_start(int channel);

void wifi_sniffer_stop(void);

void save_packets_to_csv(void);

#endif /* MAIN_PACKET_CAPTURE_H_ */
