/*
 * http_server.c
 *
 *  Created on: 4 avr. 2025
 *      Author: HP
 */

#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include "esp_log.h"
#include "esp_spiffs.h"
#include "esp_http_server.h"
#include <errno.h>

#define MAX_LINE_LENGTH 512
#define MAX_NETWORKS 20

#define TAG "HTTP_SERVER"

//Declare categories tables
static char critical_networks[20][MAX_LINE_LENGTH];
static char warning_networks[20][MAX_LINE_LENGTH];
static char change_networks[20][MAX_LINE_LENGTH];
static char secure_networks[20][MAX_LINE_LENGTH];

// Declare server handler
static httpd_handle_t server = NULL;

// Forward declarations
void stop_webserver();
esp_err_t csv_handler(httpd_req_t *req);
esp_err_t send_chunk_with_check(httpd_req_t *req, const char *buf);
esp_err_t init_spiffs(void);

// Sending request chunks with checks
esp_err_t send_chunk_with_check(httpd_req_t *req, const char *buf) {
	vTaskDelay(100 / portTICK_PERIOD_MS);
    esp_err_t ret = httpd_resp_sendstr_chunk(req, buf);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to send chunk: %s", esp_err_to_name(ret));
        return ret;
    }
    vTaskDelay(200 / portTICK_PERIOD_MS);
    return ESP_OK;
}

// Initialize SPIFFS
esp_err_t init_spiffs() {
    ESP_LOGI(TAG, "Initializing SPIFFS");
    esp_vfs_spiffs_conf_t conf = {
        .base_path = "/spiffs",
        .partition_label = NULL,
        .max_files = 5,
        .format_if_mount_failed = false
    };
    esp_err_t ret = esp_vfs_spiffs_register(&conf);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize SPIFFS: %s", esp_err_to_name(ret));
        return ret;
    }

    // Verify file existence
    FILE *f = fopen("/spiffs/comparative_report.csv", "r");
    if (f == NULL) {
        ESP_LOGE(TAG, "CSV file not found or inaccessible: %s", strerror(errno));
        esp_vfs_spiffs_unregister(NULL);
        return ESP_FAIL;
    }
    fclose(f);
    ESP_LOGI(TAG, "SPIFFS initialized successfully");
    return ESP_OK;
}


