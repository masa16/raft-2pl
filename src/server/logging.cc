#include <assert.h>
#include <sys/time.h>
#include <atomic>

#include "constant.h"
#include "header.h"
#include "config.h"
#include "debug.h"
#include "logging.h"

uint64_t getLogSequenceNumber()
{
    uint64_t lsn;
    static std::atomic<uint64_t> _lsn(0);
    lsn = _lsn.fetch_add(1);
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
