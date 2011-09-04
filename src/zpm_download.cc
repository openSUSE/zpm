
#include <fcntl.h>
#include <stdio.h>
#include <sys/wait.h>

extern "C" {
#include <solv/solvable.h>
#include <solv/solv_xfopen.h>
}

#include "zpm_checksum.h"
#include "zpm_download.h"
#include "zpm_metalink.h"
#include "zpm_repoinfo.h"
#include "zpm_tmpfile.h"

FILE * curlfopen(zpm_repoinfo *cinfo, const char *file, int uncompress, const unsigned char *chksum, Id chksumtype, int *badchecksump)
{
  pid_t pid;
  int fd, l;
  int status;
  char url[4096];
  const char *baseurl = cinfo->baseurl;

  if (!baseurl)
    {
      if (!cinfo->metalink && !cinfo->mirrorlist)
        return 0;
      if (file != cinfo->metalink && file != cinfo->mirrorlist)
	{
	  FILE *fp = curlfopen(cinfo, cinfo->metalink ? cinfo->metalink : cinfo->mirrorlist, 0, 0, 0, 0);
	  unsigned char mlchksum[32];
	  Id mlchksumtype = 0;
	  if (!fp)
	    return 0;
	  if (cinfo->metalink)
	    cinfo->baseurl = findmetalinkurl(fp, mlchksum, &mlchksumtype);
	  else
	    cinfo->baseurl = findmirrorlisturl(fp);
	  fclose(fp);
	  if (!cinfo->baseurl)
	    return 0;
#ifdef FEDORA
	  if (strchr(cinfo->baseurl, '$'))
	    {
	      char *b = yum_substitute(cinfo->repo->pool, cinfo->baseurl);
	      free(cinfo->baseurl);
	      cinfo->baseurl = strdup(b);
	    }
#endif
	  if (!chksumtype && mlchksumtype && !strcmp(file, "repodata/repomd.xml"))
	    {
	      chksumtype = mlchksumtype;
	      chksum = mlchksum;
	    }
	  return curlfopen(cinfo, file, uncompress, chksum, chksumtype, badchecksump);
	}
      snprintf(url, sizeof(url), "%s", file);
    }
  else
    {
      l = strlen(baseurl);
      if (l && baseurl[l - 1] == '/')
	snprintf(url, sizeof(url), "%s%s", baseurl, file);
      else
	snprintf(url, sizeof(url), "%s/%s", baseurl, file);
    }
  fd = opentmpfile();
  // printf("url: %s\n", url);
  if ((pid = fork()) == (pid_t)-1)
    {
      perror("fork");
      exit(1);
    }
  if (pid == 0)
    {
      if (fd != 1)
	{
          dup2(fd, 1);
	  close(fd);
	}
      execlp("curl", "curl", "-f", "-s", "-L", url, (char *)0);
      perror("curl");
      _exit(0);
    }
  status = 0;
  while (waitpid(pid, &status, 0) != pid)
    ;
  if (lseek(fd, 0, SEEK_END) == 0 && (!status || !chksumtype))
    {
      /* empty file */
      close(fd);
      return 0;
    }
  lseek(fd, 0, SEEK_SET);
  if (status)
    {
      printf("%s: download error %d\n", file, status >> 8 ? status >> 8 : status);
      if (badchecksump)
	*badchecksump = 1;
      close(fd);
      return 0;
    }
  if (chksumtype && !zpm_verify_checksum(fd, file, chksum, chksumtype))
    {
      if (badchecksump)
	*badchecksump = 1;
      close(fd);
      return 0;
    }
  if (uncompress)
    return solv_xfopen_fd(".gz", fd, "r");
  fcntl(fd, F_SETFD, FD_CLOEXEC);
  return fdopen(fd, "r");
}
