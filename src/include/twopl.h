#pragma once

#include "kvs.h"
#include "log.h"

bool transactionWork(KVS *kvs, client_request &req);
void commitWork(KVS *kvs, client_request &req);
int loggingWork(Log *log, trans_req *logBuffer, int nlog, int term);
void loggingWork(Log *log, append_entries_rpc &arpc);
