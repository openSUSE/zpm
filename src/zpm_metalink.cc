
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <poll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <string.h>

extern "C" {
#include <solv/util.h>
#include <solv/chksum.h>
}

#include "zpm_metalink.h"

void
findfastest(char **urls, int nurls)
{
  int i, j, port;
  int *socks, qc;
  struct pollfd *fds;
  char *p, *p2, *q;
  char portstr[16];
  struct addrinfo hints, *result;;

  fds = (struct pollfd *) solv_calloc(nurls, sizeof(*fds));
  socks = (int *) solv_calloc(nurls, sizeof(*socks));

  for (i = 0; i < nurls; i++)
    {
      socks[i] = -1;
      p = strchr(urls[i], '/');
      if (!p)
	continue;
      if (p[1] != '/')
	continue;
      p += 2;
      q = strchr(p, '/');
      qc = 0;
      if (q)
	{
	  qc = *q;
	  *q = 0;
	}
      if ((p2 = strchr(p, '@')) != 0)
	p = p2 + 1;
      port = 80;
      if (!strncmp("https:", urls[i], 6))
	port = 443;
      else if (!strncmp("ftp:", urls[i], 4))
	port = 21;
      if ((p2 = strrchr(p, ':')) != 0)
	{
	  port = atoi(p2 + 1);
	  if (q)
	    *q = qc;
	  q = p2;
	  qc = *q;
	  *q = 0;
	}
      sprintf(portstr, "%d", port);
      memset(&hints, 0, sizeof(struct addrinfo));
      hints.ai_family = AF_UNSPEC;
      hints.ai_socktype = SOCK_STREAM;
      hints.ai_flags = AI_NUMERICSERV;
      result = 0;
      if (!getaddrinfo(p, portstr, &hints, &result))
	{
	  socks[i] = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
	  if (socks[i] >= 0)
	    {
	      fcntl(socks[i], F_SETFL, O_NONBLOCK);
	      if (connect(socks[i], result->ai_addr, result->ai_addrlen) == -1)
		{
		  if (errno != EINPROGRESS)
		    {
		      close(socks[i]);
		      socks[i] = -1;
		    }
		}
	    }
	  freeaddrinfo(result);
	}
      if (q)
	*q = qc;
    }
  for (;;)
    {
      for (i = j = 0; i < nurls; i++)
	{
	  if (socks[i] < 0)
	    continue;
	  fds[j].fd = socks[i];
	  fds[j].events = POLLOUT;
	  j++;
	}
      if (j < 2)
	{
	  i = j - 1;
	  break;
	}
      if (poll(fds, j, 10000) <= 0)
	{
	  i = -1;	/* something is wrong */
	  break;
	}
      for (i = 0; i < j; i++)
	if ((fds[i].revents & POLLOUT) != 0)
	  {
	    int soe = 0;
	    socklen_t soel = sizeof(int);
	    if (getsockopt(fds[i].fd, SOL_SOCKET, SO_ERROR, &soe, &soel) == -1 || soe != 0)
	      {
	        /* connect failed, kill socket */
	        for (j = 0; j < nurls; j++)
		  if (socks[j] == fds[i].fd)
		    {
		      close(socks[j]);
		      socks[j] = -1;
		    }
		i = j + 1;
		break;
	      }
	    break;	/* horray! */
	  }
      if (i == j + 1)
	continue;
      if (i == j)
        i = -1;		/* something is wrong, no bit was set */
      break;
    }
  /* now i contains the fastest fd index */
  if (i >= 0)
    {
      for (j = 0; j < nurls; j++)
	if (socks[j] == fds[i].fd)
	  break;
      if (j != 0)
	{
	  char *url0 = urls[0];
	  urls[0] = urls[j];
	  urls[j] = url0;
	}
    }
  for (i = j = 0; i < nurls; i++)
    if (socks[i] >= 0)
      close(socks[i]);
  free(socks);
  free(fds);
}

char *
findmetalinkurl(FILE *fp, unsigned char *chksump, Id *chksumtypep)
{
  char buf[4096], *bp, *ep;
  char **urls = 0;
  int nurls = 0;
  int i;

  if (chksumtypep)
    *chksumtypep = 0;
  while((bp = fgets(buf, sizeof(buf), fp)) != 0)
    {
      while (*bp == ' ' || *bp == '\t')
	bp++;
      if (chksumtypep && !*chksumtypep && !strncmp(bp, "<hash type=\"sha256\">", 20))
	{
	  bp += 20;
	  if (solv_hex2bin((const char **)&bp, chksump, 32) == 32)
	    *chksumtypep = REPOKEY_TYPE_SHA256;
	  continue;
	}
      if (strncmp(bp, "<url", 4))
	continue;
      bp = strchr(bp, '>');
      if (!bp)
	continue;
      bp++;
      ep = strstr(bp, "repodata/repomd.xml</url>");
      if (!ep)
	continue;
      *ep = 0;
      if (strncmp(bp, "http", 4))
	continue;
      urls = (char **) solv_extend(urls, nurls, 1, sizeof(*urls), 15);
      urls[nurls++] = strdup(bp);
    }
  if (nurls)
    {
      if (nurls > 1)
        findfastest(urls, nurls > 5 ? 5 : nurls);
      bp = urls[0];
      urls[0] = 0;
      for (i = 0; i < nurls; i++)
        solv_free(urls[i]);
      solv_free(urls);
      ep = strchr(bp, '/');
      if ((ep = strchr(ep + 2, '/')) != 0)
	{
	  *ep = 0;
	  printf("[using mirror %s]\n", bp);
	  *ep = '/';
	}
      return bp;
    }
  return 0;
}

char *
findmirrorlisturl(FILE *fp)
{
  char buf[4096], *bp, *ep;
  int i, l;
  char **urls = 0;
  int nurls = 0;

  while((bp = fgets(buf, sizeof(buf), fp)) != 0)
    {
      while (*bp == ' ' || *bp == '\t')
	bp++;
      if (!*bp || *bp == '#')
	continue;
      l = strlen(bp);
      while (l > 0 && (bp[l - 1] == ' ' || bp[l - 1] == '\t' || bp[l - 1] == '\n'))
	bp[--l] = 0;
      urls = (char **) solv_extend(urls, nurls, 1, sizeof(*urls), 15);
      urls[nurls++] = strdup(bp);
    }
  if (nurls)
    {
      if (nurls > 1)
        findfastest(urls, nurls > 5 ? 5 : nurls);
      bp = urls[0];
      urls[0] = 0;
      for (i = 0; i < nurls; i++)
        solv_free(urls[i]);
      solv_free(urls);
      ep = strchr(bp, '/');
      if ((ep = strchr(ep + 2, '/')) != 0)
	{
	  *ep = 0;
	  printf("[using mirror %s]\n", bp);
	  *ep = '/';
	}
      return bp;
    }
  return 0;
}
