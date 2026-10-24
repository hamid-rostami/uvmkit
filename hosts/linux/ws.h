#pragma once

#include <stdint.h>
#include <libwebsockets.h>

#define MAX_MESSAGE 4096

struct session {
  //unsigned char data[LWS_PRE + MAX_MESSAGE];
  //size_t len;
  //int pending;
  uint32_t last_frame_num;
};

struct ws_object {
  struct {
    uint8_t* frame_buf;
    uint16_t w;
    uint16_t h;
    
    struct {
      uint32_t frame_num;
      uint16_t x;
      uint16_t y;
      uint16_t w;
      uint16_t h;
    } last_refresh;
  } disp;
  
  struct lws_context *context;
};

enum {
  WS_TAG_NONE = 0,
  WS_TAG_FRAME,
  // ---
  WS_TAG_SIZE
};

struct ws_wire_msg_s {
  uint32_t tag;
  uint32_t len;
  uint8_t data[];
};

struct ws_wire_data_frame_s {
  uint16_t x;
  uint16_t y;
  uint16_t w;
  uint16_t h;
  uint8_t pixels[];
};
//#define ws_wire_data_frame ws_wire_data_frame_s

//typedef struct ws_object* ws_handle;

uintptr_t ws_server_init(uint16_t port, uint16_t disp_width,
                         uint16_t disp_height);
int32_t ws_service(uintptr_t handle);

// display functions
int32_t ws_display_update_rec(uintptr_t handle,
                              uint16_t x1, uint16_t y1,
                              uint16_t width, uint16_t height,
                              uint8_t *pixels, size_t pixels_count);
