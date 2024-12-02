/*
**  stptok() -- public domain by Ray Gardner, modified by Bob Stout
**
**   You pass this function a string to parse, a buffer to receive the
**   "token" that gets scanned, the length of the buffer, and a string of
**   "break" characters that stop the scan.  It will copy the string into
**   the buffer up to any of the break characters, or until the buffer is
**   full, and will always leave the buffer null-terminated.  It will
**   return a pointer to the first non-breaking character after the one
**   that stopped the scan.
*/

#include "MantidNexusCpp/nx_stptok.h"
#include <stdlib.h>
#include <string.h>

char *stptok(const char *s, char *tok, size_t toklen, char *brk) {
  char *lim, *b;

  if (!*s)
    return NULL;

  lim = tok + toklen - 1;
  while (*s && tok < lim) {
    for (b = brk; *b; b++) {
      if (*s == *b) {
        *tok = 0;
        return (char *)(s + 1);
      }
    }
    *tok++ = *s++;
  }
  *tok = 0;
  return (char *)s;
}
