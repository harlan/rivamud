#ifndef _MESSAGING_H
#define _MESSAGING_H

#include "user.h"

/** 
 * Interthread communication via message queues.
 *
 * Threads will communicate with each other via pipes.  Sends under
 * PIPE_BUF * are guaranteed to be atomic and don't need to be locked.
 *
 * PIPE_BUF is 512 under osx and 4096 under linux.
 *
 * Right now, the design is that if a thread needs to send a message
 * to a different thread, you send it through the pipe.  The
 * destination thread will kick out of select and handle the sending
 * over the network.
 *
 */

#define RM_SCOPE_ALL  0
#define RM_SCOPE_AREA 1
#define RM_SCOPE_ROOM 2

/** 
 * Send message to multiple destinations.
 * @param sender Who is sending message, ignored if echo is set to 1.
 * @param msg_scope Define the scope; who will get the message ?
 * @param echo if non-zero,
 *             the sender will also receive message if within scope.
 * @param msg_ptr The message.
 * @param msg_len The message length.
 * @return 1 on success, -1 on failure.
 */
int msg_broadcast(User *sender, int msg_scope, int echo,
                 const char *msg_ptr, size_t msg_len);

/**
 * Send message to single destination
 * @param receiver To whom you want to send the message
 * @param msg_ptr The message.
 * @param msg_len The message length.
 * @return bytes written, -1 on failure
 */
int msg_direct(User *receiver, const char *msg_ptr, size_t msg_len);

/**
 * Create new message queue for read and write
 * @param sockfd Socket file descriptor.  Queues need unique names so we're
 *               basing it off the socket descriptor.
 * @return 1 on success, -1 on failure.
 */
int msg_create(int pfds[]);

/**
 * Close pipes
 * @param pfds The pipe array to close.
 */
void msg_destroy(int pfds[]);

#endif
