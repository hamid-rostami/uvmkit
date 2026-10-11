#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <raylib.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include "gdbstub.h"
#include "uvmkit/common/uvmkit_display_wire.h"
#include "uvmkit/common/uvmkit_sys.h"
#include "uvmkit/common/uvmkit_wire.h"
#include "uvmkit/events.h"
#include "uvmkit/utils.h"

#define LCD_WIDTH 240
#define LCD_HEIGHT 135
#define LCD_SCALE 1

static Texture2D display_texture;
static Color *display_pixels;

static Color rgb565_to_color(uint16_t pixel) {
  return (Color){
      .r = (uint8_t)((((pixel >> 11) & 0x1f) * 255 + 15) / 31),
      .g = (uint8_t)((((pixel >> 5) & 0x3f) * 255 + 31) / 63),
      .b = (uint8_t)(((pixel & 0x1f) * 255 + 15) / 31),
      .a = 255,
  };
}

static void display_draw(void) {
  BeginDrawing();
  ClearBackground(BLACK);
  DrawTextureEx(display_texture, (Vector2){0, 0}, 0.0f, LCD_SCALE, WHITE);
  EndDrawing();
}

static bool display_init(void) {
  InitWindow(LCD_WIDTH * LCD_SCALE, LCD_HEIGHT * LCD_SCALE, "UVMKit LCD");
  if (!IsWindowReady()) {
    fprintf(stderr, "raylib window initialization failed\n");
    return false;
  }

  SetTargetFPS(60);
  display_pixels = calloc(LCD_WIDTH * LCD_HEIGHT, sizeof(*display_pixels));
  if (NULL == display_pixels) {
    fprintf(stderr, "failed to allocate LCD framebuffer\n");
    CloseWindow();
    return false;
  }
  /* RGB565 color bars provide a visible startup frame until the VM renders. */
  static const uint16_t sample_colors[] = {
      0xf800, 0xffe0, 0x07e0, 0x07ff, 0x001f, 0xf81f, 0xffff, 0x0000,
  };
  for (int y = 0; y < LCD_HEIGHT; ++y) {
    for (int x = 0; x < LCD_WIDTH; ++x) {
      size_t bar = (size_t)x * (sizeof(sample_colors) / sizeof(sample_colors[0])) /
                   LCD_WIDTH;
      display_pixels[y * LCD_WIDTH + x] = rgb565_to_color(sample_colors[bar]);
    }
  }

  Image image = {
      .data = display_pixels,
      .width = LCD_WIDTH,
      .height = LCD_HEIGHT,
      .mipmaps = 1,
      .format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8,
  };
  display_texture = LoadTextureFromImage(image);
  if (display_texture.id == 0) {
    fprintf(stderr, "raylib texture initialization failed\n");
    free(display_pixels);
    display_pixels = NULL;
    CloseWindow();
    return false;
  }
  SetTextureFilter(display_texture, TEXTURE_FILTER_POINT);
  display_draw();
  return true;
}

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

  if (WindowShouldClose()) {
    uvm32_arg_setval(vmst, evt, RET, (uint32_t)-3);
    return false;
  }

  const uint8_t *src = pixels.ptr;
  for (size_t i = 0; i < pixel_count; ++i) {
    uint16_t rgb565 = (uint16_t)src[2 * i] | ((uint16_t)src[2 * i + 1] << 8);
    display_pixels[i] = rgb565_to_color(rgb565);
  }
  UpdateTextureRec(display_texture,
                   (Rectangle){(float)x1, (float)y1, (float)width,
                               (float)height},
                   display_pixels);
  display_draw();
  uvm32_arg_setval(vmst, evt, RET, 0);
  return true;
}

