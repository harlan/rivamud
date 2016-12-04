#include <stdio.h>
#include "network_listener.h"

int main(int argc, char **argv) {
  int port = 8888;
  start_server(port);
  return 0;
}

