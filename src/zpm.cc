
#include "zpm.h"
#include "zpm_solvable_iter.h"
#include "zpm_types_private.h"

#include <zypp/ZYppFactory.h>
#include <zypp/ZYpp.h>
#include <zypp/sat/Solvable.h>

using namespace zypp;
using namespace zypp::sat;


zpm * zpm_create(const char *root) 
{
    zpm *z = new zpm;
    z->zypp = getZYpp();

    z->zypp->initializeTarget(root);
    z->zypp->target()->load();    
    return z;
}


zpm_solvable * zpm_solvable_iter_next(zpm_solvable_iter *it)
{
    return it->next();
}

void zpm_solvable_destroy(zpm_solvable *s)
{
    delete s;    
}

const char * zpm_solvable_name(zpm_solvable *s)
{
    return s->name().c_str();    
}

const char * zpm_solvable_version(zpm_solvable *s)
{
    return s->edition().c_str();
}


void zpm_destroy(zpm *z)
{
    delete z;    
}

