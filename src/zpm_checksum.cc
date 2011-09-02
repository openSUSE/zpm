#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>

extern "C" {
#include <solv/chksum.h>
}

#include "zpm_checksum.h"

int
verify_checksum(int fd, const char *file, const unsigned char *chksum, Id chksumtype)
{
  char buf[1024];
  const unsigned char *sum;
  void *h;
  int l;

  h = solv_chksum_create(chksumtype);
  if (!h)
    {
      printf("%s: unknown checksum type\n", file);
      return 0;
    }
  while ((l = read(fd, buf, sizeof(buf))) > 0)
    solv_chksum_add(h, buf, l);
  lseek(fd, 0, SEEK_SET);
  l = 0;
  sum = solv_chksum_get(h, &l);
  if (memcmp(sum, chksum, l))
    {
      printf("%s: checksum mismatch\n", file);
      solv_chksum_free(h, 0);
      return 0;
    }
  solv_chksum_free(h, 0);
  return 1;
}

#ifndef DEBIAN

static void
cleanupgpg(char *gpgdir)
{
  char cmd[256];
  snprintf(cmd, sizeof(cmd), "%s/pubring.gpg", gpgdir);
  unlink(cmd);
  snprintf(cmd, sizeof(cmd), "%s/pubring.gpg~", gpgdir);
  unlink(cmd);
  snprintf(cmd, sizeof(cmd), "%s/secring.gpg", gpgdir);
  unlink(cmd);
  snprintf(cmd, sizeof(cmd), "%s/trustdb.gpg", gpgdir);
  unlink(cmd);
  snprintf(cmd, sizeof(cmd), "%s/keys", gpgdir);
  unlink(cmd);
  rmdir(gpgdir);
}

int
checksig(Pool *sigpool, FILE *fp, FILE *sigfp)
{
  char *gpgdir;
  char *keysfile;
  const char *pubkey;
  char cmd[256];
  FILE *kfp;
  Solvable *s;
  Id p;
  off_t posfp, possigfp;
  int r, nkeys;

  gpgdir = mkdtemp(pool_tmpjoin(sigpool, "/var/tmp/solvgpg.XXXXXX", 0, 0));
  if (!gpgdir)
    return 0;
  keysfile = pool_tmpjoin(sigpool, gpgdir, "/keys", 0);
  if (!(kfp = fopen(keysfile, "w")) )
    {
      cleanupgpg(gpgdir);
      return 0;
    }
  nkeys = 0;
  for (p = 1, s = sigpool->solvables + p; p < sigpool->nsolvables; p++, s++)
    {
      if (!s->repo)
	continue;
      pubkey = solvable_lookup_str(s, SOLVABLE_DESCRIPTION);
      if (!pubkey || !*pubkey)
	continue;
      if (fwrite(pubkey, strlen(pubkey), 1, kfp) != 1)
	break;
      if (fputc('\n', kfp) == EOF)	/* Just in case... */
	break;
      nkeys++;
    }
  if (fclose(kfp) || !nkeys)
    {
      cleanupgpg(gpgdir);
      return 0;
    }
  snprintf(cmd, sizeof(cmd), "gpg2 -q --homedir %s --import %s", gpgdir, keysfile);
  if (system(cmd))
    {
      fprintf(stderr, "key import error\n");
      cleanupgpg(gpgdir);
      return 0;
    }
  unlink(keysfile);
  posfp = lseek(fileno(fp), 0, SEEK_CUR);
  lseek(fileno(fp), 0, SEEK_SET);
  possigfp = lseek(fileno(sigfp), 0, SEEK_CUR);
  lseek(fileno(sigfp), 0, SEEK_SET);
  snprintf(cmd, sizeof(cmd), "gpg -q --homedir %s --verify /dev/fd/%d /dev/fd/%d >/dev/null 2>&1", gpgdir, fileno(sigfp), fileno(fp));
  fcntl(fileno(fp), F_SETFD, 0);	/* clear CLOEXEC */
  fcntl(fileno(sigfp), F_SETFD, 0);	/* clear CLOEXEC */
  r = system(cmd);
  lseek(fileno(sigfp), possigfp, SEEK_SET);
  lseek(fileno(fp), posfp, SEEK_SET);
  fcntl(fileno(fp), F_SETFD, FD_CLOEXEC);
  fcntl(fileno(sigfp), F_SETFD, FD_CLOEXEC);
  cleanupgpg(gpgdir);
  return r == 0 ? 1 : 0;
}

#else

int
checksig(Pool *sigpool, FILE *fp, FILE *sigfp)
{
  char cmd[256];
  int r;

  snprintf(cmd, sizeof(cmd), "gpgv -q --keyring /etc/apt/trusted.gpg /dev/fd/%d /dev/fd/%d >/dev/null 2>&1", fileno(sigfp), fileno(fp));
  fcntl(fileno(fp), F_SETFD, 0);	/* clear CLOEXEC */
  fcntl(fileno(sigfp), F_SETFD, 0);	/* clear CLOEXEC */
  r = system(cmd);
  fcntl(fileno(fp), F_SETFD, FD_CLOEXEC);
  fcntl(fileno(sigfp), F_SETFD, FD_CLOEXEC);
  return r == 0 ? 1 : 0;
}

#endif

#define CHKSUM_IDENT "1.1"

void
calc_checksum_fp(FILE *fp, Id chktype, unsigned char *out)
{
  char buf[4096];
  void *h = solv_chksum_create(chktype);
  int l;

  solv_chksum_add(h, CHKSUM_IDENT, strlen(CHKSUM_IDENT));
  while ((l = fread(buf, 1, sizeof(buf), fp)) > 0)
    solv_chksum_add(h, buf, l);
  rewind(fp);
  solv_chksum_free(h, out);
}

void
calc_checksum_stat(struct stat *stb, Id chktype, unsigned char *out)
{
  void *h = solv_chksum_create(chktype);
  solv_chksum_add(h, CHKSUM_IDENT, strlen(CHKSUM_IDENT));
  solv_chksum_add(h, &stb->st_dev, sizeof(stb->st_dev));
  solv_chksum_add(h, &stb->st_ino, sizeof(stb->st_ino));
  solv_chksum_add(h, &stb->st_size, sizeof(stb->st_size));
  solv_chksum_add(h, &stb->st_mtime, sizeof(stb->st_mtime));
  solv_chksum_free(h, out);
}
