#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include "messaging.h"
#include "user.h"

int msg_direct(User *receiver, const char *msg_ptr, size_t msg_len) {

  // this will hang if the pipe is full
  // TODO consider sentting pipes to nonblock
  return write(receiver->pfds[1], msg_ptr, msg_len);
}

int msg_broadcast(User *sender, int mq_scope, int echo,
                 const char *msg_ptr, size_t msg_len) {

  pthread_rwlock_rdlock(&allUsersLock);

  for (User *user = allUsers; user != NULL; user = user->next) {
    if (!echo && user == sender)
      continue;

    // pretty much ignore failures, could log possibly ?
    msg_direct(user, msg_ptr, msg_len);
  }
  pthread_rwlock_unlock(&allUsersLock);

  return 1;
}

int msg_create(int pfds[]) {
  if (pipe(pfds) == -1) {
    perror("pipe");
    return -1;
  }
  return 1;
}

void msg_destroy(int pfds[]) {
  close(pfds[0]);
  close(pfds[1]);
}
