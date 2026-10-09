#pragma once

#include <stdint.h>

/* LVGL-style inclusive display area, encoded as four little-endian u32s. */
struct uvmkit_display_area_s {
  uint32_t x1;
  uint32_t y1;
  uint32_t x2;
  uint32_t y2;
};
