#include <stdio.h>

extern "C" {
#include <solv/repo.h>
#include <solv/pool.h>
#include <solv/poolid.h>
}

#include "zpm.h"
#include "zpm_private.h"
#include "zpm_solvable_iter.h"

class ZSearchIterator : public zpm_solvable_iter
{
public:

    Pool *_pool;
    Id _p;    
    Map _m;

    ZSearchIterator(Pool *pool, const char *what)
        : _pool(pool)
        , _p(1)
    {
        map_init(&_m, _pool->nsolvables);        
        Dataiterator di;
        
        dataiterator_init(&di, pool, 0, 0, 0, what, SEARCH_SUBSTRING|SEARCH_NOCASE);
        dataiterator_set_keyname(&di, SOLVABLE_NAME);
        dataiterator_set_search(&di, 0, 0);
        while (dataiterator_step(&di))
            MAPSET(&_m, di.solvid);
        dataiterator_set_keyname(&di, SOLVABLE_SUMMARY);
        dataiterator_set_search(&di, 0, 0);
        while (dataiterator_step(&di))
            MAPSET(&_m, di.solvid);
        dataiterator_set_keyname(&di, SOLVABLE_DESCRIPTION);
        dataiterator_set_search(&di, 0, 0);
        while (dataiterator_step(&di))
            MAPSET(&_m, di.solvid);
        dataiterator_free(&di);
    }

    virtual ~ZSearchIterator()
    {
        map_free(&_m);
    }
    
    virtual zpm_solvable * next()
    {
        if (_p == 0)
            return 0;
        
        while (_p < _pool->nsolvables)
	{
            Solvable *s = pool_id2solvable(_pool, _p);
            if (MAPTST(&_m, _p)) {                
                return new zpm_solvable(pool_id2solvable(_pool, _p++));
            }
            _p++;            
	}
        _p = 0;
        return 0;
    }
};

zpm_solvable_iter * zpm_query_search(zpm *z, const char *what)
{    
    zpm_solvable_iter *iter = new ZSearchIterator(z->pool, what);
    return iter;    
}
