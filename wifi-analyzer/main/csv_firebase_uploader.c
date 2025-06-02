#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "esp_log.h"
#include "esp_http_client.h"
#include "csv_firebase_uploader.h"

#define FIREBASE_URL "https://esp32-auditwifi-default-rtdb.europe-west1.firebasedatabase.app/reports"
#define FIREBASE_AUTH "cIjanaDBTRPcwyz3MZWfehHMjzpERm3tRJHnxXtY"

static const char *TAG = "CSV_UPLOADER";

static const char *FIREBASE_ROOT_CA = \
		"-----BEGIN CERTIFICATE-----\n" \
		"MIIC/TCCAeWgAwIBAgIJAJsaKkBAKNIdMA0GCSqGSIb3DQEBBQUAMCAxHjAcBgNV\n" \
		"BAMMFTEwNjM1Mzk1ODM4NzAzNzIwOTYwNjAgFw0yNTA1MDIwMDA5NDlaGA85OTk5\n" \
		"MTIzMTIzNTk1OVowIDEeMBwGA1UEAwwVMTA2MzUzOTU4Mzg3MDM3MjA5NjA2MIIB\n" \
		"IjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAvOtKL5QfOmMCTDGvRJdtpG2o\n" \
		"+X44T5+y3LBWROgjB/FcazHwfUmN6A+JZCveP4m+t7djIVYEebwCAwhK0/0DYMoL\n" \
		"/XVaN5j51HoJePtzOYpQc1qkVhi9A/lLeq27P5gdJaJ++upeUXucoxqQfpRFZuD2\n" \
		"wm1VZ5LdM6rkM0e2qEIc9JvhvaIo5KYyQalTWMJ7XM1kQyhYEuVh3faOj15RS4tW\n" \
		"H/LFu+AuTCxGN/6D/YCPJAB5toamkgnIE7yis6aPqw134s+naZNxm+G/QVfNd57l\n" \
		"5EI2A382oLcOcA5BLwP5XoLNDXTG6JJS9DYlI0NMK5cTQOrAcmhBMlpZfExoEQID\n" \
		"AQABozgwNjAMBgNVHRMBAf8EAjAAMA4GA1UdDwEB/wQEAwIHgDAWBgNVHSUBAf8E\n" \
		"DDAKBggrBgEFBQcDAjANBgkqhkiG9w0BAQUFAAOCAQEAdaU/1GEkvfMoHwm3FpP0\noR8oOzwlUazW57uLQDwxvxFKwtWEmrOQ9AOnnN2pDiqnh/YrRfM8fo1+TDiVslQ5\n" \
		"nSUviQ59lIv8FV5FIiPl7Ml+FQUHMG+zG4gODdv33oBGTilssVyN5aBfEUe9HscP\nd2V0eUVNRhSnM50Cygo20pyb3gTM+m5RPF68noolRSim0+GBSGxomajnirGq8sTV\n" \
		"TSBI3QR6epsC1k2wzvPQ4ZoqV+5eH5au9d0UagPwmfpyK/n4ddJTP582NmQ31hpf\n" \
		"wIcGsv7g5M9LUcM2T9ZGmLW11i+aGZqoM9X2KGayO2Z77UuX6PBamWkPEz2DNeWN\neQ==\n" \
		"-----END CERTIFICATE-----\n";

// Function to send JSON to Firebase
static void send_to_firebase(int row_id, const char *json_payload) {
    char url[256];
    snprintf(url, sizeof(url), "%s/report_%lld/row_%d.json?auth=%s",
             FIREBASE_URL, (long long)time(NULL), row_id, FIREBASE_AUTH);

    // Configure HTTPS with certificate validation
    esp_http_client_config_t config = {
        .url = url,
        .method = HTTP_METHOD_PUT,
        .cert_pem = FIREBASE_ROOT_CA,
        .skip_cert_common_name_check = false
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);
    esp_http_client_set_header(client, "Content-Type", "application/json");
    esp_http_client_set_post_field(client, json_payload, strlen(json_payload));

    esp_err_t err = esp_http_client_perform(client);
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "Upload success: %s", url);
    } else {
        ESP_LOGE(TAG, "Upload failed: %s", esp_err_to_name(err));
    }

    esp_http_client_cleanup(client);
}

// Function to create JSON payload
static char* create_json_payload(const char *timestamp, const char *value1, const char *value2) {
    // Manually constructing JSON string
    size_t length = strlen(timestamp) + strlen(value1) + strlen(value2) + 50;
    char *json_payload = malloc(length);
    if (!json_payload) {
        ESP_LOGE(TAG, "Memory allocation failed for JSON payload");
        return NULL;
    }

    snprintf(json_payload, length,
             "{\"timestamp\":\"%s\", \"value1\":\"%s\", \"value2\":\"%s\"}",
             timestamp, value1, value2);

    return json_payload;
}

void parse_and_send_csv(const char *csv_path) {
    FILE *file = fopen(csv_path, "r");
    if (!file) {
        ESP_LOGE(TAG, "Cannot open CSV file: %s", csv_path);
        return;
    }

    char line[256];
    int row_id = 0;

    while (fgets(line, sizeof(line), file)) {
        line[strcspn(line, "\r\n")] = 0;

        char *field1 = strtok(line, ",");
        char *field2 = strtok(NULL, ",");
        char *field3 = strtok(NULL, ",");

        if (!field1 || !field2 || !field3) {
            ESP_LOGW(TAG, "Skipping malformed line: %s", line);
            continue;
        }

        // Create the JSON payload
        char *json_payload = create_json_payload(field1, field2, field3);
        if (json_payload) {
            // Send the JSON data to Firebase
            send_to_firebase(row_id++, json_payload);
            free(json_payload);
        }
    }

    fclose(file);
}
