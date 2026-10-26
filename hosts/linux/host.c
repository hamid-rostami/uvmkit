#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

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

#include "ws.h"

#define LCD_WIDTH 240
#define LCD_HEIGHT 135
#define LCD_SCALE 1

static uintptr_t ws_handle;
const char *usage = "Usage: %s [-d] [file...]\n";
#define PORT 1234

static bool display_frame(uvm32_state_t *vmst, uvm32_evt_t *evt) {
  if (0 == ws_handle) {
    uvm32_arg_setval(vmst, evt, RET, (uint32_t)-1);
    return false;
  }

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
  (void) ws_display_update_rec(ws_handle,
                               x1, y1,
                               width, height,
                               pixels.ptr, pixel_count);
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

  /* Vector2 ballPosition = GetMousePosition(); */
  /* data->x = (uint32_t)ballPosition.x; */
  /* data->y = (uint32_t)ballPosition.y; */
  /* data->state = IsMouseButtonPressed(MOUSE_BUTTON_LEFT); */
  struct ws_wire_data_mouse_s mouse;
  if (0 != ws_get_mouse(ws_handle, &mouse)) {
    uvm32_arg_setval(vmst, evt, RET, (uint32_t)-2);
    return;
  }
  data->x = mouse.x;
  data->y = mouse.y;
  data->state = (mouse.btn == 1);
  uvm32_arg_setval(vmst, evt, RET, (uint32_t)0);
}

/* static void display_destroy(void) { */
/*   if (display_texture.id != 0) { */
/*     UnloadTexture(display_texture); */
/*   } */
/*   free(display_pixels); */
/*   if (IsWindowReady()) { */
/*     CloseWindow(); */
/*   } */
/* } */

/* static void display_wait_until_closed(void) { */
/*   while (!WindowShouldClose()) { */
/*     display_draw(); */
/*   } */
/* } */


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
  enum { STEPS = 10000 };
  uvm32_evt_t evt;
  bool running = true;

  while (running) {
    uint32_t ran_steps = uvm32_run(vmst, &evt, STEPS);
    ws_service(ws_handle);

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
        ws_service(ws_handle);
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
  uint16_t port = 9000;
  //(void) ws_server_init(9000);
  
    
  int opt;
  while ((opt = getopt(argc, argv, "d:p:")) != -1) {
    switch (opt) {
    case 'd':
      debug = true;
      break;
    case 'p':
      port = atoi(optarg);
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

  ws_handle = ws_server_init(port, LCD_WIDTH, LCD_HEIGHT);
  if (0 == ws_handle) {
    printf("Failed to create websocket server\n");
  }

  int32_t err = 0;
  uvm32_state_t *vmst = uvm32_init_from_file(argv[optind], &err);
  assert((NULL != vmst) && (0 == err)); // FIXME

  if (debug) {
    run_gdbstub(vmst);
  } else {
    run(vmst);
  }

  free(vmst);
  return 0;
}
