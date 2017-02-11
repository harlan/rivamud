#ifndef _MESSAGE_QUEUE_H
#define _MESSAGE_QUEUE_H

#include <mqueue.h>

/** 
 * Interthread communication via message queues.
 *
 * Threads will communicate with each other via message queues.  Sends
 * are guaranteed to be atomic and don't need to be locked.
 *
 * Right now, the design is that if a thread needs to send a message
 * to a different thread, you send it through the message queue.  The
 * destination thread will kick out of select and handle the sending
 * over the network.
 *
 * We're using these queues in select calls in the user interface methods.
 * This is linux specific and not portable.
 *
 * TODO
 * We need to figure out proper msgsizes and queue sizes.
 *
 */

#define RM_SCOPE_ALL  0
#define RM_SCOPE_AREA 1
#define RM_SCOPE_ROOM 2

/** 
 * Send message to multiple destinations.
 * @param sender Who is sending message, ignored if echo is set to 1.
 * @param mq_scope Define the scope; who will get the message ?
 * @param echo if non-zero,
 *             the sender will also receive message if within scope.
 * @param msg_ptr The message, passed to mq functions.
 * @param msg_len The message length, passed to mq functions.
 * @param msg_prio The message priority, passed to mq functions.
 * @return 0 on success, -1 on failure.
 */
int mq_broadcast(mqd_t sender, int mq_scope, int echo,
                 const char *msg_ptr, size_t msg_len, unsigned int msg_prio);

/**
 * Send message to single destination
 * @param receiver To whom you want to send the message
 * @param msg_ptr The message, passed to mq functions.
 * @param msg_len The message length, passed to mq functions.
 * @param msg_prio The message priority, passed to mq functions.
 * @return 0 on success, -1 on failure.
 */
int mq_direct(mqd_t receiver,
              const char *msg_ptr, size_t msg_len, unsigned int msg_prio);

/**
 * Create new message queue for read and write
 * @param sockfd Socket file descriptor.  Queues need unique names so we're
 *               basing it off the socket descriptor.
 * @return Opened message queue.
 */
mqd_t mq_create(int sockfd);

/**
 * Close and unlink message queue.
 * @param mq The message queue to destroy.
 */
void mq_destroy(mqd_t mq);

#endif
