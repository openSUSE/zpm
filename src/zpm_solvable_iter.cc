
#include "zpm_solvable_iter.h"

zpm_solvable * zpm_solvable_iter_next(zpm_solvable_iter *it)
{
    return it->next();
}

void zpm_solvable_iter_destroy(zpm_solvable_iter *iter)
{
    delete iter;    
}
