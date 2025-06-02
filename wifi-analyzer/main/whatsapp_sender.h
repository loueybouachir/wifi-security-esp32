/*
 * whatsapp_sender.h
 *
 *  Created on: 27 avr. 2025
 *      Author: HP
 */

#ifndef MAIN_WHATSAPP_SENDER_H_
#define MAIN_WHATSAPP_SENDER_H_

#include "esp_err.h"

esp_err_t send_whatsapp_message(void);

const char* get_status(void);

char* url_encode(const char *str);

#endif /* MAIN_WHATSAPP_SENDER_H_ */
