/*
 * telegram_sender.h
 *
 *  Created on: 29 avr. 2025
 *      Author: HP
 */

#ifndef MAIN_TELEGRAM_SENDER_H_
#define MAIN_TELEGRAM_SENDER_H_

#include "esp_err.h"

esp_err_t send_telegram_message(void);

const char* get_status(void);

char* url_encode(const char *str);

#endif /* MAIN_TELEGRAM_SENDER_H_ */
