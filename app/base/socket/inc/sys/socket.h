#pragma once

#include <stdint.h>

#define AF_UNSPEC       0
#define AF_INET         2
#define AF_INET6        10

/* Socket protocol types (TCP/UDP/RAW) */
#define SOCK_STREAM     1
#define SOCK_DGRAM      2
#define SOCK_RAW        3

/** 255.255.255.255 */
#define INADDR_NONE         ((uint32_t)0xffffffffUL)
/** 127.0.0.1 */
#define INADDR_LOOPBACK     ((uint32_t)0x7f000001UL)
/** 0.0.0.0 */
#define INADDR_ANY          ((uint32_t)0x00000000UL)
/** 255.255.255.255 */
#define INADDR_BROADCAST    ((uint32_t)0xffffffffUL)


#define htons(x) __builtin_bswap16(x)
#define htonl(x) __builtin_bswap32(x)

typedef uint32_t socklen_t;

struct in_addr {
  uint32_t s_addr;
};

struct sockaddr_in {
  uint8_t  sin_family;
  uint16_t sin_port;
  struct in_addr sin_addr;
};


struct sockaddr {
  uint8_t _[];
};


int socket(int domain, int type, int protocol);
int bind(int sockfd,
         const struct sockaddr *addr,
         socklen_t addrlen);
int listen(int s, int backlog);
int accept(int s, struct sockaddr *addr, socklen_t *addrlen);
ssize_t recv(int sockfd, void *mem, size_t len, int flags);
ssize_t send(int sockfd, const void *mem, size_t len, int flags);
int close(int sockfd);
