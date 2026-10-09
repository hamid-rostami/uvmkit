#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include "lvgl.h"
#include "uvm32_target.h"
#include "uvmkit_display_wire.h"
#include <malloc.h>

#define LCD_WIDTH           240
#define LCD_HEIGHT          135

static size_t draw_buf_sz = LCD_WIDTH * LCD_HEIGHT / 5 * 2; // / 10 * 2;

static void example_lvgl_flush_cb(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map)
{
  //esp_lcd_panel_handle_t panel_handle = lv_display_get_user_data(disp);
  /* int offsetx1 = area->x1; */
  /* int offsetx2 = area->x2; */
  /* int offsety1 = area->y1; */
  /* int offsety2 = area->y2; */
  // copy a buffer's content to a specific area of the display
  //lcd_draw_bitmap(panel_handle, offsetx1, offsety1, offsetx2+1, offsety2+1, px_map); TODO
  
  struct uvmkit_display_area_s wire_area = {
      .x1 = (uint32_t)area->x1,
      .y1 = (uint32_t)area->y1,
      .x2 = (uint32_t)area->x2,
      .y2 = (uint32_t)area->y2,
  };
  /* The host derives the RGB565 map size from the inclusive area. */
  render(&wire_area, px_map);

  lv_display_flush_ready(disp);
}

extern char __heap_start, __heap_end;
//extern char __bss_start, __bss_end;

int main(void) {
  lv_init();
  lv_display_t *display = lv_display_create(LCD_WIDTH, LCD_HEIGHT);

  //struct mallinfo mi = mallinfo();
  uint8_t *buf = malloc(draw_buf_sz);
  lv_display_set_buffers(display,
                         buf,
                         NULL,
                         draw_buf_sz,
                         LV_DISPLAY_RENDER_MODE_PARTIAL);

  lv_display_set_color_format(display, LV_COLOR_FORMAT_RGB565);

  lv_display_set_flush_cb(display, example_lvgl_flush_cb);

  //Create widgets
  lv_obj_t * label = lv_label_create(lv_screen_active());
  //lv_label_set_text(label, "Hello from RISC-V! Let's create a long text to see how it fits!");
  lv_label_set_text(label, "Hello from RISC-V!");
  lv_obj_set_pos(label, 0, 0);

  while(1) {
  //for(uint8_t i = 0; i < 5; ++i) {
    lv_timer_handler();
    //usleep(100000); TODO
    lv_tick_inc(100);
  }
  println("hi\n");

  return 0;
}
