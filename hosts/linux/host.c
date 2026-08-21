#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include "gdbstub.h"
#include "uvmkit/events.h"
#include "uvmkit/utils.h"

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
      (void)uvmkit_event_handler(vmst, &evt);
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

  if (debug) {
    run_gdbstub(vmst);
  } else {
    run(vmst);
  }

  free(vmst);
  return 0;
}
