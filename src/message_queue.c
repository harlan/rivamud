#include <stdio.h>
#include <pthread.h>
#include "message_queue.h"
#include "user.h"

int mq_direct(mqd_t receiver,
              const char *msg_ptr, size_t msg_len, unsigned int msg_prio) {
  int rv;

  // this will hang if the queue is full
  // TODO when creating message queues, we should be setting O_NONBLOCK,
  // this will set EAGAIN and return -1 if the queue is full.  not much
  // we can do at that point, message will be lost
  rv = mq_send(receiver, msg_ptr, msg_len, msg_prio);

  return rv;
}

int mq_broadcast(mqd_t sender, int mq_scope, int echo,
                 const char *msg_ptr, size_t msg_len, unsigned int msg_prio) {
  mqd_t dest;
  User *user;

  pthread_rwlock_rdlock(&allUsersLock);

  for (user = allUsers; user != NULL; user = user->next) {
    dest = user->mq;
    if (!echo && dest == sender)
      continue;

    // pretty much ignore failures, could log possibly ?
    mq_direct(dest, msg_ptr, msg_len, msg_prio);
  }
  pthread_rwlock_unlock(&allUsersLock);

  return 0;
}

mqd_t mq_create(int sockfd) {
  char mqname[7]; // file descriptors are ints so we shouldn't need more than
                  // 5 chars (65535) + '/' + termination, unlikely we'll get
                  // over 3 chars for a fd
  struct mq_attr attr;
  int mq_msgsize = 255; // TODO figure out a proper message size here
  int mq_qsize   = 10;  // TODO figure out a proper queue size here
  mqd_t mq;

  attr.mq_flags   = 0;
  attr.mq_maxmsg  = mq_qsize;
  attr.mq_msgsize = mq_msgsize;
  attr.mq_curmsgs = 0;

  snprintf(mqname, sizeof mqname, "/%d", sockfd);

  mq = mq_open(mqname, O_RDWR | O_CREAT, 0644, &attr);
  if (mq < 0)
    perror("mq_open");

  return mq;
}
