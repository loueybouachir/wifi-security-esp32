/*
 * app_status.c
 *
 *  Created on: 30 avr. 2025
 *      Author: HP
 */
#include "app_status.h"
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "wifi_init.h"
#include "results_analyzer.h"
#include "screen_printer.h"
#include "networks_status.h"
#include "storage.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "led_manager.h"
#include "buzzer.h"
#include "whatsapp_sender.h"
#include <stdatomic.h>
#include "esp_log.h"
#include "wifi_connect.h"
#include "http_server.h"

static const char *TAG = "STATUS";
atomic_bool is_danger = false;
atomic_bool is_connected = false;

void task_line1(void *param) {
    while (1) {
        int counter = read_counter();
        if (counter < 3) {
        	char buffer[32];
        	snprintf(buffer, sizeof(buffer), "SCANNING #%d", counter+1);
            print_on_line_1(buffer);
        } else {
        	vTaskDelay(pdMS_TO_TICKS(4000));
            char *ip = get_ip_address();
            if (ip == NULL || strcmp(ip, "0.0.0.0") == 0 || strlen(ip) == 0) {
                print_on_line_1("CONNECTING ...");
            } else {
            	char url[64];
            	snprintf(url, sizeof(url), "http://%s/results ", ip);
            	vTaskDelay(pdMS_TO_TICKS(4000));
            	while(1){
            		print_on_line_1(url);
            	}

            }
        }
        vTaskDelay(pdMS_TO_TICKS(3000));
    }
}

void task_line2(void *param) {
    while (1) {
        int counter = read_counter();
        if (counter < 3) {
            char *results = get_simple_analyse_results(counter - 1);
            if (results != NULL) {
                print_on_line_2(results);
            } else {
                print_on_line_2("WAITING RESULTS");
            }
        } else {
        	vTaskDelay(pdMS_TO_TICKS(5000));
            const char *message = get_network_status();
            if (message != NULL) {
            		print_on_line_2(message);
                	vTaskDelay(pdMS_TO_TICKS(80000));
            } else {
            	while(0){
            		print_on_line_2("NO RESULTS");
            		break;
            	}
            }
        }
        vTaskDelay(pdMS_TO_TICKS(3000));  // Update every 3s
    }
}

void task_led_and_buzzer(void *param) {
	bool is_first_try = true;
    while (1) {
    	int counter = read_counter();
    	if (counter < 3 ){
            safe_led_on();
            danger_led_off();
    	}else {
    		if (is_first_try){
    			vTaskDelay(pdMS_TO_TICKS(5000));
    			is_first_try = false;
    		}
    		const char *message = get_network_status();
    		        if (message != NULL && strcmp(message, "No threats or new networks detected.\n") != 0 && strcmp(message, "") != 0 && strncmp(message, "Error", 5) != 0) {
    		        	atomic_store(&is_danger, true);
    		        	safe_led_off();
    		        	while(1){
    		        		danger_led_on();
    		        		buzzer_beep();
    		        	}
    		        }else{
    		        	while(0){
    		        		safe_led_on();
    		        	}
    		        }
    	}
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}

void task_ap_connector(void *param){
	while(1){
		int counter = read_counter();
		if (counter > 2 ){
			vTaskDelay(pdMS_TO_TICKS(5000));
			esp_netif_ip_info_t ip_info = {
			            .ip =       { .addr = ESP_IP4TOADDR(192,168,1,50) },
			            .gw =       { .addr = ESP_IP4TOADDR(192,168,1,1)  },
			            .netmask =  { .addr = ESP_IP4TOADDR(255,255,255,0) }
			        };

			        if (wifi_connect_static("your-SSID", "your-PASSWORD", ip_info) == ESP_OK) { // Set your local network SSID and PASSWORD
			            ESP_LOGI(TAG, "Connected with static IP");
			        } else {
			            ESP_LOGE(TAG, "Failed static IP connect");
			        }
			vTaskDelay(5000 / portTICK_PERIOD_MS);
			send_whatsapp_message();
            start_webserver();
            vTaskDelay(60000 / portTICK_PERIOD_MS);
            stop_webserver();
            printf("Loop Ended!\n");
            vTaskDelay(5000 / portTICK_PERIOD_MS);
            esp_restart();

		}
		vTaskDelay(pdMS_TO_TICKS(3000));
	}

}



