#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <ctype.h>
#include <fcntl.h>
#include <sys/stat.h>
#include "user.h"
#include "network_listener.h"
#include "message_queue.h"


static int user_add_allUsers(User *user);
static char *trim(char *str);

User *allUsers = NULL;
pthread_rwlock_t allUsersLock = PTHREAD_RWLOCK_INITIALIZER;

static char *trim(char *str) {
  char *start, *end;

  start = str;
  while (!isalnum(*start)) {
    if (*start == '\0')
      return start;
    start++;
  }

  end = start + strlen(start) - 1;
  while (end > start && !isalnum(*end)) end--;
  end++;
  end = '\0';

  return start;
}

User *user_login(int sockfd) {
  char recvBuf[256];    // this is random, the bufsize should be set once we
                        // encapsulate recv calls
  char sendBuf[32];
  int sendlen;
  char *name;
  int nbytes;
  int namelen;
  User *me;

  while (1) {
    sprintf(sendBuf, "Login: ");
    sendlen = strlen(sendBuf) + 1;
    sendall(sockfd, sendBuf, &sendlen);
    // TODO wrap recv calls
    nbytes = recv(sockfd, recvBuf, sizeof recvBuf, 0);
    if (nbytes < 1) {
      perror("recv");
      return NULL;
    }
    if (nbytes == 0)
      return NULL;

    recvBuf[MAX_NAME_LEN] = '\0';
    name = trim(recvBuf);
    namelen = strlen(name);
    if (namelen < MIN_NAME_LEN || namelen > MAX_NAME_LEN)
      continue;

    break;
    // TODO do some more name verification, keep asking for login until
    // we find a suitable name
  }
  me = user_create(name, sockfd);
  return me;
}

User *user_create(char *name, int sockfd) {
  User *newUser;

  // TODO encapsulate malloc calls
  if ((newUser = malloc(sizeof(User))) == NULL)
    return NULL;

  newUser->sockfd = sockfd;
  newUser->name   = strdup(name);
  newUser->mq     = mq_create(sockfd);

  if (newUser->mq < 0)
    return NULL;

  user_add_allUsers(newUser);

  return newUser;
}


static int user_add_allUsers(User *user) {
  // TODO test returns on locking calls.. once implemented, remember to
  // change ALL calls !
  pthread_rwlock_wrlock(&allUsersLock);
  user->next = allUsers;
  allUsers = user;
  pthread_rwlock_unlock(&allUsersLock);

  return 1;
}


int user_destroy(User *user) {
  User dummy;
  User *currentUser;
  User *prev;
  char mqname[255]; // shouldnt' be more than like 10 chars

  pthread_rwlock_wrlock(&allUsersLock);

  dummy.next    = allUsers;
  prev          = &dummy;
  currentUser   = allUsers;

  // find
  while (currentUser != NULL && currentUser != user) {
    prev = prev->next;
    currentUser = currentUser->next;
  }
  if (currentUser == NULL) // lol wut
    return -1;

  prev = currentUser->next;
  pthread_rwlock_unlock(&allUsersLock);

  snprintf(mqname, sizeof mqname, "/%d", currentUser->sockfd);
  mq_close(currentUser->mq);
  mq_unlink(mqname);
  free(currentUser->name);
  free(currentUser);

  return 1;
}

int user_thread_handler(User *user) {
  int sockfd = user->sockfd;
  mqd_t mq = user->mq;
  int fdmax;
  fd_set m_readfds, m_writefds, readfds, writefds;
  int recvbytes;
  char recvbuf[MAX_BUF_SIZE];
  int sendbytes; // not used yet
  char sendbuf[MAX_BUF_SIZE]; // not used yet
  char mqbuf[MAX_BUF_SIZE];
  char *trimmed;
  int mqlen;
  int nready;
  int closed_connection = 0;

  FD_ZERO(&m_readfds);
  FD_ZERO(&m_writefds);
  FD_ZERO(&readfds);
  FD_ZERO(&writefds);
  FD_SET(sockfd, &m_readfds);
  FD_SET(mq, &m_readfds);
  fdmax = sockfd > mq ? sockfd : mq;

  while (1) {
    readfds = m_readfds;
    writefds = m_writefds;
    if ((nready = select(fdmax+1, &readfds, NULL, NULL, NULL)) < 1) {
      perror("select");
      break;
    }

    for (; nready > 0; nready--) {
      if (FD_ISSET(sockfd, &readfds)) {
        recvbytes = recv(sockfd, recvbuf, (sizeof recvbuf) - 1, 0);
        if (recvbytes < 0) {
          perror("recv");
          break;
        }
        if (recvbytes == 0) {
          closed_connection = 1;
          break;
        }

        printf("%d received\n", recvbytes);
        // TODO this is where we need proper command parsing and verification, 
        // for now, we're just a chat server
        recvbuf[recvbytes] = '\0';
        trimmed = trim(recvbuf);
        if (strlen(trimmed) > 0) {
          mqlen = snprintf(mqbuf, sizeof mqbuf, "%s says, %s\r\n",
                           user->name, trimmed);
          mq_broadcast(mq, RM_SCOPE_ALL, 0, mqbuf, mqlen+1, 0);
        }

        continue;
      }
      if (FD_ISSET(sockfd, &writefds)) {
        // handle socket writes
        continue;
      }
      if (FD_ISSET(mq, &readfds)) {
        // handle mq
        mqlen = mq_receive(mq, mqbuf, sizeof mqbuf, NULL);
        printf("%d bytes in mq\n", mqlen);
        mqlen--; // don't need to send \0
        sendall(sockfd, mqbuf, &mqlen);

        continue;
      }
    }
    if (closed_connection)
      break;
  }

  user_destroy(user);

  return 1;
}
