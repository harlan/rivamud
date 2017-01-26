#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <unistd.h>
#include <netdb.h>
#include "network_listener.h"

int sendall(int sockfd, char *buf, int *len) {
  int nleft = *len;
  int total = 0;
  int n;

  while (total < *len) {
    n = send(sockfd, buf+total, nleft, 0);
    total += n;
    nleft -= n;
  }

  *len = total;

  return n==-1?-1:0;
}

void *connection_handler(void *socket_desc) {
  int sock = *(int*)socket_desc;
  struct sockaddr_storage remoteaddr;
  socklen_t remoteaddrlen;
  char *message = "Greetings!\n";
  const int buf_len = 10000;
  char read_buf[buf_len];
  int read_size;
  char ipstr[INET6_ADDRSTRLEN];
  char portstr[NI_MAXSERV];
  int len;

  remoteaddrlen = sizeof remoteaddr;
  getpeername(sock, (struct sockaddr *)&remoteaddr, &remoteaddrlen);
  getnameinfo((struct sockaddr *)&remoteaddr, remoteaddrlen,
              ipstr, sizeof ipstr, portstr, sizeof portstr,
              NI_NUMERICHOST | NI_NUMERICSERV);

  len = strlen(message);
  if (sendall(sock, message, &len) == -1) {
    perror("send");  // are send errors fatal ?
    fprintf(stderr, "server: send error to %s:%s", ipstr, portstr);
  }

  // Receive a message from the client
  int did_find_end = 0;
  while (!did_find_end && (read_size = recv(sock, read_buf, buf_len, 0)) > 0) {
    read_buf[read_size] = '\0';

    // send message back to client
    len = strlen(read_buf);
    if (sendall(sock, read_buf, &len) == -1) {
      perror("send");
      fprintf(stderr, "server: send error to %s:%s", ipstr, portstr);
    }

    for (int i = 0; i < read_size; i++) {
      char eof = '\0';
      char end_of_transmission = '\4';
      if (read_buf[i] == eof || read_buf[i] == end_of_transmission) {
	did_find_end = 1;
      }
    }
    memset(read_buf, 0, buf_len);
  }

  if (-1 == read_size) {
    // TODO  not sure if this always indicates a connection error that
    // requires the socket to be closed.  might be able to move this into
    // recv loop and continue looping
    perror("recv failed");
  }
  printf("server: client %s:%s disconnected\n", ipstr, portstr);

  if (0 > close(sock)) {
    perror("Error shutting down socket");
  }
  fprintf(stderr, "Connection closed\n");

  return NULL;
}

int start_server(int port) {
  int listenfd, client_sock;
  char portstr[NI_MAXSERV];
  char ipstr[INET6_ADDRSTRLEN];
  int rv;
  int yes=1;
  struct addrinfo hints, *ai, *p;
  struct sockaddr_storage remoteaddr;
  socklen_t remoteaddrlen;

  snprintf(portstr, NI_MAXSERV, "%d", port);

  memset(&hints, 0, sizeof hints);
  hints.ai_family   = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags    = AI_PASSIVE;
  if ( (rv=getaddrinfo(NULL, portstr, &hints, &ai)) != 0 ) {
    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
    exit(1);
  }

  for (p = ai; p != NULL; p = p->ai_next) {
    listenfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
    if (listenfd == -1) {
      perror("server: socket");
      continue;
    }

    rv = setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
    if (rv == -1) {
      perror("setsockopt");
      exit(2);
    }

    rv = bind(listenfd, p->ai_addr, p->ai_addrlen);
    if (rv == -1) {
      perror("server: bind");
      close(listenfd);
      continue;
    }

    break;
  }

  if (p == NULL) {
    fprintf(stderr, "server: could not bind\n");
    exit(2);
  }
  getnameinfo(p->ai_addr, p->ai_addrlen,
      ipstr, sizeof ipstr, portstr, sizeof portstr,
      NI_NUMERICHOST | NI_NUMERICSERV);
  freeaddrinfo(ai);

  // listen
  if ((rv = listen(listenfd, BACKLOG)) == -1) {
    perror("server: listen");
    exit(2);
  }
  printf("server: listening on %s:%s...\n", ipstr, portstr);

  pthread_t thread_id;

  // accept connections
  while (1) {
    remoteaddrlen = sizeof remoteaddr;
    client_sock = accept(listenfd, (struct sockaddr *)&remoteaddr,
                         &remoteaddrlen);
    if (client_sock == -1) {
      perror("accept");
      continue;
    }

    getnameinfo((struct sockaddr *)&remoteaddr, remoteaddrlen,
        ipstr, sizeof ipstr, portstr, sizeof portstr,
        NI_NUMERICHOST | NI_NUMERICSERV);
    printf("server: accepted connection from %s:%s\n", ipstr, portstr);

    if (pthread_create(&thread_id, NULL, connection_handler, 
		       (void*)&client_sock) < 0) {
      perror("Could not spawn thread to handle connection");
    }
  }

  return 0;
}
