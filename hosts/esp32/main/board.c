#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include "sdmmc_cmd.h"
#include "esp_vfs_fat.h"
#include "board.h"

#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_panel_rgb.h"
#include "esp_lcd_panel_io.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "esp_err.h"
#include "esp_lcd_io_spi.h"

static const char *TAG = "S3-GEEK-BOARD";

static esp_lcd_panel_handle_t panel_handle;
static SemaphoreHandle_t lcd_color_trans_mtx = NULL;


bool lcd_on_color_trans_done_cb_t(esp_lcd_panel_io_handle_t panel_io,
                                  esp_lcd_panel_io_event_data_t *edata,
                                  void *user_ctx) {
  (void)panel_io;
  (void)edata;
  (void)user_ctx;
  xSemaphoreGive(lcd_color_trans_mtx);
  return true;
}

int32_t lcd_backlight(bool on) {
  // TODO: check if lcd initialized
  gpio_set_level(LCD_BK_LIGHT_GPIO, (on == true));
  return 0;
}

int32_t lcd_init(void) {
  lcd_color_trans_mtx = xSemaphoreCreateBinary();
  if (NULL == lcd_color_trans_mtx) {
    return (-1);
  }
  
  // Backlight GPIO setup
  gpio_config_t bk_gpio_config = {
    .pin_bit_mask = 1ULL << LCD_BK_LIGHT_GPIO,
    .mode = GPIO_MODE_OUTPUT,
  };
  ESP_ERROR_CHECK(gpio_config(&bk_gpio_config));
    

  // SPI bus config (host 1, dedicated DMA)
  spi_bus_config_t buscfg = {
    .mosi_io_num = LCD_MOSI_GPIO,
    .miso_io_num = -1,
    .sclk_io_num = LCD_SCLK_GPIO,
    .quadwp_io_num = -1,
    .quadhd_io_num = -1,
    .max_transfer_sz = LCD_WIDTH * 80 * sizeof(uint16_t),
  };
  ESP_ERROR_CHECK(spi_bus_initialize(LCD_SPI, &buscfg, SPI_DMA_CH_AUTO));

  // SPI panel IO config
  esp_lcd_panel_io_handle_t io_handle = NULL;
  esp_lcd_panel_io_spi_config_t io_config = {
    .dc_gpio_num = LCD_DC_GPIO,
    .cs_gpio_num = LCD_CS_GPIO,
    .pclk_hz = LCD_PIXEL_CLOCK_HZ,
    .lcd_cmd_bits = 8,
    .lcd_param_bits = 8,
    .spi_mode = 0,
    //.trans_queue_len = 10,
    .trans_queue_depth = 10,
    /* .flags = { */
    /*     .dc_low_on_data = 0, */
    /*     .octal_mode = 0, */
    /*     .lsb_first = 0, */
    /* } */
  };
  ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)SPI3_HOST, &io_config, &io_handle));

  // ST7789 panel config (240x135, RGB565)
  esp_lcd_panel_dev_config_t panel_config = {
    .reset_gpio_num = LCD_RST_GPIO,
    .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB,
    .bits_per_pixel = 16,
    .data_endian = LCD_RGB_DATA_ENDIAN_LITTLE,
  };
  ESP_ERROR_CHECK(esp_lcd_new_panel_st7789(io_handle, &panel_config, &panel_handle));

  /* // Panel init sequence */
  ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_handle));
  ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle));
  //ESP_ERROR_CHECK(esp_lcd_panel_mirror(panel_handle, true, false));
  ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel_handle, true));
  ESP_ERROR_CHECK(esp_lcd_panel_invert_color(panel_handle, true)); // ST7789 needs inversion

  /* // Set orientation (landscape) */
  esp_lcd_panel_mirror(panel_handle, true, false);
  esp_lcd_panel_swap_xy(panel_handle, true);
  esp_lcd_panel_set_gap(panel_handle, 40, 52); // ST7789 offset for 240x135

    
  
  const esp_lcd_panel_io_callbacks_t cbs = {
    .on_color_trans_done = lcd_on_color_trans_done_cb_t,
  };
  /* Register done callback */
  ESP_ERROR_CHECK(esp_lcd_panel_io_register_event_callbacks(io_handle,
                                                            &cbs,
                                                            panel_handle));

  xSemaphoreGive(lcd_color_trans_mtx);
  return 0;
}

int32_t lcd_update_rec(uint16_t x1, uint16_t y1, uint16_t width, uint16_t height,
                       uint8_t *data, size_t pixels_count) {
  if (NULL == panel_handle) {
    return (-1);
  }

  // TODO: sanity check pixels_count
  if(xSemaphoreTake(lcd_color_trans_mtx, portMAX_DELAY)) {
    esp_lcd_panel_draw_bitmap(panel_handle, x1, y1,
                              x1+width, y1+height, data);
    return 0;
  }

  return (-2);
}

esp_err_t sd_init(void) {
  esp_err_t ret;
  
  esp_vfs_fat_sdmmc_mount_config_t mount_config = {
    .format_if_mount_failed = false,
    .max_files = 5,
    .allocation_unit_size = 16 * 1024
  };

  sdmmc_card_t *card;
  const char mount_point[] = "/sda"; // TODO: param

  sdmmc_host_t host = SDSPI_HOST_DEFAULT();
  host.slot = SD_SPI;

  spi_bus_config_t bus_cfg = {
    .mosi_io_num = SD_SPI_MOSI_GPIO,
    .miso_io_num = SD_SPI_MISO_GPIO,
    .sclk_io_num = SD_SPI_SCLK_GPIO,
    .quadwp_io_num = -1,
    .quadhd_io_num = -1,
    .max_transfer_sz = 4000,
  };

  ret = spi_bus_initialize(host.slot, &bus_cfg, SDSPI_DEFAULT_DMA);
  if (ret != ESP_OK) {
    ESP_LOGE(TAG, "Failed to initialize bus.");
    return (-1);
  }

  sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
  slot_config.gpio_cs = SD_SPI_CS_GPIO;
  slot_config.host_id = host.slot;


  ESP_LOGI(TAG, "Mounting filesystem");
  ret = esp_vfs_fat_sdspi_mount(mount_point, &host, &slot_config, &mount_config, &card);

  if (ret != ESP_OK) {
    if (ret == ESP_FAIL) {
      ESP_LOGE(TAG, "Failed to mount filesystem. ");
    } else {
      ESP_LOGE(TAG, "Failed to initialize the card (%s) ", esp_err_to_name(ret));
    }
    return (-2);
  }
  ESP_LOGI(TAG, "Filesystem mounted");

  sdmmc_card_print_info(stdout, card);

  return ESP_OK;
}
