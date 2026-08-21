#pragma once

#include <stdint.h>

struct uvmkit_socket_s {
  uint32_t domain;
  uint32_t type;
  uint32_t protocol;
};

struct uvmkit_bind_s {
  uint32_t sockfd;
  uint32_t family;
  uint32_t port;
  uint32_t address;
};

struct uvmkit_listen_s {
  uint32_t sockfd;
  uint32_t backlog;
};

struct uvmkit_accept_s {
  union {
    /* to host */
    struct {
      uint32_t sockfd;
    } to_host;

    /* from host */
    struct {
      uint32_t family;
      uint32_t port;
      uint32_t addr;
    } from_host;
  };
};

struct uvmkit_recv_s {
  uint32_t sockfd;
  uint32_t len;
  uint32_t flags;
  // Pointer to buf goes with ARG1
};

struct uvmkit_send_s {
  uint32_t sockfd;
  uint32_t len;
  uint32_t flags;
  // Pointer to buf goes with ARG1
};

struct uvmkit_close_s {
  uint32_t sockfd;
};
