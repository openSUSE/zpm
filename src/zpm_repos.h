
#ifndef ZPM_REPOS_H
#define ZPM_REPOS_H

#include <sys/stat.h>
extern "C" {
#include <solv/pool.h>
#include <solv/repo.h>
}
#include "zpm_repoinfo.h"
#include "zpm_checksum.h"


int load_stub(Pool *pool, Repodata *data, void *dp);
Id nscallback(Pool *pool, void *data, Id name, Id evr);

void read_repos(Pool *pool, zpm_repoinfo *repoinfos, int nrepoinfos);
int usecachedrepo(Repo *repo, const char *repoext, unsigned char *cookie, int mark);
void writecachedrepo(Repo *repo, Repodata *info, const char *repoext, unsigned char *cookie);

#endif
