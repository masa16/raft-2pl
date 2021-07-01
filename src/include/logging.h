#pragma once

#include "log.h"

int loggingWork(Log *log, trans_req *logBuffer, int nlog, int term);
void loggingWork(Log *log, append_entries_rpc &arpc);
