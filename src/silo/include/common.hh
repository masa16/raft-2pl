#pragma once

#include <pthread.h>
#include <atomic>
#include <iostream>
#include <queue>

#include "tuple.hh"

#include "cache_line_size.hh"
//#include "int64byte.hh"
//#include "masstree_wrapper.hh"

#include "gflags/gflags.h"
#include "glog/logging.h"

#ifdef GLOBAL_VALUE_DEFINE
#define GLOBAL
alignas(CACHE_LINE_SIZE) std::atomic<uint64_t> GlobalEpoch(1);
#else
#define GLOBAL extern
alignas(CACHE_LINE_SIZE) GLOBAL std::atomic<uint64_t> GlobalEpoch;
#endif

#ifdef GLOBAL_VALUE_DEFINE
DEFINE_uint64(clocks_per_us, 2100,
              "CPU_MHz. Use this info for measuring time.");
DEFINE_double(epoch_time, 40, "Epoch interval[msec].");
DEFINE_uint64(extime, 3, "Execution time[sec].");
DEFINE_uint64(max_ope, 10,
              "Total number of operations per single transaction.");
DEFINE_bool(rmw, false,
            "True means read modify write, false means blind write.");
DEFINE_uint64(rratio, 50, "read ratio of single transaction.");
DEFINE_uint64(thread_num, 10, "Total number of worker threads.");
DEFINE_uint64(logger_num, 1, "Total number of logger threads.");
DEFINE_string(affinity, "", "CPU affinity between worker and logger thread, e.g., \"0:1,2,3+4:5,6,7\" (overwrites thread_num&logger_num).");
DEFINE_int64(notifier_cpu, -1, "logical core id for notifier.");
DEFINE_uint64(buffer_num, 2, "Number of log buffers per logger thread.");
DEFINE_uint64(buffer_size, 512, "Size of log buffer in KiB");
DEFINE_uint64(epoch_diff, 0, "Epoch difference threshold to stop transaction (0 for no stop)");
DEFINE_bool(latency_log, false, "Output latency.dat or not");
DEFINE_uint64(tuple_num, 1000000, "Total number of records.");
DEFINE_bool(ycsb, true,
            "True uses zipf_skew, false uses faster random generator.");
DEFINE_double(zipf_skew, 0, "zipf skew. 0 ~ 0.999...");
#else
DECLARE_uint64(clocks_per_us);
DECLARE_double(epoch_time);
DECLARE_uint64(extime);
DECLARE_uint64(max_ope);
DECLARE_bool(rmw);
DECLARE_uint64(rratio);
DECLARE_uint64(thread_num);
DECLARE_uint64(logger_num);
DECLARE_string(affinity);
DECLARE_int64(notifier_cpu);
DECLARE_uint64(buffer_num);
DECLARE_uint64(buffer_size);
DECLARE_uint64(epoch_diff);
DECLARE_bool(latency_log);
DECLARE_uint64(tuple_num);
DECLARE_bool(ycsb);
DECLARE_double(zipf_skew);
#endif

alignas(CACHE_LINE_SIZE) GLOBAL std::atomic<uint64_t> *ThLocalEpoch;
alignas(CACHE_LINE_SIZE) GLOBAL std::atomic<uint64_t> *CTIDW;
alignas(CACHE_LINE_SIZE) GLOBAL std::atomic<uint64_t> *ThLocalDurableEpoch;
alignas(CACHE_LINE_SIZE) GLOBAL std::atomic<uint64_t> DurableEpoch;
