#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <ctype.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/select.h>
#include "user.h"
#include "network_listener.h"
#include "messaging.h"


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
  *end = '\0';

  return start;
}

// this just normalizes, it doesn't validate
static char *normalize_name(char *name) {
  name = trim(name);
  len = strlen(name);

  name[0] = toupper(name[0]);
  for (int i = 1; i < len; i++)
    name[i] = tolower(name[i]);

  return name;
}

User *user_login(int sockfd) {
  char recvBuf[256];    // this is random, the bufsize should be set once we
                        // encapsulate recv calls
  char sendBuf[32];
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
    name = normalize_name(recvBuf);
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
  if (msg_create(newUser->pfds) == -1)
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

  msg_destroy(currentUser->pfds);
  free(currentUser->name);
  free(currentUser);

  return 1;
}

int user_thread_handler(User *me) {
  int sockfd = me->sockfd;
  int fdmax;
  fd_set m_readfds, m_writefds, readfds, writefds;
  int recvbytes;
  char recvbuf[MAX_BUF_SIZE];
  int sendbytes; // not used yet
  char sendbuf[MAX_BUF_SIZE]; // not used yet
  char msgbuf[MAX_BUF_SIZE];
  char *trimmed;
  int msglen;
  int nready;
  int closed_connection = 0;

  FD_ZERO(&m_readfds);
  FD_ZERO(&m_writefds);
  FD_ZERO(&readfds);
  FD_ZERO(&writefds);
  FD_SET(sockfd, &m_readfds);
  FD_SET(me->pfds[0], &m_readfds);
  fdmax = sockfd > me->pfds[0] ? sockfd : me->pfds[0];

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
          msglen = snprintf(msgbuf, sizeof msgbuf, "%s says, %s\r\n",
                            me->name, trimmed);
          msg_broadcast(me, RM_SCOPE_ALL, 0, msgbuf, msglen+1);
        }

        continue;
      }
      if (FD_ISSET(sockfd, &writefds)) {
        // handle socket writes
        continue;
      }
      if (FD_ISSET(me->pfds[0], &readfds)) {
        // handle pipe
        msglen = read(me->pfds[0], msgbuf, sizeof msgbuf);
        printf("%d bytes in pipe\n", msglen);
        sendall(sockfd, msgbuf, &msglen);

        continue;
      }
    }
    if (closed_connection)
      break;
  }

  user_destroy(me);

  return 1;
}