// CSV Handler
esp_err_t csv_handler(httpd_req_t *req) {
    FILE *f = fopen("/spiffs/comparative_report.csv", "r");
    if (f == NULL) {
        ESP_LOGE("CSV_HANDLER", "Failed to open CSV file: %d", errno);
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to open CSV file");
        return ESP_FAIL;
    }

    char line[MAX_LINE_LENGTH];
    bool first_data_line = true;

    int critical_count = 0;
    int warning_count = 0;
    int change_count = 0;
    int secure_count = 0;

    while (fgets(line, sizeof(line), f)) {
        if (strstr(line, "Type") != NULL && first_data_line) {
            first_data_line = false;
            continue;
        }

        // Create a copy of the line before parsing
        char line_copy[MAX_LINE_LENGTH];
        strncpy(line_copy, line, MAX_LINE_LENGTH);
        line_copy[MAX_LINE_LENGTH - 1] = '\0';

        // Parse the line for categorization
        char *token = strtok(line, ",");
        char type[32] = "";
        char evtwin[8] = "";
        char deauth[8] = "";
        char security[32] = "";
        char cur_auth[32] = "";
        char ssid[64] = "";
        int col_num = 0;

        while (token != NULL && col_num <= 10) {
            token[strcspn(token, "\r\n")] = 0;
            if (col_num == 0) { // Type
                strcpy(type, token);
            } else if (col_num == 1) { // SSID
                strcpy(ssid, token);
            } else if (col_num == 7) { // Current Auth
                strcpy(cur_auth, token);
            } else if (col_num == 8) { // EvTwin
                strcpy(evtwin, token);
            } else if (col_num == 9) { // Deauth
                strcpy(deauth, token);
            } else if (col_num == 10) { // SecurityEvaluation
                strcpy(security, token);
            }

            token = strtok(NULL, ",");
            col_num++;
        }

        bool is_critical = false;
        bool is_warning = false;
        bool is_change = false;

        // CRITICAL THREATS
        if (strcmp(evtwin, "1") == 0 || strcmp(deauth, "1") == 0 || strcmp(type, "Unauthorized") == 0) {
            is_critical = true;
        }
        // SECURITY WARNINGS
        else if ((strcmp(ssid, "Hidden Network") == 0) ||
                 strcmp(cur_auth, "Open") == 0 ||
                 strcmp(cur_auth, "WEP") == 0 ||
                 (strcmp(security, "Correct") != 0 &&
                  strcmp(security, "Secured") != 0 &&
                  strcmp(security, "Very secured") != 0 &&
                  strcmp(security, "") != 0)) {
            is_warning = true;
        }
        // NETWORK CHANGES
        else if (strcmp(type, "SecChange") == 0 ||
                 strcmp(type, "New") == 0 ||
                 strcmp(type, "Disappeared") == 0 ||
                 strlen(ssid) == 0) {
            is_change = true;
        }

        if (is_critical) {
            strcpy(critical_networks[critical_count], line_copy);
            critical_count++;
        } else if (is_warning) {
            strcpy(warning_networks[warning_count], line_copy);
            warning_count++;
        } else if (is_change) {
            strcpy(change_networks[change_count], line_copy);
            change_count++;
        } else {
            strcpy(secure_networks[secure_count], line_copy);
            secure_count++;
        }
    }

    // Set HTML header to keep the server alive
    httpd_resp_set_hdr(req, "Connection", "keep-alive");
    httpd_resp_set_hdr(req, "Keep-Alive", "timeout=60, max=100");
    httpd_resp_set_type(req, "text/html");

    ESP_LOGI(TAG, "Sending Header");
    if (send_chunk_with_check(req,
            "<!DOCTYPE html><html><head><title>WiFi Security Analysis</title>"
            "<meta name='viewport' content='width=device-width, initial-scale=1'>"
            "<link rel='stylesheet' href='https://cdnjs.cloudflare.com/ajax/libs/font-awesome/6.0.0/css/all.min.css'>"
            "<style>"
            "* { box-sizing: border-box; }"
            "body { font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif; background:  #f0f2f5; margin: 0; padding: 20px; color: #333; min-height: 100vh; }"
            "h1 { text-align: center; color:#2c3e50; margin-bottom: 30px; text-shadow: 2px 2px 4px rgba(0,0,0,0.3); font-size: 2.5em; }"
            "h2 { color: #34495e; margin: 30px 0 20px 0; padding: 15px 20px; border-radius: 8px; font-size: 1.5em; display: flex; align-items: center; gap: 10px; }"
            "h2 i { font-size: 1.2em; }"
            ".container { max-width: 1200px; margin: 0 auto; }"
            ".card-container { display: flex; flex-wrap: wrap; gap: 20px; margin-bottom: 40px; }"
            ".card { background: white; border-radius: 12px; box-shadow: 0 4px 15px rgba(0,0,0,0.1); padding: 20px; width: 350px; transition: all 0.3s ease; border: 2px solid transparent; }"
            ".card:hover { transform: translateY(-8px); box-shadow: 0 8px 25px rgba(0,0,0,0.15); }"

            "/* Critical Threats - Red */\n"
            ".critical-section { color: linear-gradient(135deg, #ff6b6b, #ee5a24); animation: pulse-red 2s infinite; }"
            ".critical-card { border-left: 5px solid #c0392b; background: linear-gradient(135deg, #fff5f5, #ffe6e6); border-color: #e74c3c; }"
            ".critical-card:hover { border-color: #c0392b; box-shadow: 0 8px 25px rgba(231, 76, 60, 0.3); }"
            "@keyframes pulse-red { 0%, 100% { box-shadow: 0 0 0 0 rgba(231, 76, 60, 0.7); } 70% { box-shadow: 0 0 0 10px rgba(231, 76, 60, 0); } }"

            "/* Security Warnings - Orange */\n"
            ".warning-section { color: #f39c12; }"
            ".warning-card { border-left: 5px solid #e67e22; background: white; border-color: #f39c12; }"
            ".warning-card:hover { border-color: #e67e22; box-shadow: 0 8px 25px rgba(243, 156, 18, 0.3); }"

            "/* Network Changes - Yellow */\n"
            ".change-section {color:linear-gradient(135deg, #ffeb3b, #ffc107); }"
            ".change-card { border-left: 5px solid #f39c12; background: white; border-color: #f1c40f; }"
            ".change-card:hover { border-color: #f39c12; box-shadow: 0 8px 25px rgba(241, 196, 15, 0.3); }"

            "/* Secure Networks - Green */\n"
            ".secure-section { color: #2ecc71; }"
            ".secure-card { border-left: 5px solid #27ae60; background: white; border-color: #2ecc71; }"
            ".secure-card:hover { border-color: #27ae60; box-shadow: 0 8px 25px rgba(46, 204, 113, 0.3); }"

            ".card-title { font-size: 18px; font-weight: 600; margin: 0 0 15px 0; white-space: nowrap; overflow: hidden; text-overflow: ellipsis; }"
            ".card-content { display: flex; flex-direction: column; gap: 10px; }"
            ".card-row { display: flex; justify-content: space-between; align-items: center; }"
            ".card-label { font-weight: 500; color: #7f8c8d; font-size: 14px; }"
            ".card-value { font-weight: 500; font-size: 14px; }"
            ".bssid { font-family: 'Courier New', monospace; font-size: 13px; color: #34495e; background: #f8f9fa; padding: 2px 6px; border-radius: 4px; }"

            "/* Signal Strength Indicators */\n"
            ".rssi { display: inline-block; padding: 4px 8px; border-radius: 6px; font-weight: bold; font-size: 12px; }"
            ".rssi-strong { background-color: #d5f5e3; color: #27ae60; }"
            ".rssi-medium { background-color: #fef9e7; color: #f39c12; }"
            ".rssi-weak { background-color: #fadbd8; color: #e74c3c; }"

            ".auth { background: linear-gradient(135deg, #3498db, #2980b9); color: white; padding: 4px 8px; border-radius: 6px; font-size: 12px; font-weight: 500; }"
            ".security { font-weight: bold; padding: 4px 8px; border-radius: 6px; font-size: 12px; }"
            ".security-correct { background-color: #d5f5e3; color: #27ae60; }"
            ".security-warning { background-color: #fef9e7; color: #f39c12; }"
            ".security-danger { background-color: #fadbd8; color: #e74c3c; }"

            ".threat-count { color: white; padding: 4px 12px; border-radius: 20px; font-size: 14px; font-weight: bold; margin-left: 10px; }"
            ".no-networks { padding: 20px; background: rgba(255,255,255,0.9); border-radius: 8px; text-align: center; color: #7f8c8d; width: 100%; border: 2px dashed #bdc3c7; }"

            "/* Summary Dashboard */\n"
            ".summary { background: rgba(255,255,255,0.95); backdrop-filter: blur(10px); padding: 30px; border-radius: 15px; margin-bottom: 40px; box-shadow: 0 8px 32px rgba(0,0,0,0.1); }"
            ".summary-title { margin: 0 0 20px 0; color: #2c3e50; text-align: center; font-size: 1.8em; }"
            ".summary-stats { display: grid; grid-template-columns: repeat(auto-fit, minmax(200px, 1fr)); gap: 20px; margin-top: 20px; }"
            ".stat-item { text-align: center; padding: 20px; border-radius: 10px; }"
            ".stat-value { font-size: 2.5em; font-weight: bold; margin-bottom: 8px; }"
            ".stat-label { color: #6c757d; font-weight: 500; }"
            ".critical-value { color: #e74c3c; }"
            ".warning-value { color: #f39c12; }"
            ".change-value { color: #f1c40f; }"
            ".secure-value { color: #27ae60; }"
            ".critical-value_bg { background: #e74c3c; }"
            ".warning-value_bg { background: #f39c12; }"
            ".change-value_bg { background: #f1c40f; }"
            ".secure-value_bg { background: #27ae60; }"

            ".issues-list { margin-top: 10px; }"
            ".issue-item { display: flex; align-items: center; gap: 8px; margin: 5px 0; padding: 6px 10px; background: rgba(231, 76, 60, 0.1); border-radius: 6px; font-size: 13px; }"
            ".issue-item i { color: #e74c3c; }"

            ".footer { text-align: center; margin-top: 40px; color: #7f8c8d; font-size: 14px; }"

            "/* Responsive Design */\n"
            "@media (max-width: 768px) {"
            "  .card { width: 100%; }"
            "  .summary-stats { grid-template-columns: repeat(2, 1fr); gap: 15px; }"
            "  h1 { font-size: 2em; }"
            "}"
            "</style></head><body>"
            "<div class='container'>"
            "<h1><i class='fas fa-shield-alt'></i> WiFi Security Analysis Dashboard</h1>") != ESP_OK) {
        fclose(f);
        ESP_LOGE(TAG, "Failed to send header");
        return ESP_FAIL;
    }

    int total_networks = critical_count + warning_count + change_count + secure_count;

    // Add summary section
    ESP_LOGI(TAG, "Sending summary section");
    if (send_chunk_with_check(req,
            "<div class='summary'>"
            "<h3 class='summary-title'><i class='fas fa-chart-pie'></i> Security Overview</h3>"
            "<div class='summary-stats'>"
            "<div class='stat-item'>"
            "<div class='stat-value'>") != ESP_OK) {
        fclose(f);
        ESP_LOGE(TAG, "Failed to send summary section");
        return ESP_FAIL;
    }

    // Total networks
    char total_str[8];
    sprintf(total_str, "%d", total_networks);
    httpd_resp_sendstr_chunk(req, total_str);

    ESP_LOGI(TAG, "Sending toal count");
    if (send_chunk_with_check(req,
            "</div>"
            "<div class='stat-label'>Total Networks</div>"
            "</div>"
            "<div class='stat-item'>"
            "<div class='stat-value critical-value'>") != ESP_OK) {
        fclose(f);
        ESP_LOGE(TAG, "Failed to send total count");
        return ESP_FAIL;
    }

    // Critical networks count
    char critical_str[8];
    sprintf(critical_str, "%d", critical_count);
    vTaskDelay(100 / portTICK_PERIOD_MS);
    httpd_resp_sendstr_chunk(req, critical_str);

    ESP_LOGI(TAG, "Sending critical networks count ");
    if (send_chunk_with_check(req,
            "</div><div class='stat-label'>Critical Threats</div></div>"
            "<div class='stat-item'>"
            "<div class='stat-value warning-value'>") != ESP_OK) {
        fclose(f);
        ESP_LOGE(TAG, "Failed to send critical networks count");
        return ESP_FAIL;
    }

    // Warning networks count
    char warning_str[8];
    sprintf(warning_str, "%d", warning_count);
    vTaskDelay(100 / portTICK_PERIOD_MS);
    httpd_resp_sendstr_chunk(req, warning_str);

    ESP_LOGI(TAG, "Sending warning networks count ");
    if (send_chunk_with_check(req,
            "</div><div class='stat-label'>Security Warnings</div></div>"
            "<div class='stat-item'>"
            "<div class='stat-value change-value'>") != ESP_OK) {
        fclose(f);
        ESP_LOGE(TAG, "Failed to send warning networks count");
        return ESP_FAIL;
    }

    // Changed networks count
    char change_str[8];
    sprintf(change_str, "%d", change_count);
    httpd_resp_sendstr_chunk(req, change_str);
    vTaskDelay(100 / portTICK_PERIOD_MS);

    ESP_LOGI(TAG, "Sending changes section");
    if (send_chunk_with_check(req,
            "</div><div class='stat-label'>Network Changes</div></div>"
            "<div class='stat-item'>"
            "<div class='stat-value secure-value'>") != ESP_OK) {
        fclose(f);
        ESP_LOGE(TAG, "Failed to send changes section");
        return ESP_FAIL;
    }

    // Secure networks count
    char secure_str[8];
    sprintf(secure_str, "%d", secure_count);
    vTaskDelay(100 / portTICK_PERIOD_MS);
    httpd_resp_sendstr_chunk(req, secure_str);
    vTaskDelay(100 / portTICK_PERIOD_MS);

    ESP_LOGI(TAG, "Sending secure section");
    if (send_chunk_with_check(req,
            "</div><div class='stat-label'>Secure Networks</div></div>"
            "</div></div>") != ESP_OK) {
        fclose(f);
        ESP_LOGE(TAG, "Failed to send secure section");
        return ESP_FAIL;
    }

            void render_network_card(httpd_req_t *req, char *network_data, const char *card_class, bool show_issues) {
                char buffer[2048];
                int offset = 0;

                char *token;
                int col_num = 0;
                char type[32] = "";
                char ssid[64] = "";
                char bssid[32] = "";
                char channel[16] = "";
                char cur_rssi[16] = "";
                char avg_rssi[16] = "";
                char cur_auth[32] = "";
                char evtwin[8] = "";
                char deauth[8] = "";
                char security[32] = "";

                // Create a copy of network_data
                char network_copy[MAX_LINE_LENGTH];
                strncpy(network_copy, network_data, MAX_LINE_LENGTH);
                network_copy[MAX_LINE_LENGTH - 1] = '\0';

                char *saveptr = NULL;
                token = strtok_r(network_copy, ",", &saveptr);
                while (token != NULL) {
                    char clean_token[64];
                    snprintf(clean_token, sizeof(clean_token), "%s", token);
                    clean_token[strcspn(clean_token, "\r\n")] = 0;

                    //ESP_LOGI(TAG, "Parsing network data: %s, Column %d: %s", network_data, col_num, clean_token);

                    if (col_num == 0) strcpy(type, clean_token);
                    else if (col_num == 1) strcpy(ssid, clean_token);
                    else if (col_num == 2) strcpy(bssid, clean_token);
                    else if (col_num == 3) strcpy(channel, clean_token);
                    else if (col_num == 4) strcpy(cur_rssi, clean_token);
                    else if (col_num == 5) strcpy(avg_rssi, clean_token);
                    else if (col_num == 7) strcpy(cur_auth, clean_token);
                    else if (col_num == 8) strcpy(evtwin, clean_token);
                    else if (col_num == 9) strcpy(deauth, clean_token);
                    else if (col_num == 10) strcpy(security, clean_token);

                    token = strtok_r(NULL, ",", &saveptr);
                    col_num++;
                }

                //ESP_LOGI(TAG, "Parsed - SSID: %s, BSSID: %s, Channel: %s, AvgRSSI: %s, Auth: %s", ssid, bssid, channel, avg_rssi, cur_auth);

                // Build card HTML
                offset += snprintf(buffer + offset, sizeof(buffer) - offset, "<div class='card %s'>", card_class);
                offset += snprintf(buffer + offset, sizeof(buffer) - offset, "<h3 class='card-title'>%s</h3>",
                                   strlen(ssid) > 0 ? ssid : "<i class='fas fa-eye-slash'></i> Hidden Network");
                offset += snprintf(buffer + offset, sizeof(buffer) - offset,
                                   "<div class='card-content'>"
                                   "<div class='card-row'>"
                                   "<span class='card-label'><i class='fas fa-fingerprint'></i> BSSID:</span>"
                                   "<span class='card-value bssid'>%s</span></div>", bssid);
                offset += snprintf(buffer + offset, sizeof(buffer) - offset,
                                   "<div class='card-row'>"
                                   "<span class='card-label'><i class='fas fa-broadcast-tower'></i> Channel:</span>"
                                   "<span class='card-value'>%s</span></div>", channel);

                // RSSI
                const char* signal_class = "rssi-weak";
                int rssi_value = atoi(avg_rssi);
                if (rssi_value > -60) signal_class = "rssi-strong";
                else if (rssi_value > -70) signal_class = "rssi-medium";
                offset += snprintf(buffer + offset, sizeof(buffer) - offset,
                                   "<div class='card-row'>"
                                   "<span class='card-label'><i class='fas fa-signal'></i> Signal:</span>"
                                   "<span class='card-value'><span class='rssi %s'>%s dBm</span></span></div>",
                                   signal_class, avg_rssi);

                // Authentication
                offset += snprintf(buffer + offset, sizeof(buffer) - offset,
                                   "<div class='card-row'>"
                                   "<span class='card-label'><i class='fas fa-key'></i> Auth:</span>"
                                   "<span class='card-value'><span class='auth'>%s</span></span></div>", cur_auth);

                // Security
                const char* sec_class = strcmp(security, "Correct") == 0 || strcmp(security, "Secured") == 0 || strcmp(security, "Very secured") == 0 ? "security-correct" : (strlen(security) == 0 ? "security-warning" : "security-danger");
                offset += snprintf(buffer + offset, sizeof(buffer) - offset,
                                   "<div class='card-row'>"
                                   "<span class='card-label'><i class='fas fa-shield-alt'></i> Security:</span>"
                                   "<span class='card-value'><span class='security %s'>%s</span></span></div>",
                                   sec_class, strlen(security) > 0 ? security : "Unknown");

                // Issues
                if (show_issues) {
                    offset += snprintf(buffer + offset, sizeof(buffer) - offset, "<div class='issues-list'>");
                    if (strcmp(evtwin, "1") == 0)
                        offset += snprintf(buffer + offset, sizeof(buffer) - offset, "<div class='issue-item'><i class='fas fa-user-secret'></i> Evil Twin Attack</div>");
                    if (strcmp(deauth, "1") == 0)
                        offset += snprintf(buffer + offset, sizeof(buffer) - offset, "<div class='issue-item'><i class='fas fa-ban'></i> Deauth Attack</div>");
                    if (strcmp(type, "Unauthorized") == 0)
                        offset += snprintf(buffer + offset, sizeof(buffer) - offset, "<div class='issue-item'><i class='fas fa-exclamation-triangle'></i> Unauthorized Network</div>");
                    if (strcmp(type, "SecChange") == 0)
                        offset += snprintf(buffer + offset, sizeof(buffer) - offset, "<div class='issue-item'><i class='fas fa-exchange-alt'></i> Security Changed</div>");
                    if (strcmp(type, "New") == 0)
                        offset += snprintf(buffer + offset, sizeof(buffer) - offset, "<div class='issue-item'><i class='fas fa-plus-circle'></i> New Network</div>");
                    if (strcmp(type, "Disappeared") == 0)
                        offset += snprintf(buffer + offset, sizeof(buffer) - offset, "<div class='issue-item'><i class='fas fa-minus-circle'></i> Network Disappeared</div>");
                    if (strcmp(cur_auth, "Open") == 0)
                        offset += snprintf(buffer + offset, sizeof(buffer) - offset, "<div class='issue-item'><i class='fas fa-unlock'></i> Open Network</div>");
                    if (strlen(ssid) == 0)
                        offset += snprintf(buffer + offset, sizeof(buffer) - offset, "<div class='issue-item'><i class='fas fa-eye-slash'></i> Hidden SSID</div>");
                    offset += snprintf(buffer + offset, sizeof(buffer) - offset, "</div>");
                }

                offset += snprintf(buffer + offset, sizeof(buffer) - offset, "</div></div>");

                // Send the combined chunk
                ESP_LOGI(TAG, "Sending network card");
                if (send_chunk_with_check(req, buffer) != ESP_OK) {
                    ESP_LOGE(TAG, "Failed to send network card");
                }
            }

                // CRITICAL THREATS Section
                if (critical_count > 0) {
                    vTaskDelay(100 / portTICK_PERIOD_MS);
                    httpd_resp_sendstr_chunk(req, "<h2 class='critical-section critical-value'><i class='fas fa-skull-crossbones'></i> CRITICAL THREATS<span class='threat-count critical-value_bg'>");
                    vTaskDelay(100 / portTICK_PERIOD_MS);
                    httpd_resp_sendstr_chunk(req, critical_str);
                    vTaskDelay(100 / portTICK_PERIOD_MS);
                    httpd_resp_sendstr_chunk(req, "</span></h2>");
                    vTaskDelay(100 / portTICK_PERIOD_MS);
                    httpd_resp_sendstr_chunk(req, "<div class='card-container'>");
                    vTaskDelay(100 / portTICK_PERIOD_MS);

                    for (int i = 0; i < critical_count; i++) {
                        char network_copy[MAX_LINE_LENGTH];
                        strcpy(network_copy, critical_networks[i]);
                        vTaskDelay(100 / portTICK_PERIOD_MS);
                        render_network_card(req, network_copy, "critical-card", true);
                        vTaskDelay(100 / portTICK_PERIOD_MS);
                    }

                    httpd_resp_sendstr_chunk(req, "</div>");
                    vTaskDelay(100 / portTICK_PERIOD_MS);
                }

                // SECURITY WARNINGS Section
                if (warning_count > 0) {
                    vTaskDelay(100 / portTICK_PERIOD_MS);
                    httpd_resp_sendstr_chunk(req, "<h2 class='warning-section warning-value'><i class='fas fa-exclamation-triangle'></i> SECURITY WARNINGS<span class='threat-count warning-value_bg'>");
                    vTaskDelay(100 / portTICK_PERIOD_MS);
                    httpd_resp_sendstr_chunk(req, warning_str);
                    vTaskDelay(100 / portTICK_PERIOD_MS);
                    httpd_resp_sendstr_chunk(req, "</span></h2>");
                    vTaskDelay(100 / portTICK_PERIOD_MS);
                    httpd_resp_sendstr_chunk(req, "<div class='card-container'>");
                    vTaskDelay(100 / portTICK_PERIOD_MS);

                    for (int i = 0; i < warning_count; i++) {
                        char network_copy[MAX_LINE_LENGTH];
                        strcpy(network_copy, warning_networks[i]);
                        vTaskDelay(100 / portTICK_PERIOD_MS);
                        render_network_card(req, network_copy, "warning-card", true);
                        vTaskDelay(100 / portTICK_PERIOD_MS);
                    }

                    vTaskDelay(100 / portTICK_PERIOD_MS);
                    httpd_resp_sendstr_chunk(req, "</div>");
                    vTaskDelay(100 / portTICK_PERIOD_MS);
                }

                // NETWORK CHANGES Section
                if (change_count > 0) {
                    vTaskDelay(100 / portTICK_PERIOD_MS);
                    httpd_resp_sendstr_chunk(req, "<h2 class='change-section change-value'><i class='fas fa-exchange-alt'></i> NETWORK CHANGES<span class='threat-count change-value_bg'>");
                    vTaskDelay(100 / portTICK_PERIOD_MS);
                    httpd_resp_sendstr_chunk(req, change_str);
                    vTaskDelay(100 / portTICK_PERIOD_MS);
                    httpd_resp_sendstr_chunk(req, "</span></h2>");
                    vTaskDelay(100 / portTICK_PERIOD_MS);
                    httpd_resp_sendstr_chunk(req, "<div class='card-container'>");
                    vTaskDelay(100 / portTICK_PERIOD_MS);

                    for (int i = 0; i < change_count; i++) {
                        char network_copy[MAX_LINE_LENGTH];
                        strcpy(network_copy, change_networks[i]);
                        vTaskDelay(100 / portTICK_PERIOD_MS);
                        render_network_card(req, network_copy, "change-card", true);
                        vTaskDelay(100 / portTICK_PERIOD_MS);
                    }

                    vTaskDelay(100 / portTICK_PERIOD_MS);
                    httpd_resp_sendstr_chunk(req, "</div>");
                    vTaskDelay(100 / portTICK_PERIOD_MS);
                }

                // SECURE NETWORKS Section
                httpd_resp_sendstr_chunk(req, "<h2 class='secure-section secure-value'><i class='fas fa-shield-alt'></i> SECURE NETWORKS<span class='threat-count secure-value_bg'>");
                vTaskDelay(100 / portTICK_PERIOD_MS);
                httpd_resp_sendstr_chunk(req, secure_str);
                vTaskDelay(100 / portTICK_PERIOD_MS);
                httpd_resp_sendstr_chunk(req, "</span></h2>");
                vTaskDelay(100 / portTICK_PERIOD_MS);
                httpd_resp_sendstr_chunk(req, "<div class='card-container'>");
                vTaskDelay(100 / portTICK_PERIOD_MS);

                if (secure_count == 0) {
                    vTaskDelay(100 / portTICK_PERIOD_MS);
                    httpd_resp_sendstr_chunk(req, "<div class='no-networks'><i class='fas fa-info-circle'></i> No secure networks found</div>");
                    vTaskDelay(100 / portTICK_PERIOD_MS);
                } else {
                    for (int i = 0; i < secure_count; i++) {
                        char network_copy[MAX_LINE_LENGTH];
                        strcpy(network_copy, secure_networks[i]);
                        vTaskDelay(100 / portTICK_PERIOD_MS);
                        render_network_card(req, network_copy, "secure-card", false);
                        vTaskDelay(100 / portTICK_PERIOD_MS);
                    }
                }


                // Footer
                ESP_LOGI(TAG, "Sending footer");
                if (send_chunk_with_check(req,
                		"</div>"
                        "<div class='footer'>"
                        "<i class='fas fa-microchip'></i> WiFi Security Audit ESP32 | "
                        "Compiled: " __DATE__ " " __TIME__
                        "</div>"
                        "</div></body></html>") != ESP_OK) {
                    fclose(f);
                    ESP_LOGE(TAG, "Failed to send footer");
                }

                fclose(f);
                vTaskDelay(100 / portTICK_PERIOD_MS);
                httpd_resp_sendstr_chunk(req, NULL);
                vTaskDelay(100 / portTICK_PERIOD_MS);
                return ESP_OK;
}
void stop_webserver() {
    if (server) {
        httpd_stop(server);
        server = NULL;
        ESP_LOGI(TAG, "Web server stopped");
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }else{
    	ESP_LOGI(TAG, "No old web server found");
    }
}

void start_webserver() {

    // Ensure SPIFFS is initialized
	if (init_spiffs() != ESP_OK) {
	        ESP_LOGE(TAG, "Failed to initialize SPIFFS, halting");
	        return;
	}

	// Set HTTP server config
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.lru_purge_enable = true;
    config.recv_wait_timeout = 20;
    config.send_wait_timeout = 20;
    config.max_open_sockets = 4;
    config.stack_size = 8192;

    // Enable Keep-Alive
    config.keep_alive_enable = true;
    config.keep_alive_idle = 60;
    config.keep_alive_interval = 5;
    config.keep_alive_count = 3;

    httpd_uri_t csv_uri = {
        .uri = "/results",
        .method = HTTP_GET,
        .handler = csv_handler
    };

    if (httpd_start(&server, &config) == ESP_OK) {
        httpd_register_uri_handler(server, &csv_uri);
        ESP_LOGI(TAG, "Web server started at /results");
    } else {
        ESP_LOGE(TAG, "Failed to start web server");
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        if (httpd_start(&server, &config) == ESP_OK) {
            httpd_register_uri_handler(server, &csv_uri);
            ESP_LOGI(TAG, "Web server started on second attempt");
        }
    }
}
