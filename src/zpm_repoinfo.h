
#ifndef ZPM_REPOINFO_H
#define ZPM_REPOINFO_H

extern "C" {
#include <solv/pool.h>
#include <solv/repo.h>
}

#define TYPE_UNKNOWN	0
#define TYPE_SUSETAGS	1
#define TYPE_RPMMD	2
#define TYPE_PLAINDIR	3
#define TYPE_DEBIAN     4

typedef struct _zpm_repoinfo {
  Repo *repo;

  char *alias;
  char *name;
  int enabled;
  int autorefresh;
  char *baseurl;
  char *metalink;
  char *mirrorlist;
  char *path;
  int type;
  int pkgs_gpgcheck;
  int repo_gpgcheck;
  int priority;
  int keeppackages;
  int metadata_expire;
  char **components;
  int ncomponents;

  unsigned char cookie[32];
  unsigned char extcookie[32];
} zpm_repoinfo;

zpm_repoinfo * zpm_read_repoinfos(Pool *pool, const char *reposdir, int *nrepoinfosp);

#endif
