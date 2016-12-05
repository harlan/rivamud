#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <unistd.h>
#include "network_listener.h"

void *connection_handler(void *socket_desc) {
  int sock = *(int*)socket_desc;
  char *message = "Greetings!\n";
  const int buf_len = 10000;
  char read_buf[buf_len];
  int read_size;

  write(sock, message, strlen(message));

  // Receive a message from the client
  int did_find_end = 0;
  while (!did_find_end && (read_size = recv(sock, read_buf, buf_len, 0)) > 0) {
    read_buf[read_size] = '\0';
    // send message back to client
    write(sock, read_buf, strlen(read_buf));

    for (int i = 0; i < read_size; i++) {
      char eof = '\0';
      char end_of_transmission = '\4';
      if (read_buf[i] == eof || read_buf[i] == end_of_transmission) {
	did_find_end = 1;
      }
    }
    memset(read_buf, 0, buf_len);
  }

  if (0 == read_size) {
    fprintf(stderr, "Client disconnected\n");
  } else if (-1 == read_size) {
    perror("recv failed");
  }

  if (0 > close(sock)) {
    perror("Error shutting down socket");
  }
  fprintf(stderr, "Connection closed\n");

  return NULL;
}

int start_server(int port) {
  int socket_desc, client_sock;
  struct sockaddr_in server, client;
  
  // Create socket
  socket_desc = socket(AF_INET, SOCK_STREAM, 0);
  if (-1 == socket_desc) {
    fprintf(stderr, "Could not create socket\n");
    exit(1);
  }
  // prepare the sockaddr_in structure
  server.sin_family = AF_INET;
  server.sin_addr.s_addr = INADDR_ANY;
  server.sin_port = htons(port);

  // bind to port
  if (bind(socket_desc, (struct sockaddr*) &server, sizeof(server)) < 0) {
    perror("Bind failed");
    exit(1);
  }

  // listen
  int max_connections_backlog = 3;
  if (0 != listen(socket_desc, max_connections_backlog)) {
    perror("Error listening to socket");
  }
  
  // accept and incoming connection
  printf("Waiting for incoming connections on port %d.\n", port);
  int c = sizeof(struct sockaddr_in);
  pthread_t thread_id;

  while ((client_sock = accept(socket_desc, (struct sockaddr*)&client, 
			       (socklen_t*) &c))) {
    char client_ip_str[200];
    if (NULL == inet_ntop(client.sin_family, &client.sin_addr.s_addr, 
			  client_ip_str, sizeof(client_ip_str))) {
      perror("Can't determine ip address of incoming connection");
    }
    printf("Connection accepted from %s\n", client_ip_str);

    if (pthread_create(&thread_id, NULL, connection_handler, 
		       (void*)&client_sock) < 0) {
      perror("Could not spawn thread to handle connection");
    }
  }
  if (client_sock < 0) {
    perror("Failed accepting incoming connection from socket");
    exit(1);
  }
  
  return 0;
}
