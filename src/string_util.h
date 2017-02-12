#ifndef _STRING_UTIL_H
#define _STRING_UTIL_H

/**
 * Trim non alphanumeric characters from beginning and end of string.
 * This function is desctructive, it will modify the original string.
 * @param str The string to trim.
 * @result Pointer to start of new string.  If the original string contains
 *         leading spaces or control characters, the returned pointer will
 *         not equal str.
 */
char *trim(char *str);

#endif
