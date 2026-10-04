#include "stdio.h"
#include "string.h"
#include "uvm32_target.h"

#include <sys/socket.h>
//#include <netinet/in.h>
#include <arpa/inet.h>

//#include "mpack.h"

int main() {
  int server_fd;
  
  

  char buffer[128];  

  /*
   * Create TCP socket
   */
  server_fd = socket(AF_INET, SOCK_STREAM, 0);
  sprintf(buffer, "socket: %d", server_fd);
  println(buffer);
  if (0 > server_fd) {
    return -1;
  }
  

  struct sockaddr_in addr = {0};
  addr.sin_family = AF_INET;
  addr.sin_port = htons(1234);
  addr.sin_addr.s_addr = INADDR_ANY;

  int ret = bind(server_fd,
                 (struct sockaddr *)&addr,
                 sizeof(addr));
  sprintf(buffer, "bind: %d, %d", server_fd, ret);
  println(buffer);
  if (0 > ret) {
    return -2;
  }

  ret = listen(server_fd, 1);
  sprintf(buffer, "listen: %d", ret);
  println(buffer);


  while (1) {
    struct sockaddr_in client_addr;
    socklen_t addr_len = sizeof(client_addr);
    int client_fd = accept(server_fd,
                           (struct sockaddr *)&client_addr,
                           &addr_len);
    sprintf(buffer, "Connected from fd: %d\n",
            client_fd);
    println(buffer);

    while (1) {
      int n = recv(client_fd,
                   buffer,
                   sizeof(buffer),
                   0);

      if (0 < n) {
        (void) send(client_fd, buffer, n, 0);
      } else {
        break;
      }
    }
    println("Closing connection\n");
    (void) close(client_fd);
  }

  return 0;
}
