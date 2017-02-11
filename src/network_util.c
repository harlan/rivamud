#include <stdio.h>
#include <sys/socket.h>
#include "network_util.h"

/* not safe yet, need to do nonblocking and have thread safe buffers
 * or something */
ssize_t readline(int fd, char *buf, size_t maxlen) {
  int rv;

  rv = recv(fd, buf, maxlen, 0);
  if (rv == -1)
    perror("recv");

  return rv;
}

/* not safe yet, need to do nonblocking and have thread safe buffers
 * or something */
ssize_t sendline(int fd, char *buf, size_t len) {
  int total = 0;
  int n;
  int nleft = len;

  while (total < len) {
    n = send(fd, buf+total, nleft, 0);
    if (n == -1) {
      perror("send");
      return -1;
    }
    total += n;
    nleft -= n;
  }

  return total;
}
