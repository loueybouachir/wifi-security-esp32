/*
 * screen_printer.c
 *
 *  Created on: 29 avr. 2025
 *      Author: HP
 */

#include "screen_printer.h"
#include "freertos/FreeRTOS.h"
#include "i2c-lcd.h"
#include "driver/i2c.h"
#include "freertos/task.h"
#include <string.h>
#include <stdint.h>

#define LCD_COLUMNS 16
#define LCD_CMD_SET_DDRAM_ADDR 0x80
#define LCD_LINE_WIDTH 16

#define I2C_MASTER_SCL_IO          GPIO_NUM_22
#define I2C_MASTER_SDA_IO          GPIO_NUM_21
#define I2C_MASTER_NUM             I2C_NUM_0
#define I2C_MASTER_FREQ_HZ         100000
#define I2C_MASTER_TX_BUF_DISABLE  0
#define I2C_MASTER_RX_BUF_DISABLE  0

void lcd_command(uint8_t cmd);

void lcd_command(uint8_t cmd) {
    lcd_send_cmd(cmd);
}

static esp_err_t i2c_master_init(void)
{
    int i2c_master_port = I2C_MASTER_NUM;

    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_MASTER_FREQ_HZ,
    };

    esp_err_t err = i2c_param_config(i2c_master_port, &conf);
    if (err != ESP_OK) {
        return err;
    }

    return i2c_driver_install(i2c_master_port, conf.mode, I2C_MASTER_RX_BUF_DISABLE, I2C_MASTER_TX_BUF_DISABLE, 0);
}
void clear_line(int line) {
    char empty_line[LCD_LINE_WIDTH + 1];
    memset(empty_line, ' ', LCD_LINE_WIDTH);
    empty_line[LCD_LINE_WIDTH] = '\0';

    if (line == 1) {
        lcd_put_cur(0, 0);
    } else if (line == 2) {
        lcd_put_cur(1, 0);
    } else {
        printf("Invalid line number: %d\n", line);
        return;
    }

    lcd_send_string(empty_line);
}
void clear_lcd(void){
	lcd_clear();
}
void init_lcd(void)
{
    i2c_master_init(); // Initialize I2C master
    lcd_init();
    vTaskDelay(pdMS_TO_TICKS(50));
    lcd_clear();
    vTaskDelay(pdMS_TO_TICKS(50));
}
void scroll_text(const char *str, uint8_t line) {
    if (str == NULL) return;

    size_t len = strlen(str);
    if (len == 0) return;

    char buffer[LCD_COLUMNS + 1];
    const int scroll_delay_ms = 400;
    const int max_duration_ms = 33500;
    int max_iterations = max_duration_ms / scroll_delay_ms;

    for (int i = 0; i < len && max_iterations > 0; i++, max_iterations--) {
        for (int j = 0; j < LCD_COLUMNS; j++) {
            buffer[j] = str[(i + j) % len];
        }
        buffer[LCD_COLUMNS] = '\0';

        if (line == 1) {
            lcd_put_cur(0, 0);
        } else if (line == 2) {
            lcd_put_cur(1, 0);
        } else {
            printf("Invalid line number: %d\n", line);
            return;
        }

        lcd_send_string(buffer);
        vTaskDelay(pdMS_TO_TICKS(scroll_delay_ms));
    }
}



void print_on_line_1(const char *text) {
    if (text == NULL) return;

    if (strlen(text) <= LCD_LINE_WIDTH) {
        clear_line(1);
        lcd_put_cur(0, 0);
        lcd_send_string(text);
    } else {
        scroll_text(text, 1);
    }
}

void print_on_line_2(const char *text) {
    if (text == NULL) return;

    if (strlen(text) <= LCD_LINE_WIDTH) {
        clear_line(2);
        lcd_put_cur(1, 0);
        lcd_send_string(text);
    } else {
        scroll_text(text, 2);
    }
}
