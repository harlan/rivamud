#ifndef _NETWORK_LISTENER_H
#define _NETWORK_LISTENER_H

#define BACKLOG 10

int start_server(int port);

int sendall(int sockfd, char *buf, int *len);

#endif
