#pragma once

//#include "esp_lcd_panel_ops.h"
//#include "esp_lcd_panel_vendor.h"
//#include "esp_lcd_panel_rgb.h"
//#include "esp_lcd_panel_io.h"
//#include "driver/spi_master.h"
//#include "driver/gpio.h"
//#include "esp_err.h"
//#include "esp_lcd_io_spi.h"

// ESP32-S3-GEEK specific pins (from Waveshare wiki)
#define LCD_PIXEL_CLOCK_HZ  (2 * 1000 * 1000) // 2MHz
#define LCD_BK_LIGHT_GPIO   7
#define LCD_MOSI_GPIO       11
#define LCD_SCLK_GPIO       12
#define LCD_CS_GPIO         10
#define LCD_DC_GPIO         8
#define LCD_RST_GPIO        9
#define LCD_WIDTH           240
#define LCD_HEIGHT          135
#define LCD_SPI             SPI3_HOST

#define SD_SPI_MOSI_GPIO    35
#define SD_SPI_MISO_GPIO    37
#define SD_SPI_SCLK_GPIO    36
#define SD_SPI_CS_GPIO      34
#define SD_SPI              SPI2_HOST

int32_t lcd_init(void);
int32_t lcd_update_rec(uint16_t x1, uint16_t y1, uint16_t width, uint16_t height,
                       uint8_t *data, size_t pixels_count);
int32_t lcd_backlight(bool on);

esp_err_t sd_init(void);
