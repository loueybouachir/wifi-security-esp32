/*
 * wifi_connect.h
 *
 *  Created on: 30 avr. 2025
 *      Author: HP
 */

#ifndef MAIN_WIFI_CONNECT_H_
#define MAIN_WIFI_CONNECT_H_

#include "esp_err.h"
#include "esp_netif.h"

esp_err_t wifi_connect_sta(const char* ssid, const char* password);

void cleanup_and_reconnect(void);

esp_err_t wifi_connect_static(const char* ssid, const char* password, esp_netif_ip_info_t ip_info);

#endif /* MAIN_WIFI_CONNECT_H_ */
