/*
 * telegram_sender.c
 *
 *  Created on: 29 avr. 2025
 *      Author: HP
 */


#include "telegram_sender.h"
#include "esp_log.h"
#include "esp_http_client.h"
#include "esp_wifi.h"
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdbool.h>
#include "networks_status.h"

#define BOT_TOKEN       "7571401562:AAEZQXAJuXscRPtS-Q1ytoBvikIDcRRySq0"
#define CHAT_ID         "6533780593"
#define CSV_FILE_PATH   "/spiffs/comparative_report.csv"

static const char *TAG = "TelegramSender";

static const char *telegram_root_cert_pem = \
"-----BEGIN CERTIFICATE-----\n" \
"MIIDrzCCApegAwIBAgIQCDvgVpBCRrGhdWrJWZHHSjANBgkqhkiG9w0BAQUFADBh\n" \
"MQswCQYDVQQGEwJVUzEVMBMGA1UEChMMRGlnaUNlcnQgSW5jMRkwFwYDVQQLExB3\n" \
"d3cuZGlnaWNlcnQuY29tMSAwHgYDVQQDExdEaWdpQ2VydCBHbG9iYWwgUm9vdCBD\n" \
"QTAeFw0wNjExMTAwMDAwMDBaFw0zMTExMTAwMDAwMDBaMGExCzAJBgNVBAYTAlVT\n" \
"MRUwEwYDVQQKEwxEaWdpQ2VydCBJbmMxGTAXBgNVBAsTEHd3dy5kaWdpY2VydC5j\n" \
"b20xIDAeBgNVBAMTF0RpZ2lDZXJ0IEdsb2JhbCBSb290IENBMIIBIjANBgkqhkiG\n" \
"9w0BAQEFAAOCAQ8AMIIBCgKCAQEA4jvhEXLeqKTTo1eqUKKPC3eQyaKl7hLOllsB\n" \
"CSDMAZOnTjC3U/dDxGkAV53ijSLdhwZAAIEJzs4bg7/fzTtxRuLWZscFs3YnFo97\n" \
"nh6Vfe63SKMI2tavegw5BmV/Sl0fvBf4q77uKNd0f3p4mVmFaG5cIzJLv07A6Fpt\n" \
"43C/dxC//AH2hdmoRBBYMql1GNXRor5H4idq9Joz+EkIYIvUX7Q6hL+hqkpMfT7P\n" \
"T19sdl6gSzeRntwi5m3OFBqOasv+zbMUZBfHWymeMr/y7vrTC0LUq7dBMtoM1O/4\n" \
"gdW7jVg/tRvoSSiicNoxBN33shbyTApOB6jtSj1etX+jkMOvJwIDAQABo2MwYTAO\n" \
"BgNVHQ8BAf8EBAMCAYYwDwYDVR0TAQH/BAUwAwEB/zAdBgNVHQ4EFgQUA95QNVbR\n" \
"TLtm8KPiGxvDl7I90VUwHwYDVR0jBBgwFoAUA95QNVbRTLtm8KPiGxvDl7I90VUw\n" \
"DQYJKoZIhvcNAQEFBQADggEBAMucN6pIExIK+t1EnE9SsPTfrgT1eXkIoyQY/Esr\n" \
"hMAtudXH/vTBH1jLuG2cenTnmCmrEbXjcKChzUyImZOMkXDiqw8cvpOp/2PV5Adg\n" \
"06O/nVsJ8dWO41P0jmP6P6fbtGbfYmbW0W5BjfIttep3Sp+dWOIrWcBAI+0tKIJF\n" \
"PnlUkiaY4IBIqDfv8NZ5YBberOgOzW6sRBc4L0na4UU+Krk2U886UAb3LujEV0ls\n" \
"YSEY1QSteDwsOoBrp+uvFRTp2InBuThs4pFsiv9kuXclVzDAGySj4dzp30d8tbQk\n" \
"CAUw7C29C79Fv1C5qfPrmAESrciIxpg0X40KPMbp1ZWVbd4=\n" \
"-----END CERTIFICATE-----\n";

const char* get_status(void) {
    char *status_message = analyze_comparative_report(CSV_FILE_PATH);
    return status_message;
}

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

// Telegram message sender
esp_err_t send_telegram_message(void) {
    const char* message = get_status();

    if (strcmp(message, "No threats or new networks detected.\n") != 0) {
        char *encoded_message = url_encode(message);
        if (!encoded_message) {
            ESP_LOGE(TAG, "Failed to URL-encode message");
            return ESP_ERR_NO_MEM;
        }

        char url[1024];
        snprintf(url, sizeof(url),
                 "https://api.telegram.org/bot%s/sendMessage?chat_id=%s&text=%s",
                 BOT_TOKEN, CHAT_ID, encoded_message);

        free(encoded_message);

        // Configure HTTP client
        esp_http_client_config_t config = {
            .url = url,
            .method = HTTP_METHOD_GET,
            .cert_pem = telegram_root_cert_pem,
            .timeout_ms = 10000,
        };

        esp_http_client_handle_t client = esp_http_client_init(&config);
        esp_err_t err = esp_http_client_perform(client);

        if (err == ESP_OK) {
            ESP_LOGI(TAG, "Telegram message sent successfully!");
        } else {
            ESP_LOGE(TAG, "Failed to send Telegram message: %s", esp_err_to_name(err));
        }

        esp_http_client_cleanup(client);
        return err;
    }
    return ESP_OK;
}
