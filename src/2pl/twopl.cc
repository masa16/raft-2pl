#include <assert.h>
#include <sys/time.h>
#include <atomic>

#include "constant.h"
#include "header.h"
#include "config.h"
#include "debug.h"
#include "include/twopl.hh"

void DB::makeDB(uint64_t tuple_num) {
    tuple_num_ = tuple_num;
    kvs_ = new KVS();
}

bool TxnExecutor::transferWork(client_request &req)
{
    assert(req.from != req.to);

    // sort
    int tmp;
    if(req.from > req.to){
        tmp = req.from;
        req.from = req.to;
        req.to = tmp;
    }

    if (db_.kvs_->lock(req.from) == true) { //thread 4 teishi point
        if (db_.kvs_->lock(req.to) == true) {
            //D(buffid);
            return true;
        } else { // [to] lock failure
            db_.kvs_->unlock(req.from);
            return false;
        }
    } else {
        return false;
    }
}

void DB::commitWork(client_request &req) {
    int fromKey = req.from;
    int toKey = req.to;
    int diffVal = req.diff;

    int fromVal = kvs_->index_read(fromKey);
    int toVal = kvs_->index_read(toKey);
    fromVal -= diffVal;
    toVal += diffVal;

    kvs_->index_update(fromKey, fromVal);
    kvs_->index_update(toKey, toVal);

    //unlock
    kvs_->unlock(fromKey);
    kvs_->unlock(toKey);
}
