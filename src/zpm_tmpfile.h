
#ifndef ZPM_TMPFILE_H
#define ZPM_TMPFILE_H

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static inline int
opentmpfile()
{
  char tmpl[100];
  int fd;

  strcpy(tmpl, "/var/tmp/solvXXXXXX");
  fd = mkstemp(tmpl);
  if (fd < 0)
    {
      perror("mkstemp");
      exit(1);
    }
  unlink(tmpl);
  return fd;
}

#endif
