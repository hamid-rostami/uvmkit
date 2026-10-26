#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ws.h"

static int callback_http(struct lws *wsi,
                         enum lws_callback_reasons reason,
                         void *user, void *in, size_t len);

static int callback_ws(struct lws *wsi,
                       enum lws_callback_reasons reason,
                       void *user, void *in, size_t len);

static const struct lws_protocols protocols[] = {
  {
    .name = "http",
    .callback = callback_http,
    .per_session_data_size = 0,
    .rx_buffer_size = 0,
  },
  {
    .name = "ws-echo",
    .callback = callback_ws,
    .per_session_data_size = sizeof(struct session),
    .rx_buffer_size = 1024,
  },
  LWS_PROTOCOL_LIST_TERM
};


static const struct lws_http_mount mount = {
  .mountpoint = "/",
  .origin = "./",
  .def = "index.html",
  .origin_protocol = LWSMPRO_FILE,
  .mountpoint_len = 1,
};

static int callback_http(struct lws *wsi,
                         enum lws_callback_reasons reason,
                         void *user, void *in, size_t len)
{
  (void)wsi;
  (void)user;
  (void)in;
  (void)len;

  /* HTTP files are served by the configured mount. */
  return lws_callback_http_dummy(wsi, reason, user, in, len);
}

static int callback_ws(struct lws *wsi,
                       enum lws_callback_reasons reason,
                       void *user, void *in, size_t len)
{
  struct session *s = user;
  struct ws_object *hdl = (struct ws_object *)lws_context_user(lws_get_context(wsi));

  (void)in;
  (void)len;

  switch (reason) {
  case LWS_CALLBACK_ESTABLISHED:
    printf("WebSocket connected\n");
    if (s) {
      s->last_frame_num = 0;
    }
    lws_callback_on_writable(wsi);
    break;

  case LWS_CALLBACK_RECEIVE: {
    if (!hdl || !in || len < sizeof(struct ws_wire_msg_s)) {
      break;
    }


    struct ws_wire_msg_s *msg = (struct ws_wire_msg_s *)in;
    if (msg->tag == WS_TAG_MOUSE &&
        msg->len >= sizeof(struct ws_wire_data_mouse_s) &&
        len >= sizeof(struct ws_wire_msg_s) + sizeof(struct ws_wire_data_mouse_s)) {
      struct ws_wire_data_mouse_s *mouse = (struct ws_wire_data_mouse_s *)msg->data;
      hdl->mouse.x = mouse->x;
      hdl->mouse.y = mouse->y;
      hdl->mouse.btn = mouse->btn;
      //printf("Mouse data updated\n");
    }
    break;
  }

  case LWS_CALLBACK_SERVER_WRITEABLE: {
    if (!hdl || !hdl->disp.frame_buf) {
      break;
    }

    if (s && (s->last_frame_num != hdl->disp.last_refresh.frame_num)) {
      uint16_t x, y, w, h;

      if ((s->last_frame_num == 0) ||
          (hdl->disp.last_refresh.frame_num - s->last_frame_num > 1)) {
        /* Initial connect or missed frames: send full frame buffer */
        x = 0;
        y = 0;
        w = hdl->disp.w;
        h = hdl->disp.h;
      } else {
        /* Incremental update: send last refreshed rectangle */
        x = hdl->disp.last_refresh.x;
        y = hdl->disp.last_refresh.y;
        w = hdl->disp.last_refresh.w;
        h = hdl->disp.last_refresh.h;
      }

      /* Sanity check rectangle boundaries */
      if ((w == 0) || (h == 0) ||
          ((uint32_t)x + w > hdl->disp.w) ||
          ((uint32_t)y + h > hdl->disp.h)) {
        break;
      }

      size_t pixels_len = (size_t)w * h * 4; /* RGBA 4 bytes per pixel */
      size_t frame_data_len = sizeof(struct ws_wire_data_frame_s) + pixels_len;
      size_t total_msg_len = sizeof(struct ws_wire_msg_s) + frame_data_len;

      uint8_t *buf = malloc(LWS_PRE + total_msg_len);
      if (!buf) {
        break;
      }

      struct ws_wire_msg_s *msg = (struct ws_wire_msg_s *)(buf + LWS_PRE);
      msg->tag = WS_TAG_FRAME;
      msg->len = (uint32_t)frame_data_len;

      struct ws_wire_data_frame_s *frame = (struct ws_wire_data_frame_s *)msg->data;
      frame->x = x;
      frame->y = y;
      frame->w = w;
      frame->h = h;

      /* Copy rectangular region from display frame_buf into frame->pixels */
      for (uint16_t row = 0; row < h; ++row) {
        size_t src_offset = ((size_t)(y + row) * hdl->disp.w + x) * 4;
        size_t dst_offset = (size_t)row * w * 4;
        memcpy(&frame->pixels[dst_offset], &hdl->disp.frame_buf[src_offset], (size_t)w * 4);
      }

      int n = lws_write(wsi, buf + LWS_PRE, total_msg_len, LWS_WRITE_BINARY);
      free(buf);

      if (n < 0 || (size_t)n != total_msg_len) {
        fprintf(stderr, "ERROR %d writing to ws socket\n", n);
        return -1;
      }

      printf("WebSocket sent %s: (%u, %u) %ux%u (%zu bytes)\n",
             (s->last_frame_num == 0) ? "full frame" : "dirty rect",
             x, y, w, h, total_msg_len);

      s->last_frame_num = hdl->disp.last_refresh.frame_num;
    }
    break;
  }

  case LWS_CALLBACK_CLOSED:
    printf("WebSocket disconnected\n");
    break;

  default:
    break;
  }

  return 0;
}

