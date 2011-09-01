
#include "zpm.h"
#include "zpm_types_private.h"

#include "zpm_solvable_iter.h"

#include <zypp/sat/WhatProvides.h>

using namespace zypp;
using namespace zypp::sat;

class ZWhatProvidesIterator : public zpm_solvable_iter
{
public:

    ZWhatProvidesIterator(const char *what)
        : _wp(new WhatProvides(Capability(what)))
        , _it(NULL)
    {
        
    }

    /**
     * copy constructor
     */
    ZWhatProvidesIterator(const ZWhatProvidesIterator &other)
        : _wp(other._wp)
        , _it(NULL)
    {
        if (other._it) {        
            _it = new WhatProvides::const_iterator;            
            *_it = *other._it;
        }
    }
    
    ZWhatProvidesIterator & operator=(const ZWhatProvidesIterator &other)
    {
        if (&other != this) {           
            _wp = other._wp;

            if (other._it) {
                if (!_it)
                    _it = new WhatProvides::const_iterator;
                *_it = *other._it;
            }            
            else
            {
                delete _it;
                _it = 0;                
            }            
        }
        
        return *this;
    }

    virtual ~ZWhatProvidesIterator()
    {
        delete _it;        
    }
    
    virtual zpm_solvable * next()
    {
        if (!_it) {
            _it = new WhatProvides::const_iterator;
            *_it = _wp->begin();
        }
        else
        {
            ++(*_it);            
        }
        
        if ((*_it) != _wp->end()) {
            return new zpm_solvable(*(*_it));
        }

        return NULL;
    }
    
    shared_ptr<WhatProvides> _wp;
    WhatProvides::const_iterator *_it;    
};

zpm_solvable_iter * zpm_query_what_provides(const char *what)
{    
    zpm_solvable_iter *iter = new ZWhatProvidesIterator(what);
    return iter;    
}
