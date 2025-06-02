# See the build system documentation in IDF programming guide
# for more information about component CMakeLists.txt files.

idf_component_register(
    SRCS "main.c" "storage.c" "wifi_scan.c" 
    "utils.c" "wifi_init.c" "packet_capture.c" "http_server.c" 
    "results_analyzer.c" "whatsapp_sender.c" "networks_status.c" 
    "buzzer.c" "telegram_sender.c" "led_manager.c" "screen_printer.c" 
    "i2c-lcd.c"  "app_status.c" "wifi_connect.c"  # list the source files of this component
    INCLUDE_DIRS        # optional, add here public include directories
    PRIV_INCLUDE_DIRS   # optional, add here private include directories
    REQUIRES driver           # optional, list the public requirements (component names)
    PRIV_REQUIRES esp_wifi spiffs nvs_flash esp_event esp_http_server esp_http_client    # optional, list the private requirements
    
)
