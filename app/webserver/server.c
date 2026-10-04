#include <assert.h>
#include "stdio.h"
#include "string.h"
#include "uvm32_target.h"

#include <sys/socket.h>
#include <arpa/inet.h>

#include "picohttpparser.h"

int main() {
  int server_fd;
  char buf[1024];

  /*
   * Create TCP socket
   */
  server_fd = socket(AF_INET, SOCK_STREAM, 0);
  sprintf(buf, "socket: %d", server_fd);
  println(buf);
  if (0 > server_fd) {
    return -1;
  }


  struct sockaddr_in addr = {0};
  addr.sin_family = AF_INET;
  addr.sin_port = htons(1235);
  addr.sin_addr.s_addr = INADDR_ANY;

  int ret = bind(server_fd,
                 (struct sockaddr *)&addr,
                 sizeof(addr));
  sprintf(buf, "bind: %d, %d", server_fd, ret);
  println(buf);
  if (0 > ret) {
    return -2;
  }

  ret = listen(server_fd, 1);
  sprintf(buf, "listen: %d", ret);
  println(buf);


  while (1) {
    struct sockaddr_in client_addr;
    socklen_t addr_len = sizeof(client_addr);
    int client_fd = accept(server_fd,
                           (struct sockaddr *)&client_addr,
                           &addr_len);
    sprintf(buf, "Connected from fd: %d\n",
            client_fd);
    println(buf);


    const char *method;
    const char *path;
    int pret, minor_version;
    struct phr_header headers[100];
    size_t buflen = 0, prevbuflen = 0, method_len, path_len, num_headers;
    ssize_t rret;

    while (1) {
      /* read the request */
      rret = recv(client_fd, buf + buflen, sizeof(buf) - buflen, 0);
      if (rret <= 0) {
        break;
      }
      prevbuflen = buflen;
      buflen += rret;
      /* parse the request */
      num_headers = sizeof(headers) / sizeof(headers[0]);
      pret = phr_parse_request(buf, buflen, &method, &method_len, &path, &path_len,
                               &minor_version, headers, &num_headers, prevbuflen);

      if (pret > 0)
        break; /* successfully parsed the request */
      else if (pret == -1)
        break; // parser error
      /* /\* request is incomplete, continue the loop *\/ */
      assert(pret == -2);
      /* if (buflen == sizeof(buf)) */
      /*   return RequestIsTooLongError; */
    }

    if (0 < pret) {
      char str[50];
      sprintf(str, "request is %d bytes long\n", pret);
      println(str);
      sprintf(str, "method is %.*s\n", (int)method_len, method);
      println(str);
      sprintf(str, "path is %.*s\n", (int)path_len, path);
      println(str);
      sprintf(str, "HTTP version is 1.%d\n", minor_version);
      println(str);
      /* printf("headers:\n"); */
      /* for (i = 0; i != num_headers; ++i) { */
      /*   printf("%.*s: %.*s\n", (int)headers[i].name_len, headers[i].name, */
      /*          (int)headers[i].value_len, headers[i].value); */
      /* } */
    }

    println("Closing connection\n");
    (void) close(client_fd);
  }


  return 0;
}
