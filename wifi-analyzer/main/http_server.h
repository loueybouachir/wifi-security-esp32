/*
 * htpp_server.h
 *
 *  Created on: 4 avr. 2025
 *      Author: HP
 */

#ifndef MAIN_HTTP_SERVER_H_
#define MAIN_HTTP_SERVER_H_

#include "esp_err.h"
#include "esp_http_server.h"

esp_err_t init_spiffs(void);

void start_webserver(void);

void stop_webserver(void);

esp_err_t send_chunk_with_check(httpd_req_t *req, const char *buf);

#endif /* MAIN_HTTP_SERVER_H_ */
