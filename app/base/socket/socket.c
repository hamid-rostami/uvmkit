#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>

#include "uvm32_target.h"
#include "uvmkit_wire.h"
#include "uvmkit_socket_wire.h"
//#include "uvmkit_sys_args.h"

int socket(int domain, int type, int protocol) {
  struct uvmkit_socket_s data = {0};
  
  uvmkit_encode_u32(&data.domain, domain);
  uvmkit_encode_u32(&data.type, type);
  uvmkit_encode_u32(&data.protocol, protocol);
  return syscall_cast(UVM32_SYSCALL_POSIX_SOCKET,
                     &data,
                     sizeof(data));
}

int bind(int sockfd,
         const struct sockaddr *addr,
         socklen_t addrlen) {
  struct uvmkit_bind_s data = {0};
  struct sockaddr_in * saddr = (struct sockaddr_in *) addr;
  
  uvmkit_encode_u32(&data.sockfd, sockfd);
  uvmkit_encode_u32(&data.family, saddr->sin_family);
  uvmkit_encode_u32(&data.port, saddr->sin_port);
  uvmkit_encode_u32(&data.address, saddr->sin_addr.s_addr);

  return syscall_cast(UVM32_SYSCALL_POSIX_BIND,
                      &data,
                      sizeof(data));
}

int listen(int sockfd, int backlog) {
  struct uvmkit_listen_s data = {0};
  uvmkit_encode_u32(&data.sockfd, sockfd);
  uvmkit_encode_u32(&data.backlog, backlog);

  return syscall_cast(UVM32_SYSCALL_POSIX_LISTEN,
                     &data,
                     sizeof(data));
}

int accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen) {
  struct uvmkit_accept_s data = {0};
  uint32_t ret = 0;
  uvmkit_encode_u32(&data.to_host.sockfd, sockfd);

  ret = syscall_cast(UVM32_SYSCALL_POSIX_ACCEPT,
                     &data,
                     sizeof(data));
  if(0 > ret) {
    return ret;
  }

  struct sockaddr_in *saddr = (struct sockaddr_in *)addr;
  (void) addrlen;
  saddr->sin_family = uvmkit_decode_u32(&data.from_host.family);
  saddr->sin_port = uvmkit_decode_u32(&data.from_host.port);
  saddr->sin_addr.s_addr = uvmkit_decode_u32(&data.from_host.addr);

  return ret;
}

ssize_t recv(int sockfd, void *mem, size_t len, int flags) {
  struct uvmkit_recv_s data = {0};
  uint32_t ret = 0;
  uvmkit_encode_u32(&data.sockfd, sockfd);
  uvmkit_encode_u32(&data.len, len);
  uvmkit_encode_u32(&data.flags, flags);

  return syscall_cast(UVM32_SYSCALL_POSIX_RECV,
                     &data,
                     (uint32_t)mem);
}

ssize_t send(int sockfd, const void *mem, size_t len, int flags)
{
  struct uvmkit_send_s data = {0};
  uint32_t ret = 0;
  uvmkit_encode_u32(&data.sockfd, sockfd);
  uvmkit_encode_u32(&data.len, len);
  uvmkit_encode_u32(&data.flags, flags);

  return syscall_cast(UVM32_SYSCALL_POSIX_SEND,
                      &data,
                      (uint32_t)mem);
}

int close(int sockfd) {
  struct uvmkit_close_s data = {0};
  uvmkit_encode_u32(&data.sockfd, sockfd);
  return syscall_cast(UVM32_SYSCALL_POSIX_CLOSE,
                      &data,
                      sizeof(data));
}

/* #define PORT 8080 */

/* int main(void) */
/* { */
/*     int server_fd; */
/*     int client_fd; */

/*     struct sockaddr_in addr; */

/*     char buffer[128]; */

/*     /\* */
/*      * Create TCP socket */
/*      *\/ */
/*     server_fd = socket(AF_INET, SOCK_STREAM, 0); */
/*     if (server_fd < 0) { */
/*         return -1; */
/*     } */

/*     /\* */
/*      * Fill server address */
/*      *\/ */
/*     memset(&addr, 0, sizeof(addr)); */

/*     addr.sin_family = AF_INET; */
/*     addr.sin_port = htons(PORT); */
/*     addr.sin_addr.s_addr = INADDR_ANY; */

/*     /\* */
/*      * Bind socket */
/*      *\/ */
/*     if (bind(server_fd, */
/*              (struct sockaddr *)&addr, */
/*              sizeof(addr)) < 0) { */
/*         return -2; */
/*     } */

/*     /\* */
/*      * Listen for connections */
/*      *\/ */
/*     if (listen(server_fd, 1) < 0) { */
/*         return -3; */
/*     } */

/*     /\* */
/*      * Accept one client */
/*      *\/ */
/*     client_fd = accept(server_fd, 0, 0); */
/*     if (client_fd < 0) { */
/*         return -4; */
/*     } */

/*     /\* */
/*      * Receive data */
/*      *\/ */
/*     int n = recv(client_fd, */
/*                  buffer, */
/*                  sizeof(buffer) - 1, */
/*                  0); */

/*     if (n > 0) { */

/*         buffer[n] = 0; */

/*         /\* */
/*          * Send reply */
/*          *\/ */
/*         const char reply[] = "hello from riscv vm\n"; */

/*         send(client_fd, */
/*              reply, */
/*              sizeof(reply) - 1, */
/*              0); */
/*     } */

/*     close(client_fd); */
/*     close(server_fd); */

/*     return 0; */
/* } */
