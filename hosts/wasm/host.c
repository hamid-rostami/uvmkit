//#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <emscripten.h>

#include "uvmkit/common/uvmkit_display_wire.h"
#include "uvmkit/common/uvmkit_sys.h"
#include "uvmkit/common/uvmkit_wire.h"
#include "uvmkit/events.h"
#include "uvmkit/utils.h"


#define LCD_WIDTH 240
#define LCD_HEIGHT 135
#define LCD_SCALE 1

static uvm32_state_t *vmst = NULL;

EM_JS(void, js_display_update_rec, (int x, int y, int width, int height,
                                 uint8_t *pixels, uint16_t pixels_count), {
    // The VM supplies little-endian RGB565 (2 bytes per pixel), while
    // ImageData requires RGBA (4 bytes per pixel).
    const rgba = new Uint8ClampedArray(pixels_count * 4);
    for (let i = 0; i < pixels_count; ++i) {
        const rgb565 = HEAPU8[pixels + i * 2] |
                       (HEAPU8[pixels + i * 2 + 1] << 8);
        const r = (rgb565 >> 11) & 0x1f;
        const g = (rgb565 >> 5) & 0x3f;
        const b = rgb565 & 0x1f;

        // Expand 5/6-bit channels to 8 bits, matching the Linux renderer.
        rgba[i * 4] = (r * 255 + 15) / 31;
        rgba[i * 4 + 1] = (g * 255 + 31) / 63;
        rgba[i * 4 + 2] = (b * 255 + 15) / 31;
        rgba[i * 4 + 3] = 255;
    }

    const canvas = document.getElementById("screen");
    const ctx = canvas.getContext("2d");
    const imgData = new ImageData(rgba, width, height);
    ctx.putImageData(imgData, x, y);
});

EM_JS(void, js_get_mouse, (uint32_t *touchpad), {
    const mouse = globalThis.uvmkitMouse ?? {
        x: 0, y: 0, buttons: 0
    };

    const touchpadView = new Uint32Array(
        HEAPU32.buffer,
        touchpad,
        3
    );

    touchpadView.set([mouse.x, mouse.y, mouse.buttons]);
});

static bool display_frame(uvm32_state_t *vmst, uvm32_evt_t *evt) {
  /* Coordinates are signed values encoded in the shared u32 wire format. */
  uvm32_slice_t area =
      uvm32_arg_getslice_fixed(vmst, evt, ARG0,
                               sizeof(struct uvmkit_display_area_s));
  if (area.len != sizeof(struct uvmkit_display_area_s)) {
    uvm32_arg_setval(vmst, evt, RET, (uint32_t)-1);
    return false;
  }

  struct uvmkit_display_area_s* data = (struct uvmkit_display_area_s*) area.ptr;
  int32_t x1 = (int32_t)uvmkit_decode_u32(&data->x1);
  int32_t y1 = (int32_t)uvmkit_decode_u32(&data->y1);
  int32_t x2 = (int32_t)uvmkit_decode_u32(&data->x2);
  int32_t y2 = (int32_t)uvmkit_decode_u32(&data->y2);
  if (x1 < 0 || y1 < 0 || x2 < x1 || y2 < y1 || x2 >= LCD_WIDTH ||
      y2 >= LCD_HEIGHT) {
    fprintf(stderr, "render syscall: invalid area (%d, %d)-(%d, %d)\n", x1,
            y1, x2, y2);
    uvm32_arg_setval(vmst, evt, RET, (uint32_t)-1);
    return false;
  }

  int32_t width = x2 - x1 + 1;
  int32_t height = y2 - y1 + 1;
  size_t pixel_count = (size_t)width * (size_t)height;
  uvm32_slice_t pixels =
      uvm32_arg_getslice_fixed(vmst, evt, ARG1,
                               (uint32_t)(pixel_count * sizeof(uint16_t)));
  if (pixels.len != pixel_count * sizeof(uint16_t)) {
    fprintf(stderr, "render syscall: invalid RGB565 pixel map size\n");
    uvm32_arg_setval(vmst, evt, RET, (uint32_t)-2);
    return false;
  }

  printf("displaying frame...\n");
  js_display_update_rec(x1, y1,
                        width, height,
                        pixels.ptr, pixel_count);
  uvm32_arg_setval(vmst, evt, RET, 0);
  return true;
}

static void get_mouse(uvm32_state_t *vmst, uvm32_evt_t *evt) {
  uvm32_slice_t buf =
      uvm32_arg_getslice_fixed(vmst, evt, ARG0,
                               sizeof(struct uvmkit_touchpad_s));
  if (buf.len != sizeof(struct uvmkit_touchpad_s)) {
    uvm32_arg_setval(vmst, evt, RET, (uint32_t)-1);
    return;
  }

  struct uvmkit_touchpad_s *data =
      (struct uvmkit_touchpad_s *)buf.ptr;
  js_get_mouse(&data->x);
  uvm32_arg_setval(vmst, evt, RET, 0);
}

// Called from JS when a new file is loaded; C takes ownership of buf
EMSCRIPTEN_KEEPALIVE
int32_t load_image(uint8_t *buf, int len) {
  if (NULL != vmst) {
    free(vmst);
  }

  vmst = malloc(sizeof(uvm32_state_t));
  uvm32_init(vmst);
  
  printf("Loading binary with size %d\n", len);
  if(!uvm32_load(vmst, buf, len)) {
    return (-1);
  }
  printf("Starting VM...\n");
  return 0;
}

static void run(void) {
  if (NULL == vmst) {
    return;
  }

  enum { STEPS = 10000 };
  uvm32_evt_t evt;

  uint32_t ran_steps = uvm32_run(vmst, &evt, STEPS);

  switch (evt.typ) {
  default:
    printf("Unknown event %d", evt.typ);
    break;

  case UVM32_EVT_END:
    printf("vm ended\n");
    //running = false; TODO
    break;

  case UVM32_EVT_ERR:
    if ((UVM32_ERR_HUNG == evt.data.err.errcode) && (STEPS == ran_steps)) {
      // keep running
      uvm32_clearError(vmst);
    } else {
      printf("UVM32_EVT_ERR '%s' (%d)\n", evt.data.err.errstr,
             (int)evt.data.err.errcode);
      free(vmst);
      vmst = NULL;
      return;
    }
    break;

  case UVM32_EVT_SYSCALL: // vm has paused to handle UVM32_SYSCALL
    switch(evt.data.syscall.code) {
    case UVM32_SYSCALL_RENDER:
      (void)display_frame(vmst, &evt);
      break;

    case UVM32_SYSCALL_TOUCHPAD:
      (void)get_mouse(vmst, &evt);
      break;

    default:
      (void)uvmkit_event_handler(vmst, &evt);
      break;
    }
    break;
  } // switch
}

int main(void) {
    // fps = 0 means "use requestAnimationFrame" (sync with screen refresh)
    emscripten_set_main_loop(run, 0, 0);
    return 0;
}
