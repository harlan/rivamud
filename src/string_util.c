#include <ctype.h>

char *trim(char *str) {
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

