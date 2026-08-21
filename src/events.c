#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "uvmkit/common/uvmkit_socket_wire.h"
#include "uvmkit/common/uvmkit_sys.h"
#include "uvmkit/common/uvmkit_wire.h"
#include "uvmkit/events.h"

int32_t uvmkit_event_handler(uvm32_state_t *vmst, uvm32_evt_t *evt) {
  int32_t ret = 0;

  switch (evt->data.syscall.code) {
  case UVM32_SYSCALL_YIELD: {
    break;
  }
  case UVM32_SYSCALL_PUTC: {
    printf("APP: %c", uvm32_arg_getval(vmst, evt, ARG0));
    break;
  }
  case UVM32_SYSCALL_PRINTLN: {
    const char *str = uvm32_arg_getcstr(vmst, evt, ARG0);
    printf("APP: %s\n", str);
    break;
  }

  case UVM32_SYSCALL_POSIX_SOCKET: {
    uvm32_slice_t buf = uvm32_arg_getslice(vmst, evt, ARG0, ARG1);
    if (sizeof(struct uvmkit_socket_s) != buf.len) {
      uvm32_arg_setval(vmst, evt, RET,
                       (uint32_t)-1); // TODO: check return value */
      break;
    }
    struct uvmkit_socket_s *s = (struct uvmkit_socket_s *)buf.ptr;
    int domain = (int)uvmkit_decode_u32(&s->domain);
    int type = (int)uvmkit_decode_u32(&s->type);
    int protocol = (int)uvmkit_decode_u32(&s->protocol);

    int server_fd = socket(domain, type, protocol);
    printf("<< SOCKET syscall %d, %d, %d >> %d\n", domain, type, protocol,
           server_fd);
    uvm32_arg_setval(vmst, evt, RET, (uint32_t)server_fd);
    break;
  }

  case UVM32_SYSCALL_POSIX_BIND: {
    uvm32_slice_t buf = uvm32_arg_getslice(vmst, evt, ARG0, ARG1);
    if (sizeof(struct uvmkit_bind_s) != buf.len) {
      uvm32_arg_setval(vmst, evt, RET,
                       (uint32_t)-1); // TODO: check return value
      break;
    }
    struct uvmkit_bind_s *data = (struct uvmkit_bind_s *)buf.ptr;
    int sockfd = (int)uvmkit_decode_u32(&data->sockfd);
    struct sockaddr_in addr = {0};
    addr.sin_family = (int)uvmkit_decode_u32(&data->family);
    addr.sin_port = (int)uvmkit_decode_u32(&data->port);
    addr.sin_addr.s_addr = (int)uvmkit_decode_u32(&data->address);

    int ret = bind(sockfd, (const struct sockaddr *)&addr, sizeof(addr));
    printf("<< BIND syscall fd:%d, port:%d, fam: %d>> %d\n", sockfd,
           addr.sin_port, addr.sin_family, ret);
    uvm32_arg_setval(vmst, evt, RET, (uint32_t)ret);
    break;
  }

  case UVM32_SYSCALL_POSIX_LISTEN: {
    uvm32_slice_t buf = uvm32_arg_getslice(vmst, evt, ARG0, ARG1);
    if (sizeof(struct uvmkit_listen_s) != buf.len) {
      uvm32_arg_setval(vmst, evt, RET,
                       (uint32_t)-1); // TODO: check return value
      break;
    }
    struct uvmkit_listen_s *data = (struct uvmkit_listen_s *)buf.ptr;

    int sockfd = (int)uvmkit_decode_u32(&data->sockfd);
    int backlog = (int)uvmkit_decode_u32(&data->backlog);

    int ret = listen(sockfd, backlog);
    printf("<< LISTEN syscall fd:%d, backlog=%d >> %d\n", sockfd, backlog, ret);
    uvm32_arg_setval(vmst, evt, RET, (uint32_t)ret);
    break;
  }

  case UVM32_SYSCALL_POSIX_ACCEPT: {
    uvm32_slice_t buf = uvm32_arg_getslice(vmst, evt, ARG0, ARG1);
    if (sizeof(struct uvmkit_accept_s) != buf.len) {
      uvm32_arg_setval(vmst, evt, RET,
                       (uint32_t)-1); // TODO: check return value */
      break;
    }
    struct uvmkit_accept_s *data = (struct uvmkit_accept_s *)buf.ptr;
    int sockfd = (int)uvmkit_decode_u32(&data->to_host.sockfd);
    ;

    struct sockaddr_in addr;
    socklen_t addr_len = sizeof(addr);

    printf("<< ACCEPT syscall %d\n", sockfd);
    int client_fd = accept(sockfd, (struct sockaddr *)&addr, &addr_len);

    uvmkit_encode_u32(&data->from_host.family, addr.sin_family);
    uvmkit_encode_u32(&data->from_host.port, addr.sin_port);
    uvmkit_encode_u32(&data->from_host.addr, addr.sin_addr.s_addr);

    uvm32_arg_setval(vmst, evt, RET, (uint32_t)client_fd);
    break;
  }

  case UVM32_SYSCALL_POSIX_RECV: {
    uvm32_slice_t buf1 =
        uvm32_arg_getslice_fixed(vmst, evt, ARG0, sizeof(struct uvmkit_recv_s));

    if (0 == buf1.len) {
      uvm32_arg_setval(vmst, evt, RET,
                       (uint32_t)-1); // TODO: check return value
      break;
    }

    struct uvmkit_recv_s *data = (struct uvmkit_recv_s *)buf1.ptr;
    int sockfd = (int)uvmkit_decode_u32(&data->sockfd);
    size_t len = (int)uvmkit_decode_u32(&data->len);
    int flags = (int)uvmkit_decode_u32(&data->flags);

    uvm32_slice_t buf2 = uvm32_arg_getslice_fixed(vmst, evt, ARG1, len);
    if (0 == buf2.len) {
      uvm32_arg_setval(vmst, evt, RET,
                       (uint32_t)-2); // TODO: check return value
      break;
    }

    printf("Recv len: %ld\n", len);
    ssize_t n = recv(sockfd, buf2.ptr, len, flags);
    printf("Recvd %ld bytes\n", n);
    uvm32_arg_setval(vmst, evt, RET, (uint32_t)n);
    break;
  }

  case UVM32_SYSCALL_POSIX_SEND: {
    uvm32_slice_t buf1 =
        uvm32_arg_getslice_fixed(vmst, evt, ARG0, sizeof(struct uvmkit_send_s));

    if (0 == buf1.len) {
      uvm32_arg_setval(vmst, evt, RET,
                       (uint32_t)-1); // TODO: check return value
      break;
    }

    struct uvmkit_send_s *data = (struct uvmkit_send_s *)buf1.ptr;
    int sockfd = (int)uvmkit_decode_u32(&data->sockfd);
    size_t len = (int)uvmkit_decode_u32(&data->len);
    int flags = (int)uvmkit_decode_u32(&data->flags);

    uvm32_slice_t buf2 = uvm32_arg_getslice_fixed(vmst, evt, ARG1, len);
    if (0 == buf2.len) {
      uvm32_arg_setval(vmst, evt, RET,
                       (uint32_t)-2); // TODO: check return value
      break;
    }

    ssize_t ret = send(sockfd, buf2.ptr, len, flags);
    uvm32_arg_setval(vmst, evt, RET, (uint32_t)ret);
    break;
  }

  case UVM32_SYSCALL_POSIX_CLOSE: {
    uvm32_slice_t buf = uvm32_arg_getslice(vmst, evt, ARG0, ARG1);

    if (sizeof(struct uvmkit_close_s) != buf.len) {
      uvm32_arg_setval(vmst, evt, RET,
                       (uint32_t)-1); // TODO: check return value
      break;
    }

    struct uvmkit_close_s *data = (struct uvmkit_close_s *)buf.ptr;
    int sockfd = (int)uvmkit_decode_u32(&data->sockfd);
    int ret = close(sockfd);
    uvm32_arg_setval(vmst, evt, RET, (uint32_t)ret);
    break;
  }

  default:
    printf("Unhandled syscall 0x%08x\n", evt->data.syscall.code);
    ret = -1;
    break;
  }

  return ret;
}
