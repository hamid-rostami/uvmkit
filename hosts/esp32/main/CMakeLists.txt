idf_component_register(
  SRCS         "esp32.c" "board.c"
  INCLUDE_DIRS "."
  EMBED_FILES  "app.bin"
  REQUIRES     "uvmkit" "fatfs" "spi_flash"
               "esp_driver_gpio" "esp_lcd"
)
