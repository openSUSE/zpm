
#include <errno.h>
#include <stdlib.h>
#include <sys/utsname.h>

extern "C" {
#include <solv/pool.h>
#include <solv/poolid.h>
#include <solv/poolarch.h>
}

#include "zpm.h"
#include "zpm_solvable_iter.h"
#include "zpm_private.h"

#include "zpm_repos.h"
#include "zpm_repoinfo.h"

static void
setarch(Pool *pool)
{
  struct utsname un;
  if (uname(&un))
    {
      perror("uname");
      exit(1);
    }
  pool_setarch(pool, un.machine);
}

zpm * zpm_create(const char *root) 
{
    zpm *z = new zpm;

    z->commandlinerepo = 0;
    z->commandlinepkgs = 0;
    z->nrepoinfos = 0;
    z->mainmode = 0;
    z->mode = 0;
    z->solv = 0;
    z->allpkgs = 0;
    z->addedfileprovides = 0;
    z->repofilter = 0;

    z->pool = pool_create();

    pool_setloadcallback(z->pool, load_stub, 0);
    z->pool->nscallback = nscallback;
    // pool_setdebuglevel(pool, 2);
    setarch(z->pool);
    return z;    
}

# define REPOINFO_PATH "/etc/zypp/repos.d"
void zpm_read_repos(zpm *z)
{
    z->repoinfos = zpm_read_repoinfos(z->pool, REPOINFO_PATH, &z->nrepoinfos);
    read_repos(z->pool, z->repoinfos, z->nrepoinfos);
    pool_createwhatprovides(z->pool);
}

void zpm_easy_init(zpm *z)
{
    zpm_read_repos(z);
}

void zpm_solvable_destroy(zpm_solvable *s)
{    
}

const char * zpm_solvable_name(zpm_solvable *s)
{
    return pool_id2str(s->repo->pool, s->name);
}

const char * zpm_solvable_version(zpm_solvable *s)
{
    return pool_id2str(s->repo->pool, s->evr);
}

void zpm_destroy(zpm *z)
{
    delete z;    
}

