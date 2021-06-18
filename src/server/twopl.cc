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

uint64_t getLogSequenceNumber()
{
    uint64_t lsn;
    static std::atomic<uint64_t> _lsn(0);
    lsn = atomic_fetch_add(&_lsn, 1);
    return lsn;
}

int loggingWork(Log *log, trans_req *logBuffer, int nlog, int term) {
    entry e;
    int lsn = getLogSequenceNumber(); // lsn == global-log-index
    D(lsn);
    assert(lsn>=0);
    e.lsn = lsn;
    e.term = term;
    e.nlog = nlog;
    memcpy(e.xactSet, logBuffer, sizeof(trans_req) * nlog);
    gettimeofday(&e.time, NULL);
    // Write log to storage
    log->addEntry(e);
    return lsn;
}

void loggingWork(Log *log, append_entries_rpc &arpc) {
    entry e;
    int lsn = arpc.lsn; // lsn == global-log-index
    int nlog = arpc.szLog;
    e.lsn = lsn;
    e.term = arpc.term;
    e.nlog = nlog;
    D(arpc.lsn); D(arpc.szLog); D(arpc.term);
    assert(lsn>=0);
    assert(nlog>0);
    memcpy(e.xactSet, arpc.log, sizeof(trans_req) * nlog);
    gettimeofday(&e.time, NULL);
    // Write log to storage
    log->addEntry(e);
}