int32_t ws_display_update_rec(uintptr_t handle,
                              uint16_t x1, uint16_t y1,
                              uint16_t width, uint16_t height,
                              uint8_t *pixels, size_t pixels_count) {
  /* 1. Handle and structure sanity checks */
  if (0 == handle) {
    return -1;
  }

  struct ws_object *ws = (struct ws_object *)handle;

  /* 2. Pixel buffer pointer sanity check */
  if (NULL == pixels) {
    return -2;
  }

  /* 3. Geometry and bounds sanity checks */
  if ((width == 0) || (height == 0)) {
    return -3;
  }

  if ((x1 >= ws->disp.w) || (y1 >= ws->disp.h)) {
    return -3;
  }

  if (((uint32_t)x1 + (uint32_t)width > (uint32_t)ws->disp.w) ||
      ((uint32_t)y1 + (uint32_t)height > (uint32_t)ws->disp.h)) {
    return -3;
  }

  /* 4. Pixel count / size sanity checks (RGB565 is 2 bytes per pixel) */
  size_t expected_pixels = (size_t)width * (size_t)height;
  size_t expected_bytes = expected_pixels * sizeof(uint16_t);

  /* Allow pixels_count to be either number of pixels or total buffer bytes */
  if ((pixels_count != expected_pixels) && (pixels_count != expected_bytes)) {
    return -4;
  }

  /* 5. Convert RGB565 pixels to RGBA and update destination rectangle in fb */
  for (uint16_t y = 0; y < height; ++y) {
    size_t src_row_offset = (size_t)y * width * 2;
    size_t dst_row_offset = ((size_t)(y1 + y) * ws->disp.w + x1) * 4;
    uint8_t *dst = &ws->disp.frame_buf[dst_row_offset];

    for (uint16_t x = 0; x < width; ++x) {
      size_t src_idx = src_row_offset + (size_t)x * 2;
      uint16_t rgb565 = (uint16_t)pixels[src_idx] | ((uint16_t)pixels[src_idx + 1] << 8);

      uint8_t r = (uint8_t)((((rgb565 >> 11) & 0x1f) * 255 + 15) / 31);
      uint8_t g = (uint8_t)((((rgb565 >> 5) & 0x3f) * 255 + 31) / 63);
      uint8_t b = (uint8_t)(((rgb565 & 0x1f) * 255 + 15) / 31);

      dst[4 * x + 0] = r;
      dst[4 * x + 1] = g;
      dst[4 * x + 2] = b;
      dst[4 * x + 3] = 255;
    }
  }

  // update last refresh info
  ws->disp.last_refresh.frame_num++;
  ws->disp.last_refresh.x = x1;
  ws->disp.last_refresh.y = y1;
  ws->disp.last_refresh.w = width;
  ws->disp.last_refresh.h = height;

  lws_callback_on_writable_all_protocol(ws->context, &protocols[1]);

  return 0;
}

int32_t ws_get_mouse(uintptr_t handle, struct ws_wire_data_mouse_s* mouse_data_out) {
  /* 1. Handle and structure sanity checks */
  if (0 == handle || NULL == mouse_data_out) {
    return (-1);
  }

  struct ws_object *ws = (struct ws_object *)handle;
  memcpy(mouse_data_out, &ws->mouse, sizeof(struct ws_wire_data_mouse_s));
  return 0;
}

uintptr_t ws_server_init(uint16_t port, uint16_t disp_width,
                         uint16_t disp_height) {
  struct ws_object *hdl = NULL;
  int32_t ret = 0;

  do {
    hdl = calloc(1, sizeof(struct ws_object));
    if (NULL == hdl) {
      ret = -1;
      break;
    }

    if ((disp_width == 0) || (disp_height == 0)) {
      ret = -2;
      break;
    }

    hdl->disp.frame_buf = calloc((size_t)disp_width * disp_height * 4UL, 1); // RGBA
    if (NULL == hdl->disp.frame_buf) {
      ret = -2;
      break;
    }
    hdl->disp.w = disp_width;
    hdl->disp.h = disp_height;
    hdl->disp.last_refresh.frame_num = 1; /* Frame 1 is the initial full frame */

    struct lws_context_creation_info info = {0};
    info.port = port;
    info.protocols = protocols;
    info.mounts = &mount;
    info.options = 0; /* Plain HTTP and WebSocket, no TLS. */
    info.user = hdl;

    hdl->context = lws_create_context(&info);
    if (!hdl->context) {
      ret = -3;
      break;
    }

  } while (0);

  if (ret != 0 && hdl != NULL) {
    switch (ret) {
    case -3:
      free(hdl->disp.frame_buf);
      hdl->disp.frame_buf = NULL;
      // FALLTHROUGH
    case -2:
      free(hdl);
      hdl = NULL;
      // FALLTHROUGH
    case -1:
      break;
    }
  }

  if (hdl) {
    printf("Open http://localhost:%d\n", port);
  }
  return (uintptr_t)hdl;
}

int32_t ws_service(uintptr_t handle) {
  if (0 == handle) {
    return -1;
  }
  struct ws_object *ws = (struct ws_object *)handle;
  return lws_service_tsi(ws->context, -1, 0);
}
