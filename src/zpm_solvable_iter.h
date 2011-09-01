
#ifndef _zpm_solvable_iter_H
#define _zpm_solvable_iter_H

#include "zpm.h"

class _zpm_solvable_iter
{
public:
    virtual zpm_solvable * next() = 0;
    virtual ~_zpm_solvable_iter()
    {
    }
};

#endif
