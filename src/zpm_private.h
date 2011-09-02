
#ifndef ZPM_TYPES_PRIVATE_H
#define ZPM_TYPES_PRIVATE_H

#include <solv/repo.h>
#include <solv/solvable.h>
#include <solv/solver.h>
#include <solv/queue.h>
#include <solv/transaction.h>

#include "zpm_repoinfo.h"

struct _zpm {
    Pool *pool;
    Repo *commandlinerepo;
    Id *commandlinepkgs;
    Id p;
    Id pp;
    zpm_repoinfo *repoinfos;
    int nrepoinfos;
    int mainmode;
    int mode;
    int i, newpkgs;
    Queue job, checkq;
    Solver *solv;
    Transaction *trans;
    char inbuf[128];
    char *ip;
    int allpkgs;
    FILE **newpkgsfps;
    Id *addedfileprovides;
    Id repofilter;
};

struct _zpm_solvable : public Solvable
{
    _zpm_solvable(Solvable *s)
        : Solvable(*s)
    {}
};
//typedef Solvable _zpm_solvable;

#endif
