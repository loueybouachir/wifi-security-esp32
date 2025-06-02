/*
 * whatsapp_sender.c
 *
 *  Created on: 27 avr. 2025
 *      Author: HP
 */


#include "whatsapp_sender.h"
#include "esp_log.h"
#include "esp_http_client.h"
#include "esp_wifi.h"
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdbool.h>
#include "networks_status.h"

#define PHONE_NUMBER    "+21658596697"
#define API_KEY         "4377416"
#define BOT_TOKEN       "7571401562:AAEZQXAJuXscRPtS-Q1ytoBvikIDcRRySq0"
#define CHAT_ID         "6533780593"
#define CSV_FILE_PATH   "/spiffs/comparative_report.csv"

static const char *TAG = "WhatsAppSender";


static const char *callmebot_root_cert_pem = \
		"-----BEGIN CERTIFICATE-----\n" \
		    "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\n" \
		    "-----END CERTIFICATE-----\n";

// URL encoder
char* url_encode(const char *str) {
    if (!str) return NULL;

    char *enc_str = malloc(strlen(str) * 3 + 1);
    if (!enc_str) return NULL;

    char *pstr = (char *)str;
    char *buf = enc_str;

    while (*pstr) {
        if (isalnum((unsigned char)*pstr) || *pstr == '-' || *pstr == '_' || *pstr == '.' || *pstr == '~') {
            *buf++ = *pstr;
        } else {
            buf += sprintf(buf, "%%%02X", (unsigned char)*pstr);
        }
        pstr++;
    }
    *buf = '\0';
    return enc_str;
}

// Send the WhatsApp message
esp_err_t send_whatsapp_message(void) {
    const char* message = get_network_status();
    if (strcmp(message, "No threats or new networks detected.\n") != 0) {
        char *encoded_message = url_encode(message);
        if (!encoded_message) {
            ESP_LOGE(TAG, "Failed to URL-encode message");
            return ESP_ERR_NO_MEM;
        }

        char url[1024];
        snprintf(url, sizeof(url),
                 "https://api.callmebot.com/whatsapp.php?phone=%s&text=%s&apikey=%s",
                 PHONE_NUMBER, encoded_message, API_KEY);
        free(encoded_message);

        esp_http_client_config_t config = {
            .url = url,
            .method = HTTP_METHOD_GET,
            .cert_pem = callmebot_root_cert_pem,
            .timeout_ms = 30000,
        };

        esp_err_t err = ESP_FAIL;
        int retry = 0;
        const int max_retries = 3;

        while (retry < max_retries) {
            esp_http_client_handle_t client = esp_http_client_init(&config);
            err = esp_http_client_perform(client);

            if (err == ESP_OK) {
                ESP_LOGI(TAG, "WhatsApp message sent successfully!");
                esp_http_client_cleanup(client);
                return ESP_OK;
            } else {
                ESP_LOGE(TAG, "Attempt %d: Failed to send WhatsApp message: %s", retry + 1, esp_err_to_name(err));
                int status_code = esp_http_client_get_status_code(client);
                ESP_LOGE(TAG, "HTTP Status Code: %d", status_code);
            }
            esp_http_client_cleanup(client);
            retry++;
            vTaskDelay(2000 / portTICK_PERIOD_MS); // Wait 2 seconds before retrying
        }
        return err;
    }
    return ESP_OK;
}
