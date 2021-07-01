#include <assert.h>
#include <sys/time.h>
#include <atomic>

#include "constant.h"
#include "header.h"
#include "config.h"
#include "debug.h"
#include "twopl.h"

bool transactionWork(KVS *kvs, client_request &req)
{
    assert(req.from != req.to);

    // sort
    int tmp;
    if(req.from > req.to){
        tmp = req.from;
        req.from = req.to;
        req.to = tmp;
    }

    if (kvs->lock(req.from) == true) { //thread 4 teishi point
        if (kvs->lock(req.to) == true) {
            //D(buffid);
            return true;
        } else { // [to] lock failure
            kvs->unlock(req.from);
            return false;
        }
    } else {
        return false;
    }
}

void commitWork(KVS *kvs, client_request &req) {
    int fromKey = req.from;
    int toKey = req.to;
    int diffVal = req.diff;

    int fromVal = kvs->index_read(fromKey);
    int toVal = kvs->index_read(toKey);
    fromVal -= diffVal;
    toVal += diffVal;

    kvs->index_update(fromKey, fromVal);
    kvs->index_update(toKey, toVal);

    //unlock
    kvs->unlock(fromKey);
    kvs->unlock(toKey);
}
