
#ifndef _zpm_solvable_iter_H
#define _zpm_solvable_iter_H

extern "C" {
#include <solv/pool.h>
}

#include "zpm.h"

class _zpm_solvable_iter
{
public:
    virtual zpm_solvable * next() = 0;
    virtual ~_zpm_solvable_iter()
    {
    }
};

typedef struct _zpm_solvable_iter zpm_solvable_iter;

void zpm_solvable_iter_destroy(zpm_solvable_iter *iter);

zpm_solvable * zpm_solvable_iter_next(zpm_solvable_iter *it);

#endif
