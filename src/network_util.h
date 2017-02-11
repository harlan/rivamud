#ifndef _NETWORK_UTIL
#define _NETWORK_UTIL

/**
 * So this doesn't really work yet, but I'm puttng it here as a placeholder
 * Eventually this needs to just return a line up until our defined
 *
 * Not sure what to do about return values, if we're not blocking we need
 * to consider the possibility that there was no long received and need to
 * distinguish if there was a disconnect.  For now just follow standard
 * read return values.
 * delimiter (either '\n' or '\r\n')
 * @param fd File descriptor to read from.
 * @param buf Where to put what I read.
 * @param maxlen Maximum bytes to read.
 * @return Number of bytes read, 0 on disconnect, -1 on error
 */
ssize_t readline(int fd, char *buf, size_t maxlen);

/**
 * This doesn't work either.  This should do a non-blocking send and store
 * anything not sent into its own buffer so that the caller can free its
 * memory
 * @param fd File descriptor to send to.
 * @param buf Data to send.
 * @param len Length of buf
 * @return Bytes sent.
 */
ssize_t sendline(int fd, char *buf, size_t len);

#endif
