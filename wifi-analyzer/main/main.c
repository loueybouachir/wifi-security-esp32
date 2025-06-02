/*
 * main.c
 *
 *  Created on: 19 mars 2025
 *      Author: HP
 */

#include <stdio.h>
#include "esp_system.h"
#include "nvs_flash.h"
#include "wifi_scan.h"
#include "storage.h"
#include "wifi_init.h"
#include "packet_capture.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "results_analyzer.h"
#include "http_server.h"
#include "buzzer.h"
#include "led_manager.h"
#include "screen_printer.h"
#include "app_status.h"
#include "wifi_connect.h"
#include "whatsapp_sender.h"
#include "csv_firebase_uploader.h"

TaskHandle_t core0_task_handle = NULL;

void core0_task(void *pvParameters) {

			init_storage();
            for (int spot = 1; spot <= 3; spot++) {
                reset_wifi();
                initialize_wifi();
                wifi_scan();
                save_scan_results_csv();
                wifi_sniffer_init();

                for (int channel = 1; channel <= 14; channel++) {
                    printf("\n=== Scanning Channel %d ===\n", channel);
                    wifi_sniffer_start(channel);
                    vTaskDelay(200 / portTICK_PERIOD_MS);
                    wifi_sniffer_stop();
                    vTaskDelay(200 / portTICK_PERIOD_MS);
                }

                results_analyzer_run();
                if (spot < 3) {
                    vTaskDelay(60000 / portTICK_PERIOD_MS);
                } else {
                    vTaskDelay(2000 / portTICK_PERIOD_MS);
                }
            }

            final_analyzer_run();
            vTaskDelay(100000 / portTICK_PERIOD_MS);

}

void app_main() {
	// Hardware Testing
	init_lcd();
	clear_lcd();
	vTaskDelay(2000/ portTICK_PERIOD_MS);
	print_on_line_1("----BOOTING----");
	print_on_line_2("SETTING CONFIG");
	vTaskDelay(1000/ portTICK_PERIOD_MS);
	safe_led_test();
	buzzer_test();
	danger_led_test();

	 printf("Demarrage de l'application - initialisations uniques\n");

    // Principal initialization

    nvs_flash_init();
    init_storage();
    results_analyzer_init();
    vTaskDelay(1000/ portTICK_PERIOD_MS);


    xTaskCreatePinnedToCore(core0_task, "core0_task", 12288, NULL, 1, &core0_task_handle, 0);

    vTaskDelay(3000/ portTICK_PERIOD_MS);

    xTaskCreatePinnedToCore(task_line1, "Line1Task", 12288, NULL, 5, NULL, 1);
    xTaskCreatePinnedToCore(task_line2, "Line2Task", 12288, NULL, 5, NULL, 1);
    xTaskCreatePinnedToCore(task_led_and_buzzer, "LEDTask", 12288, NULL, 5, NULL, 1);
    xTaskCreatePinnedToCore(task_ap_connector, "ConnectionTask", 12288, NULL, 5, NULL, 1);

}
