
#ifndef ZPM_TYPES_PRIVATE_H
#define ZPM_TYPES_PRIVATE_H

#include <zypp/ZYpp.h>
#include <zypp/sat/Solvable.h>

struct _zpm {
    zypp::ZYpp::Ptr zypp;
};

struct _zpm_solvable : public zypp::sat::Solvable
{
    _zpm_solvable(zypp::sat::Solvable s)
        : Solvable(s)
    {}
};

#endif