static void get_mouse(uvm32_state_t *vmst, uvm32_evt_t *evt) {
  /* Coordinates are signed values encoded in the shared u32 wire format. */
  uvm32_slice_t buf =
      uvm32_arg_getslice_fixed(vmst, evt, ARG0,
                               sizeof(struct uvmkit_touchpad_s));
  if (buf.len != sizeof(struct uvmkit_touchpad_s)) {
    uvm32_arg_setval(vmst, evt, RET, (uint32_t)-1);
    return;
  }

  struct uvmkit_touchpad_s* data = (struct uvmkit_touchpad_s*) buf.ptr;

  if (WindowShouldClose()) {
    uvm32_arg_setval(vmst, evt, RET, (uint32_t)-3);
    return;
  }

  display_draw();
  Vector2 ballPosition = GetMousePosition();
  data->x = (uint32_t)ballPosition.x;
  data->y = (uint32_t)ballPosition.y;
  data->state = IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
  uvm32_arg_setval(vmst, evt, RET, (uint32_t)0);
}

static void display_destroy(void) {
  if (display_texture.id != 0) {
    UnloadTexture(display_texture);
  }
  free(display_pixels);
  if (IsWindowReady()) {
    CloseWindow();
  }
}

static void display_wait_until_closed(void) {
  while (!WindowShouldClose()) {
    display_draw();
  }
}

const char *usage = "Usage: %s [-d] [file...]\n";
#define PORT 1234

static void run_gdbstub(uvm32_state_t *vmst) {
  int server_sock;
  struct sockaddr_in server_addr;

  uvm32_useBreak(vmst, true); // enable ebreak

  // Create server socket
  if ((server_sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    printf("Socket creation failed\n");
    return;
  }

  // Set up the server address
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = INADDR_ANY;
  server_addr.sin_port = htons(PORT);

  // Bind the server socket
  if (bind(server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) <
      0) {
    printf("Bind failed\n");
    close(server_sock);
    return;
  }

  // Listen for incoming connections
  if (listen(server_sock, 1) < 0) {
    printf("Listen failed\n");
    close(server_sock);
    return;
  }

  printf("GDB Stub listening on port %d, fd:%d...\n", PORT, server_sock);

  struct sockaddr_in client_addr;
  socklen_t addr_len = sizeof(client_addr);

  // Accept a client connection
  int client_sock =
      accept(server_sock, (struct sockaddr *)&client_addr, &addr_len);
  if (0 > client_sock) {
    printf("Accept failed: %d\n", client_sock);
    close(server_sock);
    return;
  }

  printf("GDB connected from %s, fd: %d\n", inet_ntoa(client_addr.sin_addr),
         client_sock);

  /* Attach gdbstub to vmst */
  struct gdb_state state;
  gdb_sys_init_state(&state, vmst, client_sock, uvmkit_event_handler);

  while (1) {
    gdb_main(&state);
  }
  close(client_sock);
  close(server_sock);
}

static void run(uvm32_state_t *vmst) {
  enum { STEPS = 1000 };
  uvm32_evt_t evt;
  bool running = true;

  while (running) {
    usleep(1);
    uint32_t ran_steps = uvm32_run(vmst, &evt, STEPS);

    switch (evt.typ) {
    default:
      printf("Unknown event %d", evt.typ);
      break;

    case UVM32_EVT_END:
      printf("vm ended\n");
      running = false;
      break;

    case UVM32_EVT_ERR:
      if ((UVM32_ERR_HUNG == evt.data.err.errcode) && (STEPS == ran_steps)) {
        // keep running
        uvm32_clearError(vmst);
      } else {
        printf("UVM32_EVT_ERR '%s' (%d)\n", evt.data.err.errstr,
               (int)evt.data.err.errcode);
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
  } // while
}

int main(int argc, char *argv[]) {
  bool debug = false;

  int opt;
  while ((opt = getopt(argc, argv, "d")) != -1) {
    switch (opt) {
    case 'd':
      debug = true;
      break;
    default:
      fprintf(stderr, usage, argv[0]);
      exit(EXIT_FAILURE);
    }
  }

  if (optind >= argc) {
    fprintf(stderr, usage, argv[0]);
    exit(EXIT_FAILURE);
  }

  int32_t err = 0;
  uvm32_state_t *vmst = uvm32_init_from_file(argv[optind], &err);
  assert((NULL != vmst) && (0 == err)); // FIXME

  if (!display_init()) {
    free(vmst);
    return EXIT_FAILURE;
  }

  if (debug) {
    run_gdbstub(vmst);
  } else {
    run(vmst);
  }

  display_wait_until_closed();

  free(vmst);
  display_destroy();
  return 0;
}
