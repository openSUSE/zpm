extern "C" {
#include <solv/pool.h>
#include <solv/poolid.h>
}

#include "zpm.h"
#include "zpm_private.h"
#include "zpm_solvable_iter.h"

class ZWhatProvidesIterator : public zpm_solvable_iter
{
public:
    Pool *_pool;
    Id _vp;

    ZWhatProvidesIterator(Pool *pool, const char *what)
        : _pool(pool)
        , _vp(pool_whatprovides(pool, pool_str2id(pool, what, 1)))
    {
        
    }

    virtual ~ZWhatProvidesIterator()
    {}
    
    virtual zpm_solvable * next()
    {
        if (_vp == 0)
            return 0;
        
        Id v = _pool->whatprovidesdata[_vp++];
        if (v == 0) {            
            _vp = 0;
            return 0;
        }        
        return new zpm_solvable(pool_id2solvable(_pool, v));
    }
};

zpm_solvable_iter * zpm_query_what_provides(zpm *z, const char *what)
{    
    zpm_solvable_iter *iter = new ZWhatProvidesIterator(z->pool, what);
    return iter;    
}
