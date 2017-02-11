#ifndef _USER_H
#define _USER_H

#include <mqueue.h> // this will change to "message_queue.h" once that's finished

#define MAX_NAME_LEN 10
#define MIN_NAME_LEN 3

// TODO this needs to be though about once we rework our send/recv methods
#define MAX_BUF_SIZE 1000

typedef struct User {
  char *name;
  int   id;             // not used yet
  int   sockfd;
  mqd_t mq;
  struct User *next;
} User;

// Just use linked list for now, there are probably better ways
extern User *allUsers;
extern pthread_rwlock_t allUsersLock;


/**
 * Prompts user for login name and sets up the User info.
 * @param sockfd Socket for communicating to user.
 * @return User pointer, NULL on failure
 */
User *user_login(int sockfd);

/**
 * Creates new user and adds to master userList
 * @param name Character name, this will be duplicated into datastructure.
 *             The original storage can be freed.
 * @param sockfd Socket used for communication
 * @return User pointer, NULL on failure
 */
User *user_create(char *name, int sockfd);

/**
 * Removes user from allUsers and frees memory.  We should consider expanding
 * this to do further cleanups such as socket closes.
 * @param user The user to be deleted.
 * @return -1 on error
 */
int user_destroy(User *user);

/**
 * Handle user connection.  This is like main() for the thread.
 * @param user The user to handle
 */
int user_thread_handler(User *user);

#endif
